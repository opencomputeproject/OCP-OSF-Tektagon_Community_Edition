/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ast_test.h"

void aspeed_ft_pre_init(void)
{
	printk("%s\n\n", __func__);
}

void aspeed_ft_post_init(struct aspeed_tests *aspeed_testcase, int count)
{
	int i;

	printk("%s\n\n", __func__);
	printk("test module count: %d\n", count);

	for (i = 0; i < count; i++) {
		printk("module %d results: %s\n", i,
			aspeed_testcase[i].results ? "FAILED" : "PASS");
	}
}

