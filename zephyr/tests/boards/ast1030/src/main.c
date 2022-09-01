/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <debug/thread_analyzer.h>
#include "ast_test.h"

#define LOG_MODULE_NAME aspeed_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

extern void test_usb(void);
extern void test_adc(void);
extern void test_adc(void);
extern void test_pwm(void);
extern void test_jtag(void);
extern void test_i2c(void);
extern void test_i3c(void);
extern void test_gpio(void);
extern void test_uart(void);
extern void test_spi(void);
extern void test_espi(void);
extern void test_peci(void);
extern void test_crypto(void);
extern int aspeed_ft_pre_init(void);
extern int aspeed_ft_post_init(struct aspeed_tests *testcase, int count);

#define run_test_suite(suite, type) \
	aspeed_run_test_suite(#suite, _##suite, type)

#define TEST_MODULE_CNT		12
#define TEST_STACKSIZE		1024

#define TEST_CI_TIMEOUT		30
#define TEST_SLT_TIMEOUT	20
#define TEST_FT_TIMEOUT		5

#define TEST_CI_FUNC_COUNT	100
#define TEST_SLT_FUNC_COUNT	1
#define TEST_FT_FUNC_COUNT	1

K_THREAD_STACK_ARRAY_DEFINE(test_thread_stack, TEST_MODULE_CNT, TEST_STACKSIZE);

static struct aspeed_test_param test_params[] = {
	{ AST_TEST_CI, "AST_TEST_CI", TEST_CI_FUNC_COUNT, TEST_CI_TIMEOUT },
	{ AST_TEST_SLT, "AST_TEST_SLT", TEST_SLT_FUNC_COUNT, TEST_SLT_TIMEOUT },
	{ AST_TEST_FT, "AST_TEST_FT", TEST_FT_FUNC_COUNT, TEST_FT_TIMEOUT },
};

typedef int (*test_func_t) (int count, enum aspeed_test_type);

static struct aspeed_tests aspeed_testcase[TEST_MODULE_CNT];
static struct k_thread ztest_thread[TEST_MODULE_CNT];
static int unit_test_count;
static int unit_test_remain;

static void aspeed_pre_init(void)
{
#if CONFIG_BOARD_AST1030_FT
	aspeed_ft_pre_init();
#endif
}

static void aspeed_post_init(void)
{
#if CONFIG_BOARD_AST1030_FT
	aspeed_ft_post_init(aspeed_testcase, TEST_MODULE_CNT);
#endif
}

static void aspeed_test_cb(void *a, void *b, void *c)
{
	int num = (int)a;
	int count = (int)b;
	enum aspeed_test_type type = (enum aspeed_test_type)c;
	int ret = TC_FAIL;

	TC_START(aspeed_testcase[num].name);

	LOG_DBG("Current thread id: %d", (int)k_current_get());
	ret = aspeed_testcase[num].test(count, type);
	aspeed_testcase[num].results = ret;

	unit_test_remain--;
	Z_TC_END_RESULT(ret, aspeed_testcase[num].name);
}

static int aspeed_create_test(struct unit_test *test, int thread_num,
			      int count, enum aspeed_test_type type)
{
	int ret = TC_PASS;
	k_tid_t pid;

	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		pid = k_thread_create(&ztest_thread[thread_num],
				      test_thread_stack[thread_num],
				      TEST_STACKSIZE,
				      (k_thread_entry_t) aspeed_test_cb,
				      (void *)thread_num,
				      (void *)count, (void *)type,
				      CONFIG_ZTEST_THREAD_PRIORITY,
				      test->thread_options | K_INHERIT_PERMS,
				      K_FOREVER);

		if (test->name != NULL) {
			k_thread_name_set(&ztest_thread[thread_num],
					  test->name);
		}

	} else {
		LOG_ERR("multithreading is not supported");
		ret = TC_FAIL;
	}

	aspeed_testcase[thread_num].name = test->name;
	aspeed_testcase[thread_num].test = (test_func_t) test->test;
	aspeed_testcase[thread_num].results = -1;

	LOG_INF("Create thread %s\t to num %d, pid: %d",
			test->name, thread_num, (int)pid);

	return ret;
}

static void aspeed_run_test(void)
{
	int i;

	for (i = 0; i < unit_test_count; i++) {
		k_thread_start(&ztest_thread[i]);
	}
}

