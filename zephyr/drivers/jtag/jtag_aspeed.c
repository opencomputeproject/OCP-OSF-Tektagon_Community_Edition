/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * JTAG opertion priority:
 * HW mode 2 > SW mode > HW mode 1
 */

#define DT_DRV_COMPAT aspeed_jtag

#include <stdlib.h>
#include <errno.h>
#include <drivers/jtag.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <soc.h>

#define LOG_LEVEL CONFIG_JTAG_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(jtag_aspeed);
#include "jtag_aspeed.h"

#include <portability/cmsis_os2.h>
#include <drivers/clock_control.h>
#include <drivers/reset_control.h>

#define DEFAULT_JTAG_FREQ 1000000

struct name_mapping {
	enum tap_state symbol;
	const char *name;
};
/* NOTE:  do not change these state names.  They're documented,
 * and we rely on them to match SVF input (except for "RUN/IDLE").
 */
static const struct name_mapping tap_name_mapping[] = {
	{
		TAP_RESET,
		"RESET",
	},
	{
		TAP_IDLE,
		"RUN/IDLE",
	},
	{
		TAP_DRSELECT,
		"DRSELECT",
	},
	{
		TAP_DRCAPTURE,
		"DRCAPTURE",
	},
	{
		TAP_DRSHIFT,
		"DRSHIFT",
	},
	{
		TAP_DREXIT1,
		"DREXIT1",
	},
	{
		TAP_DRPAUSE,
		"DRPAUSE",
	},
	{
		TAP_DREXIT2,
		"DREXIT2",
	},
	{
		TAP_DRUPDATE,
		"DRUPDATE",
	},
	{
		TAP_IRSELECT,
		"IRSELECT",
	},
	{
		TAP_IRCAPTURE,
		"IRCAPTURE",
	},
	{
		TAP_IRSHIFT,
		"IRSHIFT",
	},
	{
		TAP_IREXIT1,
		"IREXIT1",
	},
	{
		TAP_IRPAUSE,
		"IRPAUSE",
	},
	{
		TAP_IREXIT2,
		"IREXIT2",
	},
	{
		TAP_IRUPDATE,
		"IRUPDATE",
	},

	/* only for input:  accept standard SVF name */
	{
		TAP_IDLE,
		"IDLE",
	},
};

struct jtag_aspeed_data {
	uint32_t fifo_length;
	enum tap_state state;
	osEventFlagsId_t evt_id;
	bool sw_tdi;
};

struct jtag_aspeed_cfg {
	struct jtag_register_s *base;
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	const reset_control_subsys_t rst_id;
	void (*irq_config_func)(const struct device *dev);
};

#define DEV_CFG(dev) ((const struct jtag_aspeed_cfg *const)(dev)->config)
#define DEV_DATA(dev) ((struct jtag_aspeed_data *)(dev)->data)

static const char *tap_state_name(enum tap_state state)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(tap_name_mapping); i++) {
		if (tap_name_mapping[i].symbol == state) {
			return tap_name_mapping[i].name;
		}
	}
	return "???";
}

static int jtag_aspeed_tap_state_check(const struct device *dev)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;

	if ((jtag_register->software_mode_and_status.fields.instr_xfer_pause &&
	     priv->state != TAP_IRPAUSE) ||
	    (jtag_register->software_mode_and_status.fields.data_xfer_pause &&
	     priv->state != TAP_DRPAUSE) ||
	    (jtag_register->software_mode_and_status.fields.engine_idle &&
	     priv->state != TAP_IDLE)) {
		return -EIO;
	}
	return 0;
}

static void jtag_aspeed_wait_ir_pause_complete(struct jtag_aspeed_data *priv)
{
	osEventFlagsWait(priv->evt_id, JTAG_ASPEED_INST_PAUSE, osFlagsWaitAll,
			 osWaitForever);
}

static void jtag_aspeed_wait_ir_complete(struct jtag_aspeed_data *priv)
{
	osEventFlagsWait(priv->evt_id, JTAG_ASPEED_INST_COMPLETE,
			 osFlagsWaitAll, osWaitForever);
}

static void jtag_aspeed_wait_dr_pause_complete(struct jtag_aspeed_data *priv)
{
	osEventFlagsWait(priv->evt_id, JTAG_ASPEED_DATA_PAUSE, osFlagsWaitAll,
			 osWaitForever);
}

