/*
 * Copyright (c) 2021 Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_spi_monitor_controller

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <drivers/clock_control.h>
#include <errno.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(spim_aspeed, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>
#include <soc.h>
#include <drivers/spi_nor.h>

#define CMD_TABLE_VALUE(G, W, R, M, DAT_MODE, DUMMY, PROG_SZ, ADDR_LEN, ADDR_MODE, CMD) \
	(G << 29 | W << 28 | R << 27 | M << 26 | DAT_MODE << 24 | DUMMY << 16 | PROG_SZ << 13 | \
	ADDR_LEN << 10 | ADDR_MODE << 8 | CMD)

struct cmd_table_info cmds_array[] = {
	{.cmd = CMD_READ_1_1_1_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 1, 0, 0, 3, 1, CMD_READ_1_1_1_3B)},
	{.cmd = CMD_READ_1_1_1_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 1, 0, 0, 4, 1, CMD_READ_1_1_1_4B)},
	{.cmd = CMD_FREAD_1_1_1_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 1, 8, 0, 3, 1, CMD_FREAD_1_1_1_3B)},
	{.cmd = CMD_FREAD_1_1_1_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 1, 8, 0, 4, 1, CMD_FREAD_1_1_1_4B)},
	{.cmd = CMD_READ_1_1_2_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 2, 8, 0, 3, 1, CMD_READ_1_1_2_3B)},
	{.cmd = CMD_READ_1_1_2_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 2, 8, 0, 4, 1, CMD_READ_1_1_2_4B)},
	{.cmd = CMD_READ_1_1_4_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 3, 8, 0, 3, 1, CMD_READ_1_1_4_3B)},
	{.cmd = CMD_READ_1_1_4_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 1, 3, 8, 0, 4, 1, CMD_READ_1_1_4_4B)},
	{.cmd = CMD_PP_1_1_1_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 1, 0, 1, 3, 1, CMD_PP_1_1_1_3B)},
	{.cmd = CMD_PP_1_1_1_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 1, 0, 1, 4, 1, CMD_PP_1_1_1_4B)},
	{.cmd = CMD_PP_1_1_4_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 3, 0, 1, 3, 1, CMD_PP_1_1_4_3B)},
	{.cmd = CMD_PP_1_1_4_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 3, 0, 1, 4, 1, CMD_PP_1_1_4_4B)},
	{.cmd = CMD_SE_1_1_0_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 0, 0, 1, 3, 1, CMD_SE_1_1_0_3B)},
	{.cmd = CMD_SE_1_1_0_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 0, 0, 1, 4, 1, CMD_SE_1_1_0_4B)},
	{.cmd = CMD_SE_1_1_0_64_3B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 0, 0, 5, 3, 1, CMD_SE_1_1_0_64_3B)},
	{.cmd = CMD_SE_1_1_0_64_4B,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 1, 0, 0, 5, 4, 1, CMD_SE_1_1_0_64_4B)},
	{.cmd = CMD_WREN,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 0, 0, 0, 0, 0, 0, 0, CMD_WREN)},
	{.cmd = CMD_WRDIS,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 0, 0, 0, 0, 0, 0, 0, CMD_WRDIS)},
	{.cmd = CMD_RDSR,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 0, 1, 0, 0, 0, 0, CMD_RDSR)},
	{.cmd = CMD_RDSR2,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 0, 1, 0, 0, 0, 0, CMD_RDSR2)},
	{.cmd = CMD_WRSR,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 0, 1, 0, 0, 0, 0, CMD_WRSR)},
	{.cmd = CMD_WRSR2,
		.cmd_table_val = CMD_TABLE_VALUE(1, 1, 0, 0, 1, 0, 0, 0, 0, CMD_WRSR2)},
	{.cmd = CMD_RDCR,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 0, 1, 0, 0, 0, 0, CMD_RDCR)},
	{.cmd = CMD_EN4B,
		.cmd_table_val = CMD_TABLE_VALUE(0, 0, 0, 0, 0, 0, 0, 0, 0, CMD_EN4B)},
	{.cmd = CMD_EX4B,
		.cmd_table_val = CMD_TABLE_VALUE(0, 0, 0, 0, 0, 0, 0, 0, 0, CMD_EX4B)},
	{.cmd = CMD_SFDP,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 0, 1, 8, 0, 3, 1, CMD_SFDP)},
	{.cmd = CMD_RDID,
		.cmd_table_val = CMD_TABLE_VALUE(1, 0, 1, 0, 1, 0, 0, 0, 0, CMD_RDID)},
};

static uint8_t spim_log_arr[SPIM_LOG_RAM_TOTAL_SIZE] NON_CACHED_BSS_ALIGN16;

/* control register */
#define SPIM_CTRL               (0x0000)
#define SPIM_IRQ_CTRL           (0x0004)
#define SPIM_EAR                (0x0008)
#define SPIM_FIFO               (0x000C)
#define SPIM_LOG_BASE           (0x0010)
#define SPIM_LOG_SZ             (0x0014)
#define SPIM_LOG_PTR            (0x0018)
#define SPIM_LOCK_REG           (0x007C)
#define SPIM_ALLOW_CMD_BASE     (0x0080)
#define SPIM_ADDR_PRIV_TABLE_BASE    (0x0100)

#define SPIM_BLOCK_CMD_EXTRA_CLK    BIT(7)
#define SPIM_SW_RST                 BIT(15)

/* allow command table */
#define SPIM_CMD_TABLE_NUM              32
#define SPIM_CMD_TABLE_VALID_MASK       GENMASK(31, 30)
#define SPIM_CMD_TABLE_VALID_ONCE_BIT   BIT(31)
#define SPIM_CMD_TABLE_VALID_BIT        BIT(30)
#define SPIM_CMD_TABLE_IS_GENERIC_CMD   BIT(29)
#define SPIM_CMD_TABLE_IS_WRITE_CMD     BIT(28)
#define SPIM_CMD_TABLE_IS_READ_CMD      BIT(27)
#define SPIM_CMD_TABLE_IS_MEM_CMD       BIT(26)
#define SPIM_CMD_TABLE_DATA_MODE_MASK   GENMASK(25, 24)
#define SPIM_CMD_TABLE_LOCK_BIT         BIT(23)
#define SPIM_CMD_TABLE_DUMMY_MASK       GENMASK(21, 16)
#define SPIM_CMD_TABLE_PROGRAM_SZ_MASK  GENMASK(15, 13)
#define SPIM_CMD_TABLE_ADDR_LEN_MASK    GENMASK(12, 10)
#define SPIM_CMD_TABLE_ADDR_MODE_MASK   GENMASK(9, 8)
#define SPIM_CMD_TABLE_CMD_MASK         GENMASK(7, 0)

