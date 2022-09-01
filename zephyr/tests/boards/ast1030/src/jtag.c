/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include "ast_test.h"

int test_jtag(int count, enum aspeed_test_type type)
{
	printk("%s, count: %d, type: %d\n", __func__, count, type);

	return ast_ztest_result();
}
