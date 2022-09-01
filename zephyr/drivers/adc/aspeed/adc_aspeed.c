/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_adc

#include <errno.h>

#include <drivers/adc.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "../adc_context.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_aspeed);
#include "adc_aspeed.h"

#include <drivers/clock_control.h>
#include <drivers/reset_control.h>

struct adc_aspeed_data {
	struct adc_context ctx;
	const struct device *dev;
	uint32_t channels;
	uint32_t clk_rate;
	uint32_t sampling_rate;
	bool battery_sensing_enable;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	bool calibrate;
	int cv;
	struct k_thread thread;
	struct k_sem acq_sem;

	K_KERNEL_STACK_MEMBER(stack,
			      CONFIG_ADC_ASPEED_ACQUISITION_THREAD_STACK_SIZE);
};

struct aspeed_adc_trim_locate {
	const unsigned int offset;
	const unsigned int field;
};
struct adc_aspeed_cfg {
	struct adc_register_s *base;
	uint32_t scu_base;
	bool trim_valid;
	struct aspeed_adc_trim_locate trim_locate;
	const struct device *clock_dev;
	const reset_control_subsys_t rst_id;
	const uint16_t ref_voltage_mv;
};

#define DEV_CFG(dev) ((const struct adc_aspeed_cfg *const)(dev)->config)
#define DEV_DATA(dev) ((struct adc_aspeed_data *)(dev)->data)

static uint32_t aspeed_adc_read_raw(const struct device *dev, uint32_t channel)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_register_s *adc_register = config->base;
	uint32_t raw_data;

	raw_data =
		(channel & 0x1) ?
		adc_register->adc_data[channel >> 1].fields.data_even :
		adc_register->adc_data[channel >> 1].fields.data_odd;
	LOG_DBG("%u\n", raw_data);
	return raw_data;
}

static uint32_t aspeed_adc_battery_read(const struct device *dev,
					uint32_t channel)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_register_s *adc_register = config->base;
	uint32_t raw_data;
	union adc_engine_control_s engine_ctrl;

	engine_ctrl.value = adc_register->engine_ctrl.value;
	engine_ctrl.fields.channel_7_selection = Ch7_BATTERY_MODE;
	engine_ctrl.fields.enable_battery_sensing = 1;
	adc_register->engine_ctrl.value = engine_ctrl.value;
	/* After enable battery sensing need to wait 1ms for adc stable */
	k_msleep(1);
	raw_data = aspeed_adc_read_raw(dev, channel);
	engine_ctrl.fields.channel_7_selection = Ch7_NORMAL_MODE;
	engine_ctrl.fields.enable_battery_sensing = 0;
	adc_register->engine_ctrl.value = engine_ctrl.value;
	return raw_data;
}

static int adc_aspeed_read_channel(const struct device *dev, uint8_t channel,
				   int32_t *result)
{
	struct adc_aspeed_data *priv = DEV_DATA(dev);

	if (priv->battery_sensing_enable && channel == 7) {
		*result = aspeed_adc_battery_read(dev, channel);
	} else {
		*result = aspeed_adc_read_raw(dev, channel);
	}
	return 0;
}

static int adc_aspeed_set_rate(const struct device *dev, uint32_t rate)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_aspeed_data *priv = DEV_DATA(dev);
	struct adc_register_s *adc_register = config->base;
	uint32_t clk_src, divisor;
	union adc_clock_control_s adc_clk_ctrl;

	/*
	 * Formula of adc clock:
	 * ADC clock = ADC src clock / (divisor_of_adc_clock + 1)
	 * ADC sampling rate = ADC clock / 12
	 */
	clock_control_get_rate(config->clock_dev, NULL, &clk_src);
	divisor = (clk_src / rate) - 1;
	if (divisor >= BIT(16)) {
		LOG_ERR("clock freq %d out of range\n", rate);
		return -ERANGE;
	}
	priv->sampling_rate = ((divisor + 1) * ASPEED_CLOCKS_PER_SAMPLE) *
			      (clk_src / USEC_PER_SEC);
	LOG_DBG("sampling rate = %dus\n", priv->sampling_rate);
	adc_clk_ctrl.value = adc_register->adc_clk_ctrl.value;
	adc_clk_ctrl.fields.divisor_of_adc_clock = divisor;
	adc_register->adc_clk_ctrl.value = adc_clk_ctrl.value;
	return 0;
}