/* allow address region configuration */
#define SPIM_PRIV_WRITE_SELECT   0x57000000
#define SPIM_PRIV_READ_SELECT    0x52000000
#define SPIM_ADDR_PRIV_REG_NUN   512
#define SPIM_ADDR_PRIV_BIT_NUN   (SPIM_ADDR_PRIV_REG_NUN * 32)

/* lock register */
/* SPIPF00 */
#define SPIM_BLOCK_FIFO_CTRL_LOCK       BIT(22)
#define SPIM_SW_RST_CTRL_LOCK           BIT(23)

/* SPIPF7C */
#define SPIM_CTRL_REG_LOCK              BIT(0)
#define SPIM_INT_STS_REG_LOCK           BIT(1)
#define SPIM_LOG_BASE_ADDR_REG_LOCK     BIT(4)
#define SPIM_LOG_CTRL_REG_LOCK          BIT(5)
#define SPIM_ADDR_PRIV_WRITE_TABLE_LOCK BIT(30)
#define SPIM_ADDR_PRIV_READ_TABLE_LOCK  BIT(31)

/* irq related bit */
#define SPIM_CMD_BLOCK_IRQ              BIT(0)
#define SPIM_WRITE_BLOCK_IRQ            BIT(1)
#define SPIM_READ_BLOCK_IRQ             BIT(2)
#define SPIM_IRQ_STS_MASK               GENMASK(2, 0)
#define SPIM_CMD_BLOCK_IRQ_EN           BIT(16)
#define SPIM_WRITE_BLOCK_IRQ_EN         BIT(17)
#define SPIM_READ_BLOCK_IRQ_EN          BIT(18)

/* spi monitor log control */
#define SPIM_BLOCK_INFO_EN              BIT(31)

/* PFR related control */
#define SPIM_MODE_SCU_CTRL              (0x00f0)

struct aspeed_spim_data {
	const struct device *dev;
	struct k_sem sem_spim; /* protect most control registers */
	struct k_spinlock irq_ctrl_lock; /* protect ISR content */
	uint8_t allow_cmd_list[SPIM_CMD_TABLE_NUM];
	uint32_t allow_cmd_num;
	uint32_t read_forbidden_regions[32];
	uint32_t read_forbidden_region_num;
	uint32_t write_forbidden_regions[32];
	uint32_t write_forbidden_region_num;
	struct k_work log_work;
	struct spim_log_info log_info;
	spim_isr_callback_t isr_callback;
};

struct aspeed_spim_config {
	mm_reg_t ctrl_base;
	uint32_t irq_num;
	uint32_t irq_priority;
	uint32_t ctrl_idx;
	bool extra_clk_en;
	const struct device *parent;
	const struct device *flash_dev;
};

struct aspeed_spim_common_config {
	mm_reg_t scu_base;
};

struct aspeed_spim_common_data {
	struct k_spinlock scu_lock; /* protect SCU0F0 */

	struct k_sem sem_log_op; /* protect log info */
	uint32_t cur_log_sz;
};

struct priv_reg_info {
	uint32_t start_reg_off;
	uint32_t start_bit_off;
	uint32_t end_reg_off;
	uint32_t end_bit_off;
};

static void acquire_spim_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct aspeed_spim_data *const data = dev->data;

		k_sem_take(&data->sem_spim, K_FOREVER);
	}
}

static void release_spim_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct aspeed_spim_data *const data = dev->data;

		k_sem_give(&data->sem_spim);
	}
}

static void acquire_log_op(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct aspeed_spim_common_data *const data = dev->data;

		k_sem_take(&data->sem_log_op, K_FOREVER);
	}
}

static void release_log_op(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct aspeed_spim_common_data *const data = dev->data;

		k_sem_give(&data->sem_log_op);
	}
}

void spim_scu_ctrl_set(const struct device *dev, uint32_t mask, uint32_t val)
{
	const struct aspeed_spim_common_config *config = dev->config;
	struct aspeed_spim_common_data *const data = dev->data;
	mm_reg_t spim_scu_ctrl = config->scu_base + SPIM_MODE_SCU_CTRL;
	uint32_t reg_val;
	/* Avoid SCU0F0 being accessed by more than a thread */
	k_spinlock_key_t key = k_spin_lock(&data->scu_lock);

	reg_val = sys_read32(spim_scu_ctrl);
	reg_val &= ~(mask);
	reg_val |= val;
	sys_write32(reg_val, spim_scu_ctrl);

	k_spin_unlock(&data->scu_lock, key);
}

void spim_scu_ctrl_clear(const struct device *dev, uint32_t clear_bits)
{
	const struct aspeed_spim_common_config *config = dev->config;
	struct aspeed_spim_common_data *const data = dev->data;
	mm_reg_t spim_scu_ctrl = config->scu_base + SPIM_MODE_SCU_CTRL;
	uint32_t reg_val;
	k_spinlock_key_t key = k_spin_lock(&data->scu_lock);

	reg_val = sys_read32(spim_scu_ctrl);
	reg_val &= ~(clear_bits);
	sys_write32(reg_val, spim_scu_ctrl);

	k_spin_unlock(&data->scu_lock, key);
}

void spim_scu_passthrough_mode(const struct device *dev,
	enum spim_passthrough_mode mode, bool passthrough_en)
{
	const struct aspeed_spim_config *config = dev->config;

	if (passthrough_en) {
		spim_scu_ctrl_set(config->parent, BIT(config->ctrl_idx - 1) << 4,
			BIT(config->ctrl_idx - 1) << 4);
	} else {
		spim_scu_ctrl_clear(config->parent, BIT(config->ctrl_idx - 1) << 4);
	}

	ARG_UNUSED(mode);
}

