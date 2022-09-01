/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <tc_util.h>
#include <random/rand32.h>
#include <drivers/sensor.h>
#include <drivers/pwm.h>
#include "ast_test.h"
#define LOG_MODULE_NAME pwm_tach_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#if DT_HAS_COMPAT_STATUS_OKAY(aspeed_pwm)
#define DT_DRV_COMPAT aspeed_pwm
#else
#error No known devicetree compatible match for pwm/tach test
#endif

#define NODE_LABELS(n) { .device_label = DT_INST_LABEL(n), .npwms = DT_INST_PROP(n, npwms) },
#define INIT_MACRO() DT_INST_FOREACH_STATUS_OKAY(NODE_LABELS)

static struct pwm_hdl {
	char *device_label;
	int npwms;
} pwm_list[] = {
	INIT_MACRO()
};

#undef DT_DRV_COMPAT
#if DT_HAS_COMPAT_STATUS_OKAY(aspeed_tach)
#define DT_DRV_COMPAT aspeed_tach
#else
#error No known devicetree compatible match for pwm/tach test
#endif

#define FAN_LABELS(node_id) { .device_label = DT_LABEL(node_id) },

#define FAN_NODE_LABELS(n) DT_FOREACH_CHILD(DT_DRV_INST(n), FAN_LABELS)

#define FAN_INIT_MACRO() DT_INST_FOREACH_STATUS_OKAY(FAN_NODE_LABELS)
static struct fan_hdl {
	char *device_label;
} fan_list[] = {
	FAN_INIT_MACRO()
};

void test_pwm_tach_enable(void)
{
	printk("Do nothing\n");
}

#if CONFIG_PWM_ASPEED_ACCURATE_FREQ
void test_pwm_tach_loopback_accurate(void)
{
	int i, hw_pwm;
	const struct device *pwm_dev, *tach_dev;
	uint64_t cycles_per_sec, cycles_per_min;
	uint32_t expected_rpm, rand_cycles, div_l, div_h;
	struct sensor_value sensor_value;
	int err;

	for (i = 0; i < ARRAY_SIZE(pwm_list); i++) {
		pwm_dev = device_get_binding(pwm_list[i].device_label);
		for (hw_pwm = 0; hw_pwm < pwm_list[i].npwms - 1; hw_pwm++) {
			pwm_get_cycles_per_sec(pwm_dev, hw_pwm, &cycles_per_sec);
			cycles_per_min = cycles_per_sec * 60;
			tach_dev = device_get_binding(fan_list[hw_pwm].device_label);

			div_l = sys_rand32_get() % 256;
			div_h = sys_rand32_get() % 4;
			rand_cycles = BIT(div_h) * (div_l + 1);
			if (rand_cycles < 2) {
				rand_cycles = 2;
			}
			pwm_pin_set_cycles(pwm_dev, hw_pwm, rand_cycles, rand_cycles / 2, 0);
			/* Needs one cylce time for period transtion */
			k_usleep(rand_cycles * USEC_PER_SEC / cycles_per_sec);
			expected_rpm = cycles_per_min / rand_cycles;
			err = sensor_sample_fetch(tach_dev);
			ast_zassert_ok(err, "Failed to read sensor %s\n",
				       fan_list[hw_pwm].device_label);
			sensor_channel_get(tach_dev, SENSOR_CHAN_RPM, &sensor_value);
			LOG_DBG("%d value = %10.6f expected = %d rand_cycles = %d", hw_pwm,
				sensor_value_to_double(&sensor_value), expected_rpm, rand_cycles);
			ast_zassert_equal(sensor_value.val1, expected_rpm, "PWM%d(%d == %d)",
					  hw_pwm, sensor_value.val1, expected_rpm);
		}
	}
}
#else
void test_pwm_tach_loopback_rough(void)
{
	int i, hw_pwm;
	const struct device *pwm_dev, *tach_dev;
	uint64_t cycles_per_sec, cycles_per_min;
	uint32_t expected_rpm, rand_cycles;
	struct sensor_value sensor_value;
	int err;

	for (i = 0; i < ARRAY_SIZE(pwm_list); i++) {
		pwm_dev = device_get_binding(pwm_list[i].device_label);
		for (hw_pwm = 0; hw_pwm < pwm_list[i].npwms - 1; hw_pwm++) {
			pwm_get_cycles_per_sec(pwm_dev, hw_pwm, &cycles_per_sec);
			cycles_per_min = cycles_per_sec * 60;
			tach_dev = device_get_binding(fan_list[hw_pwm].device_label);

			rand_cycles = (sys_rand32_get() % 2000) + 2;
			pwm_pin_set_cycles(pwm_dev, hw_pwm, rand_cycles, rand_cycles / 2, 0);
			/* Needs one cylce time for period transtion */
			k_usleep(rand_cycles * USEC_PER_SEC / cycles_per_sec);
			expected_rpm = cycles_per_min / rand_cycles;
			err = sensor_sample_fetch(tach_dev);
			ast_zassert_ok(err, "Failed to read sensor %s\n",
				       fan_list[hw_pwm].device_label);
			sensor_channel_get(tach_dev, SENSOR_CHAN_RPM, &sensor_value);
			LOG_DBG("%d value = %10.6f expected = %d rand_cycles = %d", hw_pwm,
				sensor_value_to_double(&sensor_value), expected_rpm, rand_cycles);
			ast_zassert_true(sensor_value.val1 >= expected_rpm, "PWM%d(%d < %d)",
					 hw_pwm, sensor_value.val1, expected_rpm);
		}
	}
}
#endif