static int aspeed_adc_set_trim_data(const struct adc_aspeed_cfg *config)
{
	uint32_t scu_otp, trimming_val;
	struct adc_register_s *adc_register = config->base;
	union adc_compensating_trimming_s comp_trim;

	if (!config->trim_locate.field || !config->scu_base) {
		return -EINVAL;
	}
	scu_otp = sys_read32(config->scu_base + config->trim_locate.offset);
	trimming_val = (scu_otp & config->trim_locate.field) >>
		       (find_lsb_set(config->trim_locate.field) - 1);

	LOG_INF("ADC-%08x trimming_val = %x", (uint32_t)config->base, trimming_val);
	comp_trim.value = adc_register->comp_trim.value;
	comp_trim.fields.trimming_value = trimming_val;
	adc_register->comp_trim.value = comp_trim.fields.trimming_value;

	return 0;
}

static void aspeed_adc_calibration(const struct device *dev)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_aspeed_data *priv = DEV_DATA(dev);
	struct adc_register_s *adc_register = config->base;
	uint32_t raw_data = 0, index, backup_value;
	union adc_engine_control_s engine_ctrl;
	int ret;

	/*TODO: trimming data setting */
	if (config->trim_valid) {
		ret = aspeed_adc_set_trim_data(config);
		if (ret) {
			LOG_WRN("Set trimming data fail = %d", ret);
		}
	}

	/* Enable Compensating Sensing mode */
	backup_value = adc_register->engine_ctrl.value;
	engine_ctrl.value = backup_value;
	engine_ctrl.fields.compensating_sensing_mode = 1;
	engine_ctrl.fields.channel_enable = BIT(0);
	adc_register->engine_ctrl.value = engine_ctrl.value;
	/* After enable compensating sensing need to wait 1ms for adc stable */
	k_msleep(1);
	for (index = 0; index < ASPEED_CV_SAMPLE_TIMES; index++) {
		k_usleep(priv->sampling_rate);
		raw_data += adc_register->adc_data[0].fields.data_odd;
	}
	raw_data /= ASPEED_CV_SAMPLE_TIMES;
	/*
	 * At compensating sensing mode hardware will generate the voltage is half of the
	 * ref. voltage.
	 */
	priv->cv = (int)(BIT(ASPEED_RESOLUTION_BITS - 1) - raw_data);

	LOG_DBG("Compensating value = %d\n", priv->cv);
	/* Restore backup value */
	adc_register->engine_ctrl.value = backup_value;
}