void spim_spi_ctrl_detour_enable(const struct device *dev,
	enum spim_spi_master spi, bool enable)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t spim_idx = config->ctrl_idx;

	if (spi == SPI_NONE || spi > SPI2)
		return;

	if (enable)
		spim_scu_ctrl_set(config->parent, 0xF, ((spi - 1) << 3) | spim_idx);
	else
		spim_scu_ctrl_clear(config->parent, 0xF);
}

void spim_passthrough_config(const struct device *dev,
	enum spim_passthrough_mode mode, bool passthrough_en)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t ctrl_reg_val;

	acquire_spim_device(dev);

	ctrl_reg_val = sys_read32(config->ctrl_base);

	ctrl_reg_val &= ~0x00000003;
	if (passthrough_en) {
		if (mode == SPIM_MULTI_PASSTHROUGH)
			ctrl_reg_val |= BIT(1);
		else
			ctrl_reg_val |= BIT(0);
	}

	sys_write32(ctrl_reg_val, config->ctrl_base);

	release_spim_device(dev);
}

void spim_ext_mux_config(const struct device *dev,
	enum spim_ext_mux_mode mode)
{
	const struct aspeed_spim_config *config = dev->config;

	if (mode == SPIM_MONITOR_MODE) {
		spim_scu_ctrl_set(config->parent, BIT(config->ctrl_idx - 1) << 12,
					BIT(config->ctrl_idx - 1) << 12);
	} else {
		spim_scu_ctrl_clear(config->parent, BIT(config->ctrl_idx - 1) << 12);
	}
}

void spim_block_mode_config(const struct device *dev, enum spim_block_mode mode)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;

	acquire_spim_device(dev);

	reg_val = sys_read32(config->ctrl_base);

	if (mode == SPIM_BLOCK_EXTRA_CLK)
		reg_val |= SPIM_BLOCK_CMD_EXTRA_CLK;
	else
		reg_val &= ~(SPIM_BLOCK_CMD_EXTRA_CLK);

	sys_write32(reg_val, config->ctrl_base);

	release_spim_device(dev);
}

/* dump command information recored in allow command table */
void spim_dump_allow_command_table(const struct device *dev)
{
	uint32_t i;
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;
	uint32_t addr_len, addr_mode;
	uint32_t dummy_cyc;
	uint32_t data_mode;
	uint8_t cmd;
	uint32_t prog_sz;

	acquire_spim_device(dev);

	for (i = 0; i < SPIM_CMD_TABLE_NUM; i++) {
		reg_val = sys_read32(config->ctrl_base + SPIM_ALLOW_CMD_BASE + i * 4);
		if (reg_val == 0)
			continue;
		printk("[%s]idx %02d: 0x%08x\n", dev->name, i, reg_val);
	}

	printk("\ncmd info:\n");
	for (i = 0; i < SPIM_CMD_TABLE_NUM; i++) {
		reg_val = sys_read32(config->ctrl_base + SPIM_ALLOW_CMD_BASE + i * 4);
		if (reg_val == 0)
			continue;

		cmd = reg_val & SPIM_CMD_TABLE_CMD_MASK;
		addr_mode = (reg_val & SPIM_CMD_TABLE_ADDR_MODE_MASK) >> 8;
		addr_len = (reg_val & SPIM_CMD_TABLE_ADDR_LEN_MASK) >> 10;
		dummy_cyc = (reg_val & SPIM_CMD_TABLE_DUMMY_MASK) >> 16;
		data_mode = (reg_val & SPIM_CMD_TABLE_DATA_MODE_MASK) >> 24;
		prog_sz = (reg_val & SPIM_CMD_TABLE_PROGRAM_SZ_MASK) >> 13;

		printk("cmd: %02x, addr: len(%d)/", cmd, addr_len);
		switch (addr_mode) {
		case 1:
			printk("mode(single),");
			break;
		case 2:
			printk("mode(dual)  ,");
			break;
		case 3:
			printk("mode(quad)  ,");
			break;
		default:
			printk("mode(no)    ,");
		}

		printk(" dummy: %d, data_mode:", dummy_cyc);

		switch (data_mode) {
		case 1:
			printk(" single,");
			break;
		case 2:
			printk(" dual  ,");
			break;
		case 3:
			printk(" quad  ,");
			break;
		default:
			printk(" no    ,");
		}

		printk(" prog_sz: %03ldKB", prog_sz == 0 ? 0 : BIT(prog_sz + 1));
		(reg_val & SPIM_CMD_TABLE_IS_MEM_CMD) != 0 ?
			printk(", mem_op") : printk(",%*c", 7, ' ');
		(reg_val & SPIM_CMD_TABLE_IS_READ_CMD) != 0 ?
			printk(", read") : printk(",%*c", 5, ' ');
		(reg_val & SPIM_CMD_TABLE_IS_WRITE_CMD) != 0 ?
			printk(", write") : printk(",%*c", 6, ' ');
		(reg_val & SPIM_CMD_TABLE_IS_GENERIC_CMD) != 0 ?
			printk(", generic") : printk(",%*c", 8, ' ');
		(reg_val & SPIM_CMD_TABLE_VALID_BIT) != 0 ?
			printk(", valid") : printk(",%*c", 6, ' ');
		(reg_val & SPIM_CMD_TABLE_VALID_ONCE_BIT) != 0 ?
			printk(", valid once") : printk(",%*c", 11, ' ');
		(reg_val & SPIM_CMD_TABLE_LOCK_BIT) != 0 ?
			printk(", locked|") : printk(",%*c|", 7, ' ');
		printk("\n");
	}

	release_spim_device(dev);
}

uint32_t spim_get_cmd_table_val(uint8_t cmd)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(cmds_array); i++) {
		if (cmds_array[i].cmd == cmd)
			return cmds_array[i].cmd_table_val;
	}

	LOG_ERR("Error: Cannot get item in command table cmd(%02x)\n", cmd);
	return 0;
}