static inline void jtag_aspeed_wait_dr_complete(struct jtag_aspeed_data *priv)
{
	osEventFlagsWait(priv->evt_id, JTAG_ASPEED_DATA_COMPLETE,
			 osFlagsWaitAll, osWaitForever);
}

static void jtag_aspeed_isr(const struct device *dev)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	uint32_t int_pending = jtag_register->mode_1_int_ctrl.value &
			       JTAG_ASPEED_INT_PEND_MASK;

	osEventFlagsSet(priv->evt_id, int_pending);
	/* W1C interrupt pending */
	jtag_register->mode_1_int_ctrl.value =
		jtag_register->mode_1_int_ctrl.value;
}

static int jtag_aspeed_tap_idle(const struct device *dev)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	union mode_1_control_s mode_1_control;
	int ret;

	ret = jtag_aspeed_tap_state_check(dev);
	if (ret) {
		return ret;
	}
	if (priv->state == TAP_IDLE) {
		return 0;
	}
	mode_1_control.value = jtag_register->mode_1_control.value;
	mode_1_control.fields.xfer_len = 0;
	mode_1_control.fields.terminating_xfer = 1;
	mode_1_control.fields.last_xfer = 1;
	LOG_DBG("mode_1_ctrl = 0x%08x, status = 0x%08x\n",
		jtag_register->mode_1_control.value,
		jtag_register->software_mode_and_status.value);
	if (priv->state == TAP_IRPAUSE) {
		mode_1_control.fields.ir_xfer_en = 1;
		jtag_register->mode_1_control.value = mode_1_control.value;
		jtag_aspeed_wait_ir_complete(priv);
		mode_1_control.fields.ir_xfer_en = 0;
		jtag_register->mode_1_control.value = mode_1_control.value;
	} else if (priv->state == TAP_DRPAUSE) {
		mode_1_control.fields.dr_xfer_en = 1;
		jtag_register->mode_1_control.value = mode_1_control.value;
		jtag_aspeed_wait_dr_complete(priv);
		mode_1_control.fields.dr_xfer_en = 0;
		jtag_register->mode_1_control.value = mode_1_control.value;
	}
	mode_1_control.fields.terminating_xfer = 0;
	jtag_register->mode_1_control.value = mode_1_control.value;
	priv->state = TAP_IDLE;
	return 0;
}

static int jtag_aspeed_tap_reset(const struct device *dev)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	union mode_1_control_s mode_1_control;

	/* Disable SW mode */
	jtag_register->software_mode_and_status.value = 0;
	mode_1_control.value = jtag_register->mode_1_control.value;
	/* Enable HW mode 1 */
	mode_1_control.fields.engine_enable = 1;
	/* Reset target tap */
	mode_1_control.fields.reset_to_tlr = 1;
	jtag_register->mode_1_control.value = mode_1_control.value;
	while (jtag_register->mode_1_control.fields.reset_to_tlr)
		;
	priv->state = TAP_IDLE;
	return 0;
}

static int jtag_aspeed_sw_xfer(const struct device *dev, enum jtag_pin pin,
			       uint8_t value)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	union software_mode_and_status_s software_mode_and_status;

	software_mode_and_status.value =
		jtag_register->software_mode_and_status.value;
	software_mode_and_status.fields.software_mode_enable = 1;
	software_mode_and_status.fields.software_tdi_and_tdo = priv->sw_tdi;
	switch (pin) {
	case JTAG_TDI:
		LOG_DBG("JTAG_TDI = %d\t", value);
		priv->sw_tdi = value;
		software_mode_and_status.fields.software_tdi_and_tdo = value;
		break;
	case JTAG_TCK:
		LOG_DBG("JTAG_TCK = %d\t", value);
		software_mode_and_status.fields.software_tck = value;
		break;
	case JTAG_TMS:
		LOG_DBG("JTAG_TMS = %d\t", value);
		software_mode_and_status.fields.software_tms = value;
		break;
	case JTAG_ENABLE:
	case JTAG_TRST:
	default:
		return -EINVAL;
	}
	jtag_register->software_mode_and_status.value =
		software_mode_and_status.value;
	LOG_DBG("Register value 0x%08x\n", software_mode_and_status.value);
	return 0;
}

