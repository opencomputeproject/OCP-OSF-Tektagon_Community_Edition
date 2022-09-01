/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <tc_util.h>
#include "ast_test.h"

static uint32_t espi_base;

int test_espi_ci(void)
{
	return 0;
}

int test_espi_ft(void)
{
	uint32_t reg;

	reg = sys_read32(espi_base + 0x80) | BIT(29);
	sys_write32(reg, espi_base + 0x80);

	return 0;
}

int test_espi_slt(void)
{
	return 0;
}

int test_espi(int count, enum aspeed_test_type type)
{
	int i, rc;
	int (*test_espi_func)(void);

	printk("%s, count: %d, type: %d\n", __func__, count, type);

	espi_base = DT_REG_ADDR(DT_NODELABEL(espi));
	if (!espi_base)
		return -ENODEV;

	switch (type) {
	case AST_TEST_CI:
		test_espi_func = test_espi_ci;
		break;
	case AST_TEST_SLT:
		test_espi_func = test_espi_slt;
		break;
	case AST_TEST_FT:
		test_espi_func = test_espi_ft;
		break;
	default:
		return -EINVAL;
	};

	for (i = 0; i < count; ++i) {
		rc = test_espi_func();
		if (rc)
			break;
	}

	return rc;
}