void spim_allow_cmd_table_init(const struct device *dev,
	const uint8_t cmd_list[], uint32_t cmd_num, uint32_t flag)
{
	const struct aspeed_spim_config *config = dev->config;
	mm_reg_t table_base = config->ctrl_base + SPIM_ALLOW_CMD_BASE;
	uint32_t i;
	uint32_t reg_val;
	uint32_t idx = 3;

	acquire_spim_device(dev);

	for (i = 0; i < cmd_num; i++) {
		reg_val = spim_get_cmd_table_val(cmd_list[i]);
		LOG_DBG("cmd %02x, val %08x", cmd_list[i], reg_val);
		if (reg_val == 0) {
			LOG_ERR("cmd is not recorded in cmds_array array");
			LOG_ERR("please edit it in spi_monitor_aspeed.c");
			continue;
		}

		if (flag & FLAG_CMD_TABLE_VALID_ONCE)
			reg_val |= SPIM_CMD_TABLE_VALID_ONCE_BIT;
		else
			reg_val |= SPIM_CMD_TABLE_VALID_BIT;

		switch (cmd_list[i]) {
		case CMD_EN4B:
			sys_write32(reg_val, table_base);
			continue;

		case CMD_EX4B:
			sys_write32(reg_val, table_base + 4);
			continue;
		default:
			idx++;
		}

		sys_write32(reg_val, table_base + idx * 4);
	}

	release_spim_device(dev);
}

static int spim_get_empty_allow_cmd_slot(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;
	int idx;
	uint32_t reg_val;

	for (idx = 4; idx < SPIM_CMD_TABLE_NUM; idx++) {
		reg_val = sys_read32(config->ctrl_base + SPIM_ALLOW_CMD_BASE + idx * 4);
		if (reg_val == 0)
			return idx;
	}

	return -ENOSR;
}

static int spim_get_allow_cmd_slot(const struct device *dev,
	uint8_t cmd, uint32_t start_off)
{
	const struct aspeed_spim_config *config = dev->config;
	int idx;
	uint32_t reg_val;

	for (idx = start_off; idx < SPIM_CMD_TABLE_NUM; idx++) {
		reg_val = sys_read32(config->ctrl_base + SPIM_ALLOW_CMD_BASE + idx * 4);
		if ((reg_val & SPIM_CMD_TABLE_CMD_MASK) == cmd)
			return idx;
	}

	return -ENOSR;
}

/* - If the command already exists in allow command table and
 *   it is disabled, it will be enabled by spim_add_allow_command.
 * - If the command already exists in allow command table and
 *   it is lock, it will not be enabled and an error code will
 *   be returned.
 * - If the command doesn't exist in allow command table, an
 *   empty slot will be found and the command info will be
 *   filled into.
 */

int spim_add_allow_command(const struct device *dev,
	uint8_t cmd, uint32_t flag)
{
	int ret = 0;
	const struct aspeed_spim_config *config = dev->config;
	mm_reg_t table_base = config->ctrl_base + SPIM_ALLOW_CMD_BASE;
	int idx;
	uint32_t off;
	uint32_t reg_val;
	bool found = false;

	acquire_spim_device(dev);

	/* check whether the command is already recorded in allow cmd table */
	for (off = 0; off < SPIM_CMD_TABLE_NUM; off++) {
		idx = spim_get_allow_cmd_slot(dev, cmd, off);
		if (idx >= 0) {
			found = true;
			reg_val = sys_read32(table_base + idx * 4);
			if ((reg_val & SPIM_CMD_TABLE_LOCK_BIT) != 0) {
				LOG_WRN("cmd %02x cannot be enabled in allow cmd table(%d)", cmd, idx);
				off = idx + 1;
				/* search for the next slot with the same command */
				continue;
			} else {
				reg_val &= ~SPIM_CMD_TABLE_VALID_MASK;
				if (flag & FLAG_CMD_TABLE_VALID_ONCE)
					reg_val |= SPIM_CMD_TABLE_VALID_ONCE_BIT;
				else
					reg_val |= SPIM_CMD_TABLE_VALID_BIT;

				sys_write32(reg_val, table_base + idx * 4);
				found = true;
				goto end;
			}
		} else {
			break;
		}
	}

	/* If the cmd already exists in the allow command table and
	 * the register is locked, the same command should not be added again.
	 */
	if (found) {
		LOG_WRN("cmd %02x should not be added in the allow command table again", cmd);
		goto end;
	}

	reg_val = spim_get_cmd_table_val(cmd);
	/* cmd info is not found in allow table array */
	if (reg_val == 0) {
		LOG_ERR("cmd is not recorded in \"cmds_array\" array");
		LOG_ERR("please edit it in spi_monitor_aspeed.c");
		ret = -EINVAL;
		goto end;
	}

	if (flag & FLAG_CMD_TABLE_VALID_ONCE)
		reg_val |= SPIM_CMD_TABLE_VALID_ONCE_BIT;
	else
		reg_val |= SPIM_CMD_TABLE_VALID_BIT;

	switch (cmd) {
	case CMD_EN4B:
		sys_write32(reg_val, table_base);
		goto end;

	case CMD_EX4B:
		sys_write32(reg_val, table_base + 4);
		goto end;

	default:
		break;
	}

	idx = spim_get_empty_allow_cmd_slot(dev);
	if (idx < 0) {
		LOG_ERR("No more space for new command");
		ret = -ENOSR;
		goto end;
	}
	sys_write32(reg_val, table_base + idx * 4);

end:
	release_spim_device(dev);

	return ret;
}

/* All command table slot which command is equal to "cmd"
 * parameter will be removed.
 */
int spim_remove_allow_command(const struct device *dev, uint8_t cmd)
{
	int ret = 0;
	const struct aspeed_spim_config *config = dev->config;
	mm_reg_t table_base = config->ctrl_base + SPIM_ALLOW_CMD_BASE;
	int idx;
	uint32_t off;
	uint32_t reg_val;
	bool found = false;

	acquire_spim_device(dev);

	/* check whether the command is already recorded in allow cmd table */
	for (off = 0; off < SPIM_CMD_TABLE_NUM; off++) {
		idx = spim_get_allow_cmd_slot(dev, cmd, off);
		if (idx >= 0) {
			found = true;
			reg_val = sys_read32(table_base + idx * 4);
			if ((reg_val & SPIM_CMD_TABLE_LOCK_BIT) != 0 &&
				(reg_val & SPIM_CMD_TABLE_VALID_MASK) != 0) {
				LOG_ERR("cmd %02x is locked and cannot be removed or disabled. (%d)",
					cmd, idx);
				ret = -EINVAL;
				goto end;
			} else if ((reg_val & SPIM_CMD_TABLE_LOCK_BIT) == 0) {
				sys_write32(0, table_base + idx * 4);
			} else {
				LOG_INF("cmd %02x is locked and cannot be removed. (%d)",
					cmd, idx);
			}

			off = idx + 1;
			continue;
		} else {
			break;
		}
	}

	if (!found) {
		LOG_ERR("cmd %02x is not found in allow command table", cmd);
		ret = -EINVAL;
		goto end;
	}

end:
	release_spim_device(dev);

	return ret;
}