static void aspeed_show_banner(bool is_passed)
{
	if (is_passed) {
		LOG_ERR("\x1b[;32;1m\n"
				"########     ###     ######   ######  ######## ########\n"
				"##     ##   ## ##   ##    ## ##    ## ##       ##     ##\n"
				"##     ##  ##   ##  ##       ##       ##       ##     ##\n"
				"########  ##     ##  ######   ######  ######   ##     ##\n"
				"##        #########       ##       ## ##       ##     ##\n"
				"##        ##     ## ##    ## ##    ## ##       ##     ##\n"
				"##        ##     ##  ######   ######  ######## ########\n"
				"\x1b[0;m\n"
		      );
	} else {
		LOG_ERR("\x1b[;31;1m\n"
				"########    ###    #### ##       ######## ########\n"
				"##         ## ##    ##  ##       ##       ##     ##\n"
				"##        ##   ##   ##  ##       ##       ##     ##\n"
				"######   ##     ##  ##  ##       ######   ##     ##\n"
				"##       #########  ##  ##       ##       ##     ##\n"
				"##       ##     ##  ##  ##       ##       ##     ##\n"
				"##       ##     ## #### ######## ######## ########\n"
				"\x1b[0;m\n"
		      );
	}
}

static void aspeed_terminate_test(int type)
{
	bool is_passed = true;
	int i;

	LOG_INF("\nTest Counter: %d. Test Type: %s.",
		test_params[type].test_count,
		test_params[type].name);

	for (i = 0; i < unit_test_count; i++) {
		k_thread_abort(&ztest_thread[i]);
		if (aspeed_testcase[i].results)
			is_passed = false;

		LOG_INF("Case %d - %s results: %s", i + 1,
			aspeed_testcase[i].name,
			aspeed_testcase[i].results ? "FAILED" : "PASSED");
	}

	aspeed_show_banner(is_passed);
	zassert_equal(is_passed, true, "%s FAILED");
}

static void aspeed_run_test_suite(const char *name, struct unit_test *suite,
				  int type)
{
	int timer = 0;
	int fail = 0;

	aspeed_pre_init();

	while (suite->test) {
		fail += aspeed_create_test(suite, unit_test_count++,
				    test_params[type].test_count, type);
		suite++;
		if (unit_test_count >= TEST_MODULE_CNT) {
			LOG_ERR("unit test numbers are limited to %d\n",
					unit_test_count);
			break;
		}
	}

	if (fail)
		goto end;

	unit_test_remain = unit_test_count;
	thread_analyzer_print();

	aspeed_run_test();

	while (unit_test_remain && (timer < test_params[type].timeout)) {
		timer++;
		k_sleep(K_SECONDS(1));
	}

	if (unit_test_remain) {
		LOG_ERR("TIMEOUT ! Total test case: %d, remaining: %d",
			unit_test_count, unit_test_remain);
		fail++;
	}

end:
	aspeed_post_init();
	aspeed_terminate_test(type);
	zassert_equal(fail, 0, "test ast1030 failed");
}

static void aspeed_test_platform(void)
{
	ztest_test_suite(test_ast1030,
			 ztest_unit_test(test_usb),
			 ztest_unit_test(test_adc),
			 ztest_unit_test(test_pwm),
			 ztest_unit_test(test_jtag),
			 ztest_unit_test(test_i2c),
			 ztest_unit_test(test_i3c),
			 ztest_unit_test(test_gpio),
			 ztest_unit_test(test_uart),
			 ztest_unit_test(test_spi),
			 ztest_unit_test(test_espi),
			 ztest_unit_test(test_peci),
			 ztest_unit_test(test_crypto)
			 );

#ifdef CONFIG_BOARD_AST1030_EVB
	run_test_suite(test_ast1030, AST_TEST_CI);
#elif CONFIG_BOARD_AST1030_SLT
	run_test_suite(test_ast1030, AST_TEST_SLT);
#elif CONFIG_BOARD_AST1030_FT
	run_test_suite(test_ast1030, AST_TEST_FT);
#endif
}

/* test case main entry */
void test_main(void)
{
	ztest_test_suite(platform_test,
			 ztest_unit_test(aspeed_test_platform));

	ztest_run_test_suite(platform_test);
}