static int jtag_aspeed_tdo_get(const struct device *dev, uint8_t *value)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	union software_mode_and_status_s software_mode_and_status;

	software_mode_and_status.value =
		jtag_register->software_mode_and_status.value;
	software_mode_and_status.fields.software_mode_enable = 1;
	software_mode_and_status.fields.software_tdi_and_tdo = priv->sw_tdi;
	jtag_register->software_mode_and_status.value =
		software_mode_and_status.value;
	*value = jtag_register->software_mode_and_status.fields.software_tdi_and_tdo;
	LOG_DBG("JTAG_TDO = %d\t", *value);
	LOG_DBG("Register value 0x%08x\n", software_mode_and_status.value);
	return 0;
}

static int jtag_aspeed_tck_cycle(const struct device *dev, uint8_t tms,
				 uint8_t tdi, uint8_t *tdo)
{
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	union software_mode_and_status_s software_mode_and_status;

	software_mode_and_status.value =
		jtag_register->software_mode_and_status.value;
	software_mode_and_status.fields.software_mode_enable = 1;
	/* TCK = 0 */
	software_mode_and_status.fields.software_tck = 0;
	software_mode_and_status.fields.software_tdi_and_tdo = tdi;
	software_mode_and_status.fields.software_tms = tms;
	jtag_register->software_mode_and_status.value =
		software_mode_and_status.value;
	/* TCK = 1 */
	software_mode_and_status.fields.software_tck = 1;
	jtag_register->software_mode_and_status.value =
		software_mode_and_status.value;
	*tdo = jtag_register->software_mode_and_status.fields
	       .software_tdi_and_tdo;
	return 0;
}

