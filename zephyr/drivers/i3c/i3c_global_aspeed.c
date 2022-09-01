/*
 * Copyright (c) ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_i3c_global

#include <soc.h>
#include <device.h>
#include <drivers/reset_control.h>
#include <sys/sys_io.h>
#include <logging/log.h>
#define LOG_LEVEL CONFIG_I3C_LOG_LEVEL
LOG_MODULE_REGISTER(i3c_global);

/* Device config */
struct i3c_global_config {
	uintptr_t base;
	const reset_control_subsys_t rst_id;
	const uint32_t ni3cs;
	const uint16_t *pull_up_resistor_tbl;
};
#define DEV_CFG(dev)			((const struct i3c_global_config *const)(dev)->config)

/* Registers */
#define I3C_GLOBAL_REG0(x)		(((x) * 0x10) + 0x10)
#define I3C_GLOBAL_REG1(x)		(((x) * 0x10) + 0x14)
#define DEFAULT_SLAVE_STATIC_ADDR	0x74
#define DEFAULT_SLAVE_INST_ID		0x4

union i3c_global_reg0_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t dg_sda_fmax : 4;		/* bit[3:0] */
		volatile uint32_t dg_sda_rmax : 4;		/* bit[7:4] */
		volatile uint32_t dg_scl_fmax : 4;		/* bit[11:8] */
		volatile uint32_t dg_scl_rmax : 4;		/* bit[15:12] */
		volatile uint32_t cdr_dly_fmax : 8;		/* bit[23:16] */
		volatile uint32_t cdr_en_dg : 1;		/* bit[24] */
		volatile uint32_t cdr_en_mask : 1;		/* bit[25] */
		volatile uint32_t reserved0 : 2;		/* bit[27:26] */
		volatile uint32_t sda_pullup_en_2k : 1;		/* bit[28] */
		volatile uint32_t sda_pullup_en_750 : 1;	/* bit[29] */
		volatile uint32_t reserved1 : 2;		/* bit[31:30] */
	} fields;
};

union i3c_global_reg1_s {
	volatile uint32_t value;
	struct {
		volatile uint32_t force_i2c_slave : 1;		/* bit[0] */
		volatile uint32_t slave_test_mode : 1;		/* bit[1] */
		volatile uint32_t act_mode : 2;			/* bit[3:2] */
		volatile uint32_t pending_int : 4;		/* bit[7:4] */
		volatile uint32_t slave_static_addr : 7;	/* bit[14:8] */
		volatile uint32_t slave_static_addr_en : 1;	/* bit[15] */
		volatile uint32_t slave_inst_id : 4;		/* bit[19:16] */
		volatile uint32_t reserved : 12;		/* bit[31:20] */
	} fields;
};

static int i3c_global_init(const struct device *dev)
{
	const struct device *reset_dev = device_get_binding(ASPEED_RST_CTRL_NAME);
	union i3c_global_reg0_s reg0;
	union i3c_global_reg1_s reg1;
	uint32_t base = DEV_CFG(dev)->base;
	uint32_t ni3cs = DEV_CFG(dev)->ni3cs;
	const uint16_t *pull_up_resistors = DEV_CFG(dev)->pull_up_resistor_tbl;
	int i;

	reset_control_deassert(reset_dev, DEV_CFG(dev)->rst_id);

	reg1.value = 0;
	reg1.fields.act_mode = 1;
	reg1.fields.slave_static_addr = DEFAULT_SLAVE_STATIC_ADDR;
	reg1.fields.slave_inst_id = DEFAULT_SLAVE_INST_ID;

	for (i = 0; i < ni3cs; i++) {
		reg0.value = sys_read32(base + I3C_GLOBAL_REG0(i));
		reg0.fields.sda_pullup_en_2k = 0;
		reg0.fields.sda_pullup_en_750 = 0;
		switch (pull_up_resistors[i]) {
		case 750:
			reg0.fields.sda_pullup_en_750 = 1;
			break;
		case 545:
			reg0.fields.sda_pullup_en_750 = 1;
			/* fallthrough */
		case 2000:
		default:
			reg0.fields.sda_pullup_en_2k = 1;
		}
		sys_write32(reg0.value, base + I3C_GLOBAL_REG0(i));
		sys_write32(reg1.value, base + I3C_GLOBAL_REG1(i));
	}

	return 0;
}

const uint16_t i3c_gr_pullup_resistor_tbl[] = DT_PROP(DT_NODELABEL(i3c_gr), pull_up_resistor_ohm);

static const struct i3c_global_config i3c_aspeed_config = {
	.base = DT_REG_ADDR(DT_NODELABEL(i3c_gr)),
	.rst_id = (reset_control_subsys_t)DT_INST_RESETS_CELL(0, rst_id),
	.ni3cs = DT_PROP(DT_NODELABEL(i3c_gr), ni3cs),
	.pull_up_resistor_tbl = &i3c_gr_pullup_resistor_tbl[0],
};


DEVICE_DT_INST_DEFINE(0, &i3c_global_init, NULL,
		      NULL, &i3c_aspeed_config,
		      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      NULL);