static int adc_aspeed_set_ref_voltage(const struct device *dev)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_register_s *adc_register = config->base;
	uint32_t ref_voltage_cfg;
	union adc_engine_control_s engine_ctrl;

	/*
	 * Set ref_voltage:
	 * If reference voltage is between 1550~1650mv, we can set
	 * fields either REF_VOLTAGE_EXT_HIGH or REF_VOLTAGE_EXT_LOW.
	 * In this place, we select REF_VOLTAGE_EXT_HIGH as higher priority.
	 */
	if (config->ref_voltage_mv == 2500) {
		ref_voltage_cfg = REF_VOLTAGE_2500mV;
	} else if (config->ref_voltage_mv == 1200) {
		ref_voltage_cfg = REF_VOLTAGE_1200mV;
	} else if ((config->ref_voltage_mv >= 1550) &&
		   (config->ref_voltage_mv <= 2700)) {
		ref_voltage_cfg = REF_VOLTAGE_EXT_HIGH;
	} else if ((config->ref_voltage_mv >= 900) &&
		   (config->ref_voltage_mv <= 1650)) {
		ref_voltage_cfg = REF_VOLTAGE_EXT_LOW;
	} else {
		return -ERANGE;
	}
	engine_ctrl.value = adc_register->engine_ctrl.value;
	engine_ctrl.fields.reference_voltage_selection = ref_voltage_cfg;
	adc_register->engine_ctrl.value = engine_ctrl.value;
	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_aspeed_data *priv =
		CONTAINER_OF(ctx, struct adc_aspeed_data, ctx);

	priv->channels = ctx->sequence.channels;
	priv->repeat_buffer = priv->buffer;

	k_sem_give(&priv->acq_sem);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_aspeed_data *data =
		CONTAINER_OF(ctx, struct adc_aspeed_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_aspeed_start_read(const struct device *dev,
				 const struct adc_sequence *sequence)
{
	struct adc_aspeed_data *priv = DEV_DATA(dev);

	if (sequence->resolution != 10) {
		LOG_ERR("unsupported resolution %d", sequence->resolution);
		return -ENOTSUP;
	}

	if (find_msb_set(sequence->channels) > ASPEED_ADC_CH_NUMBER + 1) {
		LOG_ERR("unsupported channels in mask: 0x%08x",
			sequence->channels);
		return -ENOTSUP;
	}

	priv->buffer = sequence->buffer;
	priv->calibrate = sequence->calibrate;
	adc_context_start_read(&priv->ctx, sequence);

	return adc_context_wait_for_completion(&priv->ctx);
}

static int adc_aspeed_read_async(const struct device *dev,
				 const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	struct adc_aspeed_data *priv = DEV_DATA(dev);
	int err;

	adc_context_lock(&priv->ctx, async ? true : false, async);
	err = adc_aspeed_start_read(dev, sequence);
	adc_context_release(&priv->ctx, err);

	return err;
}

static int adc_aspeed_read(const struct device *dev,
			   const struct adc_sequence *sequence)
{
	return adc_aspeed_read_async(dev, sequence, NULL);
}

static int adc_aspeed_channel_setup(const struct device *dev,
				    const struct adc_channel_cfg *channel_cfg)
{
	struct adc_aspeed_data *priv = DEV_DATA(dev);
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_id > ASPEED_ADC_CH_NUMBER) {
		LOG_ERR("Channel %d is not valid", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Unsupported channel acquisition time");
		return -ENOTSUP;
	}

	/* The last channel have gain feature for battery sensing */
	if (channel_id == ASPEED_ADC_CH_NUMBER) {
		if (channel_cfg->gain != ADC_GAIN_1) {
			if ((config->ref_voltage_mv < 1550 &&
			     channel_cfg->gain != ADC_GAIN_1_3) ||
			    (config->ref_voltage_mv >= 1550 &&
			     channel_cfg->gain != ADC_GAIN_2_3)) {
				LOG_ERR("Invalid channel gain");
				return -EINVAL;
			}
			priv->battery_sensing_enable = 1;
		} else {
			priv->battery_sensing_enable = 0;
		}
	} else {
		if (channel_cfg->gain != ADC_GAIN_1) {
			LOG_ERR("unsupported channel gain '%d'",
				channel_cfg->gain);
			return -ENOTSUP;
		}
	}

	return 0;
}

static uint16_t adc_aspeed_get_ref(const struct device *dev)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);

	return config->ref_voltage_mv;
}

static void aspeed_acquisition_thread(struct adc_aspeed_data *data)
{
	int32_t result = 0;
	uint8_t channel;
	int err;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);

		while (data->channels) {
			channel = find_lsb_set(data->channels) - 1;

			LOG_DBG("reading channel %d", channel);

			err = adc_aspeed_read_channel(data->dev, channel,
						      &result);
			if (data->calibrate) {
				result += data->cv;
			}

			if (err) {
				adc_context_complete(&data->ctx, err);
				break;
			}

			LOG_DBG("finished channel %d, result = %d", channel,
				result);

			/*
			 * ADC samples are stored as int32_t regardless of the
			 * resolution in order to provide a uniform interface
			 * for the driver.
			 */
			*data->buffer++ = result;
			WRITE_BIT(data->channels, channel, 0);
		}

		adc_context_on_sampling_done(&data->ctx, data->dev);
	}
}