/* - The overall allow command table will be locked when
 *   flag is FLAG_CMD_TABLE_LOCK_ALL.
 * - All command table slot which command is equal to "cmd"
 *   parameter will be locked.
 */
int spim_lock_allow_command_table(const struct device *dev,
	uint8_t cmd, uint32_t flag)
{
	int ret = 0;
	const struct aspeed_spim_config *config = dev->config;
	mm_reg_t table_base = config->ctrl_base + SPIM_ALLOW_CMD_BASE;
	int idx;
	uint32_t off;
	uint32_t reg_val;
	bool found = false;

	acquire_spim_device(dev);

	if ((flag & FLAG_CMD_TABLE_LOCK_ALL) != 0) {
		for (idx = 0; idx < SPIM_CMD_TABLE_NUM; idx++) {
			reg_val = sys_read32(table_base + idx * 4);
			reg_val |= SPIM_CMD_TABLE_LOCK_BIT;
			sys_write32(reg_val, table_base + idx * 4);
		}
		goto end;
	}

	for (off = 0; off < SPIM_CMD_TABLE_NUM; off++) {
		idx = spim_get_allow_cmd_slot(dev, cmd, off);
		if (idx >= 0) {
			found = true;
			reg_val = sys_read32(table_base + idx * 4);
			if ((reg_val & SPIM_CMD_TABLE_LOCK_BIT) != 0) {
				LOG_INF("cmd %02x is already locked (%d)", cmd, idx);
			} else {
				reg_val |= SPIM_CMD_TABLE_LOCK_BIT;
				sys_write32(reg_val, table_base + idx * 4);
			}

			off = idx + 1;
			continue;
		} else {
			break;
		}
	}

	if (!found) {
		LOG_ERR("cmd %02x is not found in allow command table", cmd);
		ret = -EINVAL;
		goto end;
	}

end:
	release_spim_device(dev);
	return ret;
}

#define SPIM_ABS_ADDR(reg_off, bit_off) (reg_off * 524288 + bit_off * 16384)

static void spim_addr_priv_access_enable(
	const struct device *dev, enum addr_priv_rw_select select)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;

	reg_val = sys_read32(config->ctrl_base);
	reg_val &= 0x00FFFFFF;

	switch (select) {
	case FLAG_ADDR_PRIV_READ_SELECT:
		reg_val |= SPIM_PRIV_READ_SELECT;
		break;
	case FLAG_ADDR_PRIV_WRITE_SELECT:
		reg_val |= SPIM_PRIV_WRITE_SELECT;
		break;
	default:
		break;
	};

	sys_write32(reg_val, config->ctrl_base);
}

static void spim_fobidden_area_parser(const struct device *dev,
	struct priv_reg_info start, struct priv_reg_info *res,
	uint32_t *num_forbidden_blk)
{
	const struct aspeed_spim_config *config = dev->config;
	mm_reg_t priv_table_base = config->ctrl_base + SPIM_ADDR_PRIV_TABLE_BASE;
	uint32_t reg_off = start.start_reg_off;
	uint32_t bit_off = start.start_bit_off;
	uint32_t reg_val;
	uint32_t i;

	/* init search result */
	*num_forbidden_blk = 0;

	while (reg_off < SPIM_ADDR_PRIV_REG_NUN) {
		reg_val = sys_read32(priv_table_base + reg_off * 4);
		reg_val >>= bit_off;
		for (i = bit_off; i < 32; i++) {
			if ((reg_val & 1) == 0) {
				if (*num_forbidden_blk == 0) {
					/* get the first forbidden block */
					res->start_reg_off = reg_off;
					res->start_bit_off = i;
				}

				(*num_forbidden_blk)++;
			} else if ((reg_val & 1) == 1 && *num_forbidden_blk != 0) {
				res->end_reg_off = reg_off;
				res->end_bit_off = i;
				return;
			}

			reg_val >>= 1;
		}

		bit_off = 0;
		reg_off++;
	}

	res->end_reg_off = SPIM_ADDR_PRIV_REG_NUN - 1;
	res->end_bit_off = 32;
}

void spim_dump_rw_addr_privilege_table(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t num_forbidden_blk = 0;
	struct priv_reg_info start;
	struct priv_reg_info res;
	bool protect_en = false;
	uint32_t rw;
	bool lock;

	acquire_spim_device(dev);

	for (rw = 0; rw < 2; rw++) {
		if (rw == 0)
			spim_addr_priv_access_enable(dev, FLAG_ADDR_PRIV_READ_SELECT);
		else
			spim_addr_priv_access_enable(dev, FLAG_ADDR_PRIV_WRITE_SELECT);

		lock = false;
		if (sys_read32(config->ctrl_base + SPIM_LOCK_REG) & BIT(31 - rw))
			lock = true;

		memset(&start, 0x0, sizeof(struct priv_reg_info));
		memset(&res, 0x0, sizeof(struct priv_reg_info));
		printk("%s protection regions:\n", rw == 0 ? "read" : "write");
		printk("privilege table is %s\n", lock ? "locked" : "unlocked");
		do {
			spim_fobidden_area_parser(dev, start, &res, &num_forbidden_blk);
			if (num_forbidden_blk != 0) {
				protect_en = true;
				printk("[0x%08x - 0x%08x]\n",
					SPIM_ABS_ADDR(res.start_reg_off, res.start_bit_off),
					SPIM_ABS_ADDR(res.end_reg_off, res.end_bit_off));
				start.start_reg_off = res.end_reg_off;
				start.start_bit_off = res.end_bit_off;
			}
		} while (num_forbidden_blk != 0);

		if (!protect_en)
			printk("all regions are %s!\n", rw == 0 ? "readable" : "writable");
		printk("======END======\n\n");
	}

	release_spim_device(dev);
}