#define AFB0812SH_FAN_ERROR 450
static const uint32_t AFB0812SH_FAN_RPM[5] = {
	4500,   /* 100% */
	3400,   /* 75% */
	2250,   /* 50% */
	1100,   /* 25% */
	700,    /* 0~17% */
};

static const uint32_t AFB0812SH_FAN_DUTY[5] = {
	100, 75, 50, 25, 0,
};

void test_pwm_tach_fan(void)
{
	int i, j, hw_pwm;
	const struct device *pwm_dev, *tach_dev;
	uint32_t period_ns = 40000;
	struct sensor_value sensor_value;
	int err;

	for (i = 0; i < ARRAY_SIZE(pwm_list); i++) {
		pwm_dev = device_get_binding(pwm_list[i].device_label);
		hw_pwm = pwm_list[i].npwms - 1;
		tach_dev = device_get_binding(fan_list[hw_pwm].device_label);
		for (j = 0; j < ARRAY_SIZE(AFB0812SH_FAN_RPM); j += 1) {
			pwm_pin_set_nsec(pwm_dev, hw_pwm, period_ns,
					 period_ns / 100 * AFB0812SH_FAN_DUTY[j], 0);
			/* Wait Fan stable */
			k_msleep(3000);
			err = sensor_sample_fetch(tach_dev);
			ast_zassert_ok(err, "Failed to read sensor %s\n",
				       fan_list[hw_pwm].device_label);
			sensor_channel_get(tach_dev, SENSOR_CHAN_RPM, &sensor_value);
			LOG_DBG("duty: %d%% rpm = %d", AFB0812SH_FAN_DUTY[j], sensor_value.val1);
			ast_zassert_within(sensor_value.val1, AFB0812SH_FAN_RPM[j],
					   AFB0812SH_FAN_ERROR, "%d != %d+-%d", sensor_value.val1,
					   AFB0812SH_FAN_RPM[j], AFB0812SH_FAN_ERROR);
		}
	}
}

int test_pwm(int count, enum aspeed_test_type type)
{
	int times;

	printk("%s, count: %d, type: %d\n", __func__, count, type);
	if (type != AST_TEST_CI)
		return AST_TEST_PASS;

	for (times = 0; times < count; times++) {
#if CONFIG_PWM_ASPEED_ACCURATE_FREQ
		test_pwm_tach_loopback_accurate();
#else
		test_pwm_tach_loopback_rough();
#endif
	}
	test_pwm_tach_fan();

	return ast_ztest_result();
}