int jtag_aspeed_freq_get(const struct device *dev, uint32_t *freq)
{
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	uint32_t src_clk, div;

	clock_control_get_rate(config->clock_dev, config->clk_id, &src_clk);
	div = jtag_register->tck_control.fields.tck_divisor;
	*freq = src_clk / (div + 1);
	return 0;
}
int jtag_aspeed_freq_set(const struct device *dev, uint32_t freq)
{
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	uint32_t src_clk, div, diff;
	union tck_control_s tck_control;

	if (freq > JTAG_ASPEED_MAX_FREQUENCY) {
		return -EINVAL;
	}
	clock_control_get_rate(config->clock_dev, config->clk_id, &src_clk);
	div = DIV_ROUND_UP(src_clk, freq);
	diff = abs(src_clk - div * freq);
	if (diff > abs(src_clk - (div - 1) * freq)) {
		div = div - 1;
	}
	/* TCK freq = HCLK / (tck_divisor + 1) */
	if (div >= 1) {
		div = div - 1;
	}
	LOG_DBG("tck divisor = %d, tck freq = %d\n", div, src_clk / (div + 1));
	tck_control.value = jtag_register->tck_control.value;
	tck_control.fields.tck_divisor = div;
	jtag_register->tck_control.value = tck_control.value;
	return 0;
}
int jtag_aspeed_tap_set(const struct device *dev, enum tap_state state)
{
	int ret;

	if (state == TAP_IDLE) {
		ret = jtag_aspeed_tap_idle(dev);
	} else if (state == TAP_RESET) {
		ret = jtag_aspeed_tap_reset(dev);
	} else {
		ret = -ENOTSUP;
	}
	if (ret) {
		LOG_ERR("Move tap state to %s fail\n", tap_state_name(state));
	} else {
		LOG_DBG("Move tap state to %s\n", tap_state_name(state));
	}
	return ret;
}
int jtag_aspeed_tap_get(const struct device *dev, enum tap_state *state)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	int ret;

	ret = jtag_aspeed_tap_state_check(dev);
	if (ret) {
		return ret;
	}
	*state = priv->state;
	return 0;
}
int jtag_aspeed_tck_run(const struct device *dev, uint32_t run_count)
{
	uint32_t i;
	uint8_t dummy;

	for (i = 0; i < run_count; i++)
		jtag_aspeed_tck_cycle(dev, 0, 0, &dummy);
	return 0;
}
int jtag_aspeed_xfer(const struct device *dev, struct scan_command_s *scan)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	uint32_t remain_xfer = scan->fields.num_bits;
	uint32_t xfer_size;
	uint32_t shift_index = 0, curr_out_index = 0, curr_in_index = 0;
	uint8_t end_xfer;
	const uint32_t *out_value = (const uint32_t *)scan->fields.out_value;
	uint32_t *in_value = (uint32_t *)scan->fields.in_value;
	union mode_1_control_s mode_1_control;

	/* Disable SW mode */
	jtag_register->software_mode_and_status.value = 0;
	/* Clear internal fifo */
	mode_1_control.value = jtag_register->mode_1_control.value;
	mode_1_control.fields.reset_internal_fifo = 1;
	jtag_register->mode_1_control.value = mode_1_control.value;
	while (jtag_register->mode_1_control.fields.reset_internal_fifo)
		;
	/* Enable HW mode 1 */
	mode_1_control.value = jtag_register->mode_1_control.value;
	mode_1_control.fields.engine_enable = 1;
	mode_1_control.fields.msb_first = 0;
	mode_1_control.fields.last_xfer = 0;
	LOG_DBG("scan info: %sScan size:%d end_state:%s\n",
		scan->ir_scan ? "IR" : "DR", scan->fields.num_bits,
		tap_state_name(scan->end_state));
	while (remain_xfer) {
		if (remain_xfer > priv->fifo_length) {
			end_xfer = 0;
			xfer_size = priv->fifo_length;
			shift_index += priv->fifo_length >> 5;
		} else {
			end_xfer = 1;
			xfer_size = remain_xfer;
			shift_index += remain_xfer >> 5;
			if (remain_xfer & 0x1f) {
				shift_index++;
			}
		}
		/* Write out data to FIFO */
		for (; curr_out_index < shift_index; curr_out_index++) {
			if (xfer_size < 32) {
				LOG_DBG("out_value[%d] = %08x\n",
					curr_out_index,
					out_value[curr_out_index] &
					(uint32_t)GENMASK(xfer_size - 1, 0));
				jtag_register->data_for_hw_mode_1[0].value =
					out_value[curr_out_index] &
					(uint32_t)GENMASK(xfer_size - 1, 0);
			} else {
				LOG_DBG("out_value[%d] = %08x\n",
					curr_out_index,
					out_value[curr_out_index]);
				jtag_register->data_for_hw_mode_1[0].value =
					out_value[curr_out_index];
			}
		}
		if (end_xfer && scan->end_state == TAP_IDLE) {
			mode_1_control.fields.last_xfer = 1;
		}
		mode_1_control.fields.xfer_len = xfer_size;
		LOG_DBG("Transfer ctrl: 0x%08x\n", mode_1_control.value);
		/* Enable transfer */
		if (scan->ir_scan) {
			mode_1_control.fields.ir_xfer_en = 1;
			jtag_register->mode_1_control.value =
				mode_1_control.value;
			if (mode_1_control.fields.last_xfer) {
				jtag_aspeed_wait_ir_complete(priv);
			} else {
				jtag_aspeed_wait_ir_pause_complete(priv);
			}
			mode_1_control.fields.ir_xfer_en = 0;
			jtag_register->mode_1_control.value =
				mode_1_control.value;
		} else {
			mode_1_control.fields.dr_xfer_en = 1;
			jtag_register->mode_1_control.value =
				mode_1_control.value;
			if (jtag_register->mode_1_control.fields.last_xfer) {
				jtag_aspeed_wait_dr_complete(priv);
			} else {
				jtag_aspeed_wait_dr_pause_complete(priv);
			}
			mode_1_control.fields.dr_xfer_en = 0;
			jtag_register->mode_1_control.value =
				mode_1_control.value;
		}
		remain_xfer -= xfer_size;
		/* Get in data to fifo */
		if (in_value != NULL) {
			for (; curr_in_index < shift_index; curr_in_index++) {
				if (xfer_size < 32) {
					in_value[curr_in_index] =
						jtag_register
						->data_for_hw_mode_1[0]
						.value >>
						(32 - xfer_size);
				} else {
					in_value[curr_in_index] =
						jtag_register
						->data_for_hw_mode_1[0]
						.value;
				}
				LOG_DBG("in_value[%d] = %08x\n", curr_in_index,
					in_value[curr_in_index]);
				xfer_size -= 32;
			}
		} else {
			for (; curr_in_index < shift_index; curr_in_index++) {
				if (xfer_size < 32) {
					LOG_DBG("in_value[%d] = %08x\n",
						curr_in_index,
						jtag_register->data_for_hw_mode_1
						[0]
						.value >>
						(32 - xfer_size));
				} else {
					LOG_DBG("in_value[%d] = %08x\n",
						curr_in_index,
						jtag_register
						->data_for_hw_mode_1[0]
						.value);
				}
			}
		}
	}
	priv->state = scan->end_state;
	return 0;
}

