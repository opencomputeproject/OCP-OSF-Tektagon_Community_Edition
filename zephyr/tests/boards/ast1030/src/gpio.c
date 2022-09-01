/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include "ast_test.h"
#include "test_gpio/test_gpio.h"

int test_gpio(int count, enum aspeed_test_type type)
{
	if (type != AST_TEST_CI)
		return AST_TEST_PASS;

	ast_test_fail |= test_gpio_port();
	ast_test_fail |= test_gpio_callback_add_remove();
	ast_test_fail |= test_gpio_callback_self_remove();
	ast_test_fail |= test_gpio_callback_enable_disable();
	ast_test_fail |= test_gpio_callback_variants();

	return ast_ztest_result();
}
