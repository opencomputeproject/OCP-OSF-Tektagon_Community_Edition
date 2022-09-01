/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/hwinfo.h>
#include <string.h>

/* SoC mapping Table */

#define ASPEED_REVISION_ID0     0x04
#define ASPEED_REVISION_ID1     0x14

#define SOC_ID(str, rev) { .name = str, .rev_id = rev, }

struct soc_id {
	const char *name;
	uint64_t rev_id;
};

static struct soc_id soc_map_table[] = {
	SOC_ID("AST1030-A0", 0x8000000080000000),
	SOC_ID("AST1030-A1", 0x8001000080010000),
	SOC_ID("AST2600-A0", 0x0500030305000303),
	SOC_ID("AST2600-A1", 0x0501030305010303),
	SOC_ID("AST2620-A1", 0x0501020305010203),
	SOC_ID("AST2600-A2", 0x0502030305010303),
	SOC_ID("AST2620-A2", 0x0502020305010203),
	SOC_ID("AST2605-A2", 0x0502010305010103),
	SOC_ID("AST2600-A3", 0x0503030305030303),
	SOC_ID("AST2620-A3", 0x0503020305030203),
	SOC_ID("AST2605-A3", 0x0503010305030103),
	SOC_ID("Unknown",    0x0000000000000000),
};

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	int i;
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));
	uint64_t rev_id;

	rev_id = sys_read32(base + ASPEED_REVISION_ID0);
	rev_id = ((uint64_t)sys_read32(base + ASPEED_REVISION_ID1) << 32) |
		 rev_id;

	for (i = 0; i < ARRAY_SIZE(soc_map_table); i++) {
		if (rev_id == soc_map_table[i].rev_id) {
			break;
		}
	}

	if (length > sizeof(rev_id)) {
		length = sizeof(rev_id);
	}

	for (i = 0; i < length; i++) {
		int sft = (length - i - 1) * 8;

		buffer[i] = (uint8_t)((rev_id >> sft) & 0xff);
	}

	return length;
}
