# Copyright (c) 2021 ASPEED Technology Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_sources(soc.c)
zephyr_sources_ifdef(
  CONFIG_CPU_HAS_CUSTOM_FIXED_SOC_MPU_REGIONS
  mpu_regions.c
  )
zephyr_include_directories(${ZEPHYR_BASE}/soc/arm/common/cortex_m)
zephyr_linker_sources(ROM_START SORT_KEY 0x1sboot sboot.ld)
zephyr_linker_sources(RAM_SECTIONS nocache.ld)
