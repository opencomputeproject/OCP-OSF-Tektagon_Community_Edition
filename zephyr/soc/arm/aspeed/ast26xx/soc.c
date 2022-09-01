/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 ASPEED Technology Inc.
 */

#include <init.h>
#include <kernel.h>
#include <stdint.h>
#include <string.h>
#include <linker/linker-defs.h>
#include <cache.h>

extern char __bss_nc_start__[];
extern char __bss_nc_end__[];

/*WDT0 registers*/
#define WDT0_BASE 0x7e785000

#define WDT_SOFTWARE_RESET_MASK_REG 0x28

#define WDT_SOFTWARE_RESET_REG 0x24
#define WDT_TRIGGER_KEY 0xAEEDF123

/* SCU registers */
#define HW_STRAP_SET            0x500
#define HW_STRAP_CLR            0x504
#define CORTEX_A7_RESET         BIT(0)

/* secure boot header : provide image size to bootROM for SPI boot */
struct sb_header {
	uint32_t key_location;
	uint32_t enc_img_addr;
	uint32_t img_size;
	uint32_t sign_location;
	uint32_t header_rev[2];
	uint32_t patch_location;        /* address of the rom patch */
	uint32_t checksum;
};

struct sb_header sbh __attribute((used, section(".sboot"))) = {
	.img_size = (uint32_t)&_image_rom_end,
};

void z_platform_init(void)
{
	cache_instr_enable();

#ifdef CONFIG_BOARD_AST2605_EVB
	uint32_t base = DT_REG_ADDR(DT_NODELABEL(syscon));

	/* de-assert Cortex-A7 Primary service processor reset */
	sys_write32(CORTEX_A7_RESET, base + HW_STRAP_CLR);
#endif

	if (CONFIG_SRAM_NC_SIZE > 0) {
		(void)memset(__bss_nc_start__, 0, __bss_nc_end__ - __bss_nc_start__);
	}
}

void sys_arch_reboot(int type)
{
	sys_write32(0x3fffff1, WDT0_BASE + WDT_SOFTWARE_RESET_MASK_REG);
	sys_write32(WDT_TRIGGER_KEY, WDT0_BASE + WDT_SOFTWARE_RESET_REG);
	ARG_UNUSED(type);
}
