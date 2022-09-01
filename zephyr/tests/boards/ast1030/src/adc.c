/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <tc_util.h>
#include <drivers/adc.h>
#include "ast_test.h"

#define LOG_MODULE_NAME adc_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if DT_HAS_COMPAT_STATUS_OKAY(aspeed_adc)
#define ASPEED_ADC_CH_NUMBER 8
#define DT_DRV_COMPAT aspeed_adc
#else
#error No known devicetree compatible match for ADC test
#endif

#define NODE_LABELS(n) DT_INST_LABEL(n),
#define ADC_HDL_LIST_ENTRY(label)      \
	{			       \
		.device_label = label, \
	}
#define INIT_MACRO() DT_INST_FOREACH_STATUS_OKAY(NODE_LABELS) "NA"
static struct adc_hdl {
	char *device_label;
	struct adc_channel_cfg channel_config[ASPEED_ADC_CH_NUMBER];
} adc_list[] = {
	FOR_EACH(ADC_HDL_LIST_ENTRY, (,), INIT_MACRO())
};

#define ERROR_RATE(percentage) (100 / percentage)
#define TOLERANCE_ERROR_RATE ERROR_RATE(2)

void test_adc_normal_mode(const uint16_t *golden_value, float tolerance)
{
	int i, j;
	int retval;
	uint16_t m_sample_buffer[ASPEED_ADC_CH_NUMBER];
	int32_t val;
	const struct device *adc_dev;
	const struct adc_sequence sequence = {
		.channels = GENMASK(6, 0),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = 10,
		.calibrate = 1,
	};

	for (i = 0; i < ARRAY_SIZE(adc_list) - 1; i++) {
		adc_dev = device_get_binding(adc_list[i].device_label);
		for (j = 0; j < ASPEED_ADC_CH_NUMBER; j++) {
			adc_list[i].channel_config[j].channel_id = j;
			adc_list[i].channel_config[j].reference = ADC_REF_INTERNAL;
			adc_list[i].channel_config[j].gain = ADC_GAIN_1;
			adc_list[i].channel_config[j].acquisition_time = ADC_ACQ_TIME_DEFAULT;
			adc_channel_setup(adc_dev, &adc_list[i].channel_config[j]);
		}
		retval = adc_read(adc_dev, &sequence);
		if (retval >= 0) {
			for (j = 0; j < 7; j++) {
				val = m_sample_buffer[j];
				adc_raw_to_millivolts(adc_get_ref(adc_dev),
						      adc_list[i].channel_config[j].gain,
						      10,
						      &val);
				LOG_DBG("%s:[%d] %dmv(raw:%d)", adc_list[i].device_label, j, val,
					m_sample_buffer[j]);
				ast_zassert_within(val, golden_value[i * ASPEED_ADC_CH_NUMBER + j],
						   adc_get_ref(adc_dev) * tolerance,
						   "%s:[%d] %dmv(raw:%d) check %d+-%f failed!!",
						   adc_list[i].device_label, j, val,
						   m_sample_buffer[j],
						   golden_value[i * ASPEED_ADC_CH_NUMBER + j],
						   adc_get_ref(adc_dev) * tolerance);
			}
		}
	}
}

void test_adc_battery_mode(const uint16_t *golden_value, float tolerance)
{
	int i;
	int retval;
	uint16_t m_sample_buffer[1];
	int32_t val;
	const struct device *adc_dev;
	const struct adc_sequence sequence = {
		.channels = BIT(7),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = 10,
		.calibrate = 1,
	};

	for (i = 0; i < ARRAY_SIZE(adc_list) - 1; i++) {
		adc_dev = device_get_binding(adc_list[i].device_label);
		adc_list[i].channel_config[7].channel_id = 7;
		adc_list[i].channel_config[7].reference = ADC_REF_INTERNAL;
		adc_list[i].channel_config[7].gain =
			(adc_get_ref(adc_dev) < 1550) ? ADC_GAIN_1_3 : ADC_GAIN_2_3;
		adc_list[i].channel_config[7].acquisition_time = ADC_ACQ_TIME_DEFAULT;
		adc_channel_setup(adc_dev, &adc_list[i].channel_config[7]);
		retval = adc_read(adc_dev, &sequence);
		if (retval >= 0) {
			val = m_sample_buffer[0];
			adc_raw_to_millivolts(adc_get_ref(adc_dev),
					      adc_list[i].channel_config[7].gain, 10, &val);
			LOG_DBG("%s:[%d] %dmv(raw:%d)", adc_list[i].device_label, 7, val,
				m_sample_buffer[0]);
			ast_zassert_within(val, golden_value[i * ASPEED_ADC_CH_NUMBER + 7],
					   adc_get_ref(adc_dev) * tolerance,
					   "%s:[%d] %dmv(raw:%d) check %d+-%f failed!!",
					   adc_list[i].device_label, 7, val, m_sample_buffer[0],
					   golden_value[i * ASPEED_ADC_CH_NUMBER + 7],
					   adc_get_ref(adc_dev) * tolerance);
		}
	}
}

static const uint16_t ci_golden_value[16] = {
	620, 1202, 1800, 620, 1145, 1873, 1126, 3000,
	620, 1202, 1800, 620, 1145, 1873, 1126, 3000,
};

static const uint16_t ft_golden_value[16] = {
	600, 1650, 2000, 1000, 600, 1650, 2000, 1000,
	600, 1650, 2000, 1000, 600, 1650, 2000, 1000,
};

int test_adc(int count, enum aspeed_test_type type)
{
	int times;
	const uint16_t *gv;
	float tolerance;

	switch (type) {
	case AST_TEST_CI:
		gv = ci_golden_value;
		tolerance = 0.02;
		break;
	case AST_TEST_SLT:
	case AST_TEST_FT:
		gv = ft_golden_value;
		tolerance = 0.03;
		break;
	default:
		return -EINVAL;
	};

	printk("%s, count: %d, type: %d\n", __func__, count, type);
	for (times = 0; times < count; times++) {
		test_adc_normal_mode(gv, tolerance);
		test_adc_battery_mode(gv, tolerance);
	}

	return ast_ztest_result();
}