static uint32_t spim_get_cross_block_num(uint32_t addr, uint32_t len)
{
	if (len == 0)
		return 0;

	if (addr % KB(16) != 0)
		len += (addr % KB(16));

	/* 16KB aligned */
	len = (len + KB(16) - 1) / KB(16) * KB(16);

	return len / KB(16);
}

int spim_address_privilege_config(const struct device *dev,
	enum addr_priv_rw_select rw_select, enum addr_priv_op priv_op,
	mm_reg_t addr, uint32_t len)
{
	const struct aspeed_spim_config *config = dev->config;
	mm_reg_t priv_table_base = config->ctrl_base + SPIM_ADDR_PRIV_TABLE_BASE;
	int ret = 0;
	uint32_t reg_off;
	uint32_t bit_off;
	uint32_t total_bit_num;
	uint32_t reg_val;

	if (addr >= MB(256) || len == 0) {
		LOG_WRN("invalid address or zero length!");
		ret = -EINVAL;
		goto end;
	}

	if (addr + len > MB(256)) {
		LOG_WRN("invalid protected regions, change the protected length...");
		len -= (addr + len - MB(256));
		LOG_WRN("the new length: 0x%08x", len);
	}

	if ((addr % KB(16)) != 0 || (len % KB(16)) != 0) {
		LOG_WRN("protected address(0x%08lx) and length(0x%08x) should be 16KB aligned",
			addr, len);
		LOG_WRN("stricter protection regions will be applied. (force 16KB aligned)");
		/* protect more region in order to align 16KB boundary */
		len = addr + len - (addr / KB(16)) * KB(16);
		addr = (addr / KB(16)) * KB(16);
		len = ((len + KB(16) - 1) / KB(16)) * KB(16);
	}

	reg_off = addr / KB(512); /* 512K per register; */
	bit_off = (addr % KB(512)) / KB(16); /* (512K / 16K); */
	total_bit_num = spim_get_cross_block_num(addr, len);
	LOG_DBG("addr: 0x%08lx, len: 0x%08x\n", addr, len);
	LOG_DBG("reg_off: 0x%08x, bit_off: 0x%08x, total_bit_num: 0x%08x\n",
		reg_off, bit_off, total_bit_num);

	acquire_spim_device(dev);

	/* check lock status */
	if (rw_select == FLAG_ADDR_PRIV_READ_SELECT &&
		(sys_read32(config->ctrl_base + SPIM_LOCK_REG) & BIT(31))) {
		LOG_ERR("read address privilege table is locked!");
		ret = -ECANCELED;
		goto end;
	} else if (rw_select == FLAG_ADDR_PRIV_WRITE_SELECT &&
		(sys_read32(config->ctrl_base + SPIM_LOCK_REG) & BIT(30))) {
		LOG_ERR("write address privilege table is locked!");
		ret = -ECANCELED;
		goto end;
	}

	/* enable access */
	if (rw_select == FLAG_ADDR_PRIV_READ_SELECT)
		spim_addr_priv_access_enable(dev, FLAG_ADDR_PRIV_READ_SELECT);
	else
		spim_addr_priv_access_enable(dev, FLAG_ADDR_PRIV_WRITE_SELECT);

	do {
		if (bit_off > 31) {
			bit_off = 0;
			reg_off++;
		}

		if (bit_off == 0 && total_bit_num >= 32) {
			/* speed up for large area configuration */
			if (priv_op == FLAG_ADDR_PRIV_ENABLE)
				sys_write32(0xffffffff, priv_table_base + reg_off * 4);
			else
				sys_write32(0x0, priv_table_base + reg_off * 4);
			reg_off++;
			total_bit_num -= 32;
		} else {
			reg_val = sys_read32(priv_table_base + reg_off * 4);
			if (priv_op == FLAG_ADDR_PRIV_ENABLE)
				sys_write32(reg_val | BIT(bit_off), priv_table_base + reg_off * 4);
			else
				sys_write32(reg_val & (~BIT(bit_off)), priv_table_base + reg_off * 4);

			LOG_DBG("reg: 0x%08lx, val: 0x%08x\n", priv_table_base + reg_off * 4,
				sys_read32(priv_table_base + reg_off * 4));
			bit_off++;
			total_bit_num--;
		}
	} while (total_bit_num > 0);

end:
	release_spim_device(dev);

	return ret;
}

void spim_lock_rw_privilege_table(const struct device *dev,
	enum addr_priv_rw_select rw_select)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;

	acquire_spim_device(dev);

	reg_val = sys_read32(config->ctrl_base + SPIM_LOCK_REG);

	if (rw_select == FLAG_ADDR_PRIV_READ_SELECT) {
		reg_val |= SPIM_ADDR_PRIV_READ_TABLE_LOCK;
	}

	if (rw_select == FLAG_ADDR_PRIV_WRITE_SELECT) {
		reg_val |= SPIM_ADDR_PRIV_WRITE_TABLE_LOCK;
	}

	sys_write32(reg_val, config->ctrl_base + SPIM_LOCK_REG);

	release_spim_device(dev);
}

void spim_lock_common(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;


	spim_lock_rw_privilege_table(dev, FLAG_ADDR_PRIV_READ_SELECT);
	spim_lock_rw_privilege_table(dev, FLAG_ADDR_PRIV_WRITE_SELECT);
	spim_lock_allow_command_table(dev, 0, FLAG_CMD_TABLE_LOCK_ALL);

	acquire_spim_device(dev);

	reg_val = sys_read32(config->ctrl_base);

	reg_val |= SPIM_BLOCK_FIFO_CTRL_LOCK |
				SPIM_SW_RST_CTRL_LOCK;

	sys_write32(reg_val, config->ctrl_base);

	reg_val = sys_read32(config->ctrl_base + SPIM_LOCK_REG);

	reg_val |= SPIM_CTRL_REG_LOCK |
				SPIM_INT_STS_REG_LOCK |
				SPIM_LOG_BASE_ADDR_REG_LOCK |
				SPIM_LOG_CTRL_REG_LOCK;

	sys_write32(reg_val, config->ctrl_base + SPIM_LOCK_REG);

	release_spim_device(dev);
}

