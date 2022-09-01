/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2021 ASPEED Technology Inc.
 */

#ifndef ZEPHYR_SOC_ARM_ASPEED_AST26XX_SOC_H_
#define ZEPHYR_SOC_ARM_ASPEED_AST26XX_SOC_H_
#include <aspeed_util.h>

#define __VTOR_PRESENT		1U
#define __NVIC_PRIO_BITS	NUM_IRQ_PRIO_BITS
#define __Vendor_SysTickConfig	0U
#define __FPU_PRESENT		0U
#define __MPU_PRESENT		0U

#define PHY_SRAM_ADDR		(*(volatile uint32_t *)(0x7e6e2a04))
#define TO_PHY_ADDR(addr)	(PHY_SRAM_ADDR + (uint32_t)(addr))
#define TO_VIR_ADDR(addr)	((uint32_t)(addr) - PHY_SRAM_ADDR)

#endif /* ZEPHYR_SOC_ARM_ASPEED_AST26XX_SOC_H_*/