static int jtag_aspeed_init(const struct device *dev)
{
	struct jtag_aspeed_data *priv = DEV_DATA(dev);
	const struct jtag_aspeed_cfg *config = DEV_CFG(dev);
	struct jtag_register_s *jtag_register = config->base;
	union mode_1_int_ctrl_s mode_1_int_ctrl;
	union mode_1_control_s mode_1_control;
	const struct device *reset_dev =
		device_get_binding(ASPEED_RST_CTRL_NAME);

	reset_control_assert(reset_dev, config->rst_id);
	reset_control_deassert(reset_dev, config->rst_id);
	priv->sw_tdi = 0;
	priv->state = TAP_IDLE;
	priv->evt_id = osEventFlagsNew(NULL);
	config->irq_config_func(dev);
	/* Enable all of the interrupt */
	mode_1_int_ctrl.value = jtag_register->mode_1_int_ctrl.value;
	mode_1_int_ctrl.fields.enable_of_data_xfer_completed = 1;
	mode_1_int_ctrl.fields.enable_of_data_xfer_pause = 1;
	mode_1_int_ctrl.fields.enable_of_instr_xfer_completed = 1;
	mode_1_int_ctrl.fields.enable_of_instr_xfer_pause = 1;
	jtag_register->mode_1_int_ctrl.value = mode_1_int_ctrl.value;
	jtag_aspeed_freq_set(dev, DEFAULT_JTAG_FREQ);
	/* Output enable */
	mode_1_control.value = jtag_register->mode_1_control.value;
	mode_1_control.fields.engine_output_enable = 1;
	jtag_register->mode_1_control.value = mode_1_control.value;
	return 0;
}

static struct jtag_driver_api jtag_aspeed_api = {
	.freq_get = jtag_aspeed_freq_get,
	.freq_set = jtag_aspeed_freq_set,
	.tap_get = jtag_aspeed_tap_get,
	.tap_set = jtag_aspeed_tap_set,
	.tck_run = jtag_aspeed_tck_run,
	.xfer = jtag_aspeed_xfer,
	.sw_xfer = jtag_aspeed_sw_xfer,
	.tdo_get = jtag_aspeed_tdo_get,
};

#define ASPEED_JTAG_INIT(n)						       \
	static struct jtag_aspeed_data jtag_aspeed_data_##n = {		       \
		.fifo_length = 512,					       \
	};								       \
	static void jtag_aspeed_config_func_##n(const struct device *dev);     \
	static const struct jtag_aspeed_cfg jtag_aspeed_cfg_##n = {	       \
		.base = (struct jtag_register_s *)DT_INST_REG_ADDR(n),	       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	       \
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n,       \
								      clk_id), \
		.rst_id = (reset_control_subsys_t)DT_INST_RESETS_CELL(n,       \
								      rst_id), \
		.irq_config_func = jtag_aspeed_config_func_##n,		       \
	};								       \
	DEVICE_DT_INST_DEFINE(n, jtag_aspeed_init, NULL,		       \
			      &jtag_aspeed_data_##n, &jtag_aspeed_cfg_##n,     \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &jtag_aspeed_api);			       \
	static void jtag_aspeed_config_func_##n(const struct device *dev)      \
	{								       \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),	       \
			    jtag_aspeed_isr, DEVICE_DT_INST_GET(n), 0);	       \
									       \
		irq_enable(DT_INST_IRQN(n));				       \
	}

DT_INST_FOREACH_STATUS_OKAY(ASPEED_JTAG_INIT)