void spim_rw_perm_init(const struct device *dev)
{
	struct aspeed_spim_data *const data = dev->data;
	int ret;
	uint32_t i;

	spim_address_privilege_config(dev, FLAG_ADDR_PRIV_READ_SELECT,
		FLAG_ADDR_PRIV_ENABLE, 0x0, MB(256));

	spim_address_privilege_config(dev, FLAG_ADDR_PRIV_WRITE_SELECT,
		FLAG_ADDR_PRIV_ENABLE, 0x0, MB(256));

	if (data->read_forbidden_region_num % 2 != 0)
		LOG_ERR("wrong read-forbidden-regions setting in .dts.");

	if (data->write_forbidden_region_num % 2 != 0)
		LOG_ERR("wrong write-forbidden-regions setting in .dts.");

	for (i = 0; i < data->read_forbidden_region_num; i += 2) {
		LOG_DBG("[%s]addr: 0x%08x, len: 0x%08x", dev->name,
				data->read_forbidden_regions[i],
				data->read_forbidden_regions[i + 1]);

		ret = spim_address_privilege_config(dev,
			FLAG_ADDR_PRIV_READ_SELECT,
			FLAG_ADDR_PRIV_DISABLE,
			data->read_forbidden_regions[i],
			data->read_forbidden_regions[i + 1]);
		if (ret != 0)
			LOG_ERR("fail to configure read address privilege table!");
	}

	for (i = 0; i < data->write_forbidden_region_num; i += 2) {
		LOG_DBG("[%s]addr: 0x%08x, len: 0x%08x", dev->name,
				data->write_forbidden_regions[i],
				data->write_forbidden_regions[i + 1]);

		ret = spim_address_privilege_config(dev,
			FLAG_ADDR_PRIV_WRITE_SELECT,
			FLAG_ADDR_PRIV_DISABLE,
			data->write_forbidden_regions[i],
			data->write_forbidden_regions[i + 1]);
		if (ret != 0)
			LOG_ERR("fail to configure write address privilege table!");
	}
}

void spim_scu_monitor_config(const struct device *dev, bool enable)
{
	const struct aspeed_spim_config *config = dev->config;

	if (enable) {
		spim_scu_ctrl_set(config->parent, BIT(config->ctrl_idx - 1) << 8,
			BIT(config->ctrl_idx - 1) << 8);
	} else {
		spim_scu_ctrl_clear(config->parent, BIT(config->ctrl_idx - 1) << 8);
	}
}

void spim_ctrl_monitor_config(const struct device *dev, bool enable)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;

	acquire_spim_device(dev);

	reg_val = sys_read32(config->ctrl_base);
	if (enable)
		reg_val |= BIT(2);
	else
		reg_val &= ~(BIT(2));

	sys_write32(reg_val, config->ctrl_base);

	release_spim_device(dev);
}

void spim_monitor_enable(const struct device *dev, bool enable)
{
	spim_scu_monitor_config(dev, enable);
	spim_ctrl_monitor_config(dev, enable);
}

void spim_rst_flash(const struct device *dev, uint32_t rst_duration_ms)
{
	uint32_t val;
	const struct aspeed_spim_config *config = dev->config;
	const struct device *parent_dev = config->parent;
	uint32_t bit_off = 1 << (config->ctrl_idx - 1);

	if (rst_duration_ms == 0)
		rst_duration_ms = 500;

	/* Using SCU0F0 to enable flash rst
	 * SCU0F0[23:20]: Reset source selection
	 * SCU0F0[27:24]: Enable reset signal output
	 */
	val = (bit_off << 20) | (bit_off << 24);
	spim_scu_ctrl_set(parent_dev, val, val);

	/* SCU0F0[19:16]: output value */
	/* reset flash */
	val = bit_off << 16;
	spim_scu_ctrl_clear(parent_dev, val);

	k_busy_wait(rst_duration_ms * 1000);

	/* release reset */
	spim_scu_ctrl_set(parent_dev, val, val);

	spi_nor_config_4byte_mode(config->flash_dev, false);

	k_busy_wait(5000); /* 5ms */
}

void spim_get_log_info(const struct device *dev, struct spim_log_info *info)
{
	const struct aspeed_spim_config *config = dev->config;
	struct aspeed_spim_data *const data = dev->data;

	if (info == NULL)
		return;

	info->log_ram_addr = data->log_info.log_ram_addr;
	info->log_max_sz = data->log_info.log_max_sz;
	info->log_idx_reg = sys_read32(config->ctrl_base + SPIM_LOG_PTR);

	return;
}

uint32_t spim_get_ctrl_idx(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;

	return config->ctrl_idx;
}

void spim_isr_callback_install(const struct device *dev,
	spim_isr_callback_t isr_callback)
{
	struct aspeed_spim_data *const data = dev->data;

	data->isr_callback = isr_callback;
}

void spim_isr(const void *param)
{
	const struct device *dev = param;
	const struct aspeed_spim_config *config = dev->config;
	struct aspeed_spim_data *const data = dev->data;
	uint32_t reg_val;
	uint32_t sts_backup;
	k_spinlock_key_t key = k_spin_lock(&data->irq_ctrl_lock);

	if (data->isr_callback != NULL)
		data->isr_callback(dev);

	/* ack */
	reg_val = sys_read32(config->ctrl_base + SPIM_IRQ_CTRL);
	sts_backup = reg_val & SPIM_IRQ_STS_MASK;
	sys_write32(reg_val | SPIM_IRQ_STS_MASK, config->ctrl_base + SPIM_IRQ_CTRL);

	k_spin_unlock(&data->irq_ctrl_lock, key);
}

void spim_irq_enable(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;
	struct aspeed_spim_data *const data = dev->data;
	uint32_t reg_val;
	k_spinlock_key_t key = k_spin_lock(&data->irq_ctrl_lock);

	reg_val = sys_read32(config->ctrl_base + SPIM_IRQ_CTRL);
	reg_val |= (SPIM_CMD_BLOCK_IRQ_EN | SPIM_WRITE_BLOCK_IRQ_EN |
				SPIM_READ_BLOCK_IRQ_EN);
	sys_write32(reg_val, config->ctrl_base + SPIM_IRQ_CTRL);

	k_spin_unlock(&data->irq_ctrl_lock, key);
}