static int aspeed_adc_engine_init(const struct device *dev,
				  uint32_t timeout_ms)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_register_s *adc_register = config->base;
	union adc_engine_control_s engine_ctrl;
	int ret;

	engine_ctrl.value = adc_register->engine_ctrl.value;
	engine_ctrl.fields.adc_operation_mode = ADC_NORMAL;
	engine_ctrl.fields.engine_enable = 1;
	adc_register->engine_ctrl.value = engine_ctrl.value;

	ret = reg_read_poll_timeout(
		adc_register, engine_ctrl, engine_ctrl,
		engine_ctrl.fields.initial_sequence_complete, 0, timeout_ms);

	if (ret) {
		return ret;
	}

	engine_ctrl.fields.channel_enable = 0xff;
	adc_register->engine_ctrl.value = engine_ctrl.value;

	return 0;
}

static int adc_aspeed_init(const struct device *dev)
{
	const struct adc_aspeed_cfg *config = DEV_CFG(dev);
	struct adc_aspeed_data *priv = DEV_DATA(dev);
	const struct device *reset_dev =
		device_get_binding(ASPEED_RST_CTRL_NAME);
	int ret;
	k_tid_t tid;

	priv->dev = dev;

	k_sem_init(&priv->acq_sem, 0, 1);

	reset_control_deassert(reset_dev, config->rst_id);

	ret = adc_aspeed_set_ref_voltage(dev);
	if (ret) {
		return ret;
	}

	ret = adc_aspeed_set_rate(dev, ASPEED_CLOCK_FREQ_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = aspeed_adc_engine_init(dev, 1000);
	if (ret) {
		return ret;
	}
	aspeed_adc_calibration(dev);

	tid = k_thread_create(&priv->thread, priv->stack,
			      CONFIG_ADC_ASPEED_ACQUISITION_THREAD_STACK_SIZE,
			      (k_thread_entry_t)aspeed_acquisition_thread, priv,
			      NULL, NULL,
			      CONFIG_ADC_ASPEED_ACQUISITION_THREAD_PRIO, 0,
			      K_NO_WAIT);
	k_thread_name_set(tid, dev->name);

	adc_context_unlock_unconditionally(&priv->ctx);

	return 0;
}

static struct adc_driver_api adc_aspeed_api = {
	.channel_setup = adc_aspeed_channel_setup,
	.read = adc_aspeed_read,
	.get_ref = adc_aspeed_get_ref,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_aspeed_read_async,
#endif
};

#define ASPEED_ADC_INIT(n)							       \
	static struct adc_aspeed_data adc_aspeed_data_##n = {			       \
		ADC_CONTEXT_INIT_TIMER(adc_aspeed_data_##n, ctx),		       \
		ADC_CONTEXT_INIT_LOCK(adc_aspeed_data_##n, ctx),		       \
		ADC_CONTEXT_INIT_SYNC(adc_aspeed_data_##n, ctx),		       \
	};									       \
	static const struct adc_aspeed_cfg adc_aspeed_cfg_##n = {		       \
		.base = (struct adc_register_s *)DT_INST_REG_ADDR(n),		       \
		.scu_base = DT_REG_ADDR_BY_IDX(DT_INST_PHANDLE(n, aspeed_scu), 0),     \
		.trim_valid = DT_INST_PROP_OR(n, aspeed_trim_data_valid, false),       \
		.trim_locate = { DT_INST_PROP_BY_IDX(n, aspeed_trim_data_locate, 0),   \
				 DT_INST_PROP_BY_IDX(n, aspeed_trim_data_locate, 1) }, \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		       \
		.rst_id = (reset_control_subsys_t)DT_INST_RESETS_CELL(n,	       \
								      rst_id),	       \
		.ref_voltage_mv = DT_INST_PROP_OR(n, ref_voltage_mv, 2500),	       \
	};									       \
	DEVICE_DT_INST_DEFINE(n, adc_aspeed_init, NULL,				       \
			      &adc_aspeed_data_##n, &adc_aspeed_cfg_##n,	       \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	       \
			      &adc_aspeed_api);

DT_INST_FOREACH_STATUS_OKAY(ASPEED_ADC_INIT)
