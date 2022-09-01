/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_PFR_ASPEED_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_PFR_ASPEED_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

/* general command */
#define CMD_RDID            0x9F
#define CMD_WREN            0x06
#define CMD_WRDIS           0x04
#define CMD_RDSR            0x05
#define CMD_RDCR            0x15
#define CMD_RDSR2           0x35
#define CMD_WRSR            0x01
#define CMD_WRSR2           0x31
#define CMD_SFDP            0x5A
#define CMD_EN4B            0xB7
#define CMD_EX4B            0xE9

/* read commands */
#define CMD_READ_1_1_1_3B   0x03
#define CMD_READ_1_1_1_4B   0x13
#define CMD_FREAD_1_1_1_3B  0x0B
#define CMD_FREAD_1_1_1_4B  0x0C
#define CMD_READ_1_1_2_3B   0x3B
#define CMD_READ_1_1_2_4B   0x3C
#define CMD_READ_1_1_4_3B   0x6B
#define CMD_READ_1_1_4_4B   0x6C

/* write command */
#define CMD_PP_1_1_1_3B     0x02
#define CMD_PP_1_1_1_4B     0x12
#define CMD_PP_1_1_4_3B     0x32
#define CMD_PP_1_1_4_4B     0x34
#define CMD_PP_1_4_4_3B     0x38
#define CMD_PP_1_4_4_4B     0x3E

/* sector erase command */
#define CMD_SE_1_1_0_3B     0x20
#define CMD_SE_1_1_0_4B     0x21
#define CMD_SE_1_1_0_64_3B  0xD8
#define CMD_SE_1_1_0_64_4B  0xDC

/* define the total ram size used to record exception log */
#define SPIM_LOG_RAM_TOTAL_SIZE 2048

struct cmd_table_info {
	uint8_t cmd;
	uint8_t reserved[3];
	uint32_t cmd_table_val;
};

enum spim_spi_master {
	SPI_NONE,
	SPI1,
	SPI2
};

enum spim_passthrough_mode {
	SPIM_SINGLE_PASSTHROUGH,
	SPIM_MULTI_PASSTHROUGH,
};

enum spim_ext_mux_mode {
	SPIM_MASTER_MODE,
	SPIM_MONITOR_MODE,
};

enum spim_block_mode {
	SPIM_DEASSERT_CS_EARLY,
	SPIM_BLOCK_EXTRA_CLK
};

void pfr_bmc_rst_enable_ctrl(bool enable);
void spim_rst_flash(const struct device *dev, uint32_t rst_duration_ms);
void spim_scu_ctrl_set(const struct device *dev, uint32_t mask, uint32_t val);
void spim_scu_ctrl_clear(const struct device *dev, uint32_t clear_bits);
void spim_ext_mux_config(const struct device *dev,
	enum spim_ext_mux_mode mode);
void spim_passthrough_config(const struct device *dev,
	enum spim_passthrough_mode mode, bool passthrough_en);
void spim_spi_ctrl_detour_enable(const struct device *dev,
	enum spim_spi_master spi, bool enable);
void spim_block_mode_config(const struct device *dev, enum spim_block_mode mode);


/* allow command table control */
#define FLAG_CMD_TABLE_VALID         0x00000000
#define FLAG_CMD_TABLE_VALID_ONCE    0x00000001
#define FLAG_CMD_TABLE_LOCK_ALL      0x00000002

/* address privilege table control */
enum addr_priv_rw_select {
	FLAG_ADDR_PRIV_READ_SELECT,
	FLAG_ADDR_PRIV_WRITE_SELECT
};

enum addr_priv_op {
	FLAG_ADDR_PRIV_ENABLE,
	FLAG_ADDR_PRIV_DISABLE
};

void spim_dump_allow_command_table(const struct device *dev);
int spim_add_allow_command(const struct device *dev, uint8_t cmd, uint32_t flag);
int spim_remove_allow_command(const struct device *dev, uint8_t cmd);
int spim_lock_allow_command_table(const struct device *dev, uint8_t cmd, uint32_t flag);
void spim_dump_rw_addr_privilege_table(const struct device *dev);
int spim_address_privilege_config(const struct device *dev,
	enum addr_priv_rw_select rw_select, enum addr_priv_op priv_op,
	mm_reg_t addr, uint32_t len);

void spim_lock_rw_privilege_table(const struct device *dev,
	enum addr_priv_rw_select rw_select);
void spim_lock_common(const struct device *dev);

void spim_monitor_enable(const struct device *dev, bool enable);
void aspeed_spi_monitor_sw_rst(const struct device *dev);

struct spim_log_info {
	mem_addr_t log_ram_addr;
	uint32_t log_max_sz;
	uint32_t log_idx_reg;
};

typedef void (*spim_isr_callback_t)(const struct device *dev);
void spim_isr_callback_install(const struct device *dev,
	spim_isr_callback_t isr_callback);
void spim_get_log_info(const struct device *dev, struct spim_log_info *info);
uint32_t spim_get_ctrl_idx(const struct device *dev);

bool get_wdt_timeout_status(const struct device *dev);

#endif