static int spim_abnormal_log_init(const struct device *dev)
{
	int ret = 0;
	const struct aspeed_spim_config *config = dev->config;
	struct aspeed_spim_data *const data = dev->data;
	const struct device *parent_dev = config->parent;
	struct aspeed_spim_common_data *const parent_data = parent_dev->data;
	uint32_t cur_log_sz;

	acquire_log_op(config->parent);
	data->log_info.log_ram_addr = (mem_addr_t)(&spim_log_arr[0] + parent_data->cur_log_sz);
	parent_data->cur_log_sz += data->log_info.log_max_sz;
	cur_log_sz = parent_data->cur_log_sz;
	release_log_op(config->parent);

	if (cur_log_sz > SPIM_LOG_RAM_TOTAL_SIZE) {
		LOG_ERR("[%s]invalid log size on ram", dev->name);
		ret = -ENOBUFS;
		goto end;
	}

	acquire_spim_device(dev);
	sys_write32(data->log_info.log_ram_addr, config->ctrl_base + SPIM_LOG_BASE);
	sys_write32(data->log_info.log_max_sz | SPIM_BLOCK_INFO_EN,
			config->ctrl_base + SPIM_LOG_SZ);
	release_spim_device(dev);

end:
	return ret;
}
void aspeed_spi_monitor_sw_rst(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;
	uint32_t reg_val;

	acquire_spim_device(dev);

	reg_val = sys_read32(config->ctrl_base + SPIM_CTRL);
	reg_val |= SPIM_SW_RST;
	sys_write32(reg_val, config->ctrl_base + SPIM_CTRL);

	k_usleep(5);

	reg_val &= ~(SPIM_SW_RST);
	sys_write32(reg_val, config->ctrl_base + SPIM_CTRL);

	release_spim_device(dev);
}

static int aspeed_spi_monitor_init(const struct device *dev)
{
	const struct aspeed_spim_config *config = dev->config;
	struct aspeed_spim_data *const data = dev->data;
	int ret = 0;

	if (IS_ENABLED(CONFIG_MULTITHREADING))
		k_sem_init(&data->sem_spim, 1, 1);

	/* always enable internal passthrough configuration */
	spim_scu_passthrough_mode(dev, 0, true);
	/* always keep at master mode during booting up stage */
	spim_ext_mux_config(dev, SPIM_MASTER_MODE);

	if (config->extra_clk_en)
		spim_block_mode_config(dev, SPIM_BLOCK_EXTRA_CLK);

	spim_allow_cmd_table_init(dev, data->allow_cmd_list, data->allow_cmd_num, 0);
	spim_rw_perm_init(dev);
	spim_monitor_enable(dev, true);

	/* log info init */
	ret = spim_abnormal_log_init(dev);
	if (ret != 0)
		return ret;

	/* irq init */
	irq_connect_dynamic(config->irq_num, config->irq_priority,
		spim_isr, dev, 0);
	irq_enable(config->irq_num);
	spim_irq_enable(dev);

	return 0;
}

static int aspeed_spi_monitor_common_init(const struct device *dev)
{
	struct aspeed_spim_common_data *const data = dev->data;

	data->cur_log_sz = 0;
	if (IS_ENABLED(CONFIG_MULTITHREADING))
		k_sem_init(&data->sem_log_op, 1, 1);

	memset(spim_log_arr, 0x0, SPIM_LOG_RAM_TOTAL_SIZE);

	return 0;
}

#define SPIM_ENUM(node_id) node_id,

#define ASPEED_SPIM_DEV_CFG(node_id) {	\
		.ctrl_base = DT_REG_ADDR(DT_PARENT(node_id)) + 0x1000 * (DT_REG_ADDR(node_id) - 1),	\
		.irq_num = DT_IRQN(node_id),		\
		.irq_priority = DT_IRQ(node_id, priority),	\
		.ctrl_idx = DT_REG_ADDR(node_id),	\
		.extra_clk_en = DT_PROP(node_id, extra_clk),	\
		.parent = DEVICE_DT_GET(DT_PARENT(node_id)),	\
		.flash_dev = DEVICE_DT_GET(DT_PHANDLE(node_id, flash_device)),	\
},

#define ASPEED_SPIM_DEV_DATA(node_id) {	\
		.allow_cmd_list = DT_PROP(node_id, allow_cmds), \
		.allow_cmd_num = DT_PROP_LEN(node_id, allow_cmds), \
		.read_forbidden_regions = DT_PROP(node_id, read_forbidden_regions),	\
		.read_forbidden_region_num = DT_PROP_LEN(node_id, read_forbidden_regions),	\
		.write_forbidden_regions = DT_PROP(node_id, write_forbidden_regions),	\
		.write_forbidden_region_num = DT_PROP_LEN(node_id, write_forbidden_regions),	\
		.log_info.log_max_sz = DT_PROP_OR(node_id, log_ram_size, 0x200),	\
		.dev = DEVICE_DT_GET(node_id),	\
},

/* child node define */
#define ASPEED_SPIM_DT_DEFINE(node_id)	\
	DEVICE_DT_DEFINE(node_id, aspeed_spi_monitor_init,	\
					NULL,	\
					&aspeed_spim_data[node_id],		\
					&aspeed_spim_config[node_id],	\
					POST_KERNEL, 71, NULL);

/* common node define */
#define ASPEED_SPI_MONITOR_COMMON_INIT(n)	\
	static struct aspeed_spim_common_config aspeed_spim_common_config_##n = { \
		.scu_base = DT_REG_ADDR_BY_IDX(DT_INST_PHANDLE_BY_IDX(n, aspeed_scu, 0), 0), \
	};								\
	static struct aspeed_spim_common_data aspeed_spim_common_data_##n;	\
		\
	DEVICE_DT_INST_DEFINE(n, &aspeed_spi_monitor_common_init,			\
			    NULL,					\
			    &aspeed_spim_common_data_##n,			\
			    &aspeed_spim_common_config_##n, POST_KERNEL,	\
			    70,		\
			    NULL);		\
	/* handle child node */	\
	static const struct aspeed_spim_config aspeed_spim_config[] = {	\
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), ASPEED_SPIM_DEV_CFG)};		\
	static struct aspeed_spim_data aspeed_spim_data[] = {			\
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), ASPEED_SPIM_DEV_DATA)};	\
		\
	enum {DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), SPIM_ENUM)};	\
	DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), ASPEED_SPIM_DT_DEFINE)


DT_INST_FOREACH_STATUS_OKAY(ASPEED_SPI_MONITOR_COMMON_INIT)
