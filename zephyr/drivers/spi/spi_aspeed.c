/*
 * Copyright (c) 2021 Chin-Ting Kuo <chin-ting_kuo@aspeedtech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_spi_controller

#include <device.h>
#include <drivers/clock_control.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <drivers/spi_nor.h>
#include <errno.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(spi_aspeed, CONFIG_SPI_LOG_LEVEL);
#include "spi_context.h"
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>

#define SPI00_CE_TYPE_SETTING       (0x0000)
#define SPI04_CE_CTRL               (0x0004)
#define SPI08_INTR_CTRL             (0x0008)
#define SPI0C_CMD_CTRL              (0x000C)

#define SPI10_CE0_CTRL              (0x0010)
#define SPI14_CE1_CTRL              (0x0014)
#define SPI18_CE2_CTRL              (0x0018)
#define DUMMY_REG                   (0x001C)
#define SPI30_CE0_ADDR_DEC          (0x0030)
#define SPI34_CE1_ADDR_DEC          (0x0034)
#define SPI38_CE2_ADDR_DEC          (0x0038)

#define SPI50_SOFT_RST_CTRL         (0x0050)

#define SPI60_WDT1                  (0x0060)
#define SPI64_WDT2                  (0x0064)
#define SPI68_WDT2_RELOAD_VAL       (0x0068)
#define SPI6C_WDT2_RESTART          (0x006C)

#define SPI7C_DMA_BUF_LEN           (0x007C)
#define SPI80_DMA_CTRL              (0x0080)
#define SPI84_DMA_FLASH_ADDR        (0x0084)
#define SPI88_DMA_RAM_ADDR          (0x0088)
#define SPI8C_DMA_LEN               (0x008C)
#define SPI90_CHECKSUM_RESULT       (0x0090)
#define SPI94_CE0_TIMING_CTRL       (0x0094)
#define SPI98_CE1_TIMING_CTRL       (0x0098)
#define SPI9C_CE2_TIMING_CTRL       (0x009C)

#define SPIA0_CMD_FILTER_CTRL       (0x00A0)
#define SPIA4_ADDR_FILTER_CTRL      (0x00A4)
#define SPIA8_REG_LOCK_SRST         (0x00A8)
#define SPIAC_REG_LOCK_WDT          (0x00AC)
/* used for HW SAFS setting */
#define SPIR6C_HOST_DIRECT_ACCESS_CMD_CTRL4	(0x006c)
#define SPIR74_HOST_DIRECT_ACCESS_CMD_CTRL2	(0x0074)

#define SPI_CALIB_LEN               0x400
#define SPI_DMA_STS                 BIT(11)
#define SPI_DMA_IRQ_EN              BIT(3)
#define SPI_DAM_REQUEST             BIT(31)
#define SPI_DAM_GRANT               BIT(30)
#define SPI_DMA_CALIB_MODE          BIT(3)
#define SPI_DMA_CALC_CKSUM          BIT(2)
#define SPI_DMA_WRITE               BIT(1)
#define SPI_DMA_ENABLE              BIT(0)
#define SPI_DMA_STATUS              BIT(11)
#define SPI_DMA_GET_REQ_MAGIC       0xaeed0000
#define SPI_DMA_DISCARD_REQ_MAGIC   0xdeea0000
#define SPI_DMA_TRIGGER_LEN         128
#define SPI_DMA_RAM_MAP_BASE        0x80000000
#define SPI_DMA_FLASH_MAP_BASE      0x60000000

#define SPI_CTRL_FREQ_MASK          0x0F000F00

#define ASPEED_MAX_CS               5

#define ASPEED_SPI_NORMAL_READ      0x1
#define ASPEED_SPI_NORMAL_WRITE     0x2
#define ASPEED_SPI_USER             0x3
#define ASPEED_SPI_USER_INACTIVE    BIT(2)

#define ASPEED_SPI_SZ_2M            0x200000
#define ASPEED_SPI_SZ_256M          0x10000000

#define ASPEED_SPI_CTRL_VAL(io_mode, opcode, dummy_cycle) \
		(io_mode | ((opcode & 0xff) << 16) | dummy_cycle)

enum aspeed_ctrl_type {
	BOOT_SPI,
	HOST_SPI,
	NORMAL_SPI
};

struct aspeed_cmd_mode {
	uint32_t normal_read;
	uint32_t normal_write;
	uint32_t user;
};

struct aspeed_spi_decoded_addr {
	mm_reg_t start;
	uint32_t len;
};

struct aspeed_spi_data {
	struct spi_context ctx;
	uint32_t (*segment_start)(uint32_t val);
	uint32_t (*segment_end)(uint32_t val);
	uint32_t (*segment_value)(uint32_t start, uint32_t end);
	struct aspeed_spi_decoded_addr decode_addr[ASPEED_MAX_CS];
	struct aspeed_cmd_mode cmd_mode[ASPEED_MAX_CS];
	uint32_t hclk;
};

struct aspeed_spim_internal_mux_ctrl {
	uint32_t master_idx;
	uint32_t spim_output_base;
	const struct device *spi_monitor_common_ctrl;
};

struct aspeed_spi_config {
	mm_reg_t ctrl_base;
	mm_reg_t spi_mmap_base;
	uint32_t max_cs;
	uint32_t platform;
	enum aspeed_ctrl_type ctrl_type;
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	uint32_t irq_num;
	uint32_t irq_priority;
	struct aspeed_spim_internal_mux_ctrl mux_ctrl;
};

uint32_t ast2600_segment_addr_start(uint32_t reg_val)
{
	return ((reg_val & 0x0ff0) << 16);
}

uint32_t ast2600_segment_addr_end(uint32_t reg_val)
{
	return ((reg_val & 0x0ff00000) | 0x000fffff);
}

uint32_t ast2600_segment_addr_val(uint32_t start, uint32_t end)
{
	return ((((((start) >> 20) << 20) >> 16) & 0xffff) | ((((end) >> 20) << 20) & 0xffff0000));
}

uint32_t ast1030_fmc_segment_addr_start(uint32_t reg_val)
{
	return ((reg_val & 0x0ff8) << 16);
}

uint32_t ast1030_fmc_segment_addr_end(uint32_t reg_val)
{
	return ((reg_val & 0x0ff80000) | 0x0007ffff);
}

uint32_t ast1030_fmc_segment_addr_val(uint32_t start, uint32_t end)
{
	return ((((((start) >> 19) << 19) >> 16) & 0xfff8) | ((((end) >> 19) << 19) & 0xfff80000));
}

uint32_t ast1030_spi_segment_addr_start(uint32_t reg_val)
{
	return ((reg_val & 0x0ff0) << 16);
}

uint32_t ast1030_spi_segment_addr_end(uint32_t reg_val)
{
	return ((reg_val & 0x0ff00000) | 0x000fffff);
}

uint32_t ast1030_spi_segment_addr_val(uint32_t start, uint32_t end)
{
	return ((((((start) >> 20) << 20) >> 16) & 0xffff) | ((((end) >> 20) << 20) & 0xffff0000));
}

static uint32_t aspeed_spi_io_mode(enum jesd216_mode_type mode)
{
	uint32_t reg = 0;

	switch (mode) {
	case JESD216_MODE_111:
	case JESD216_MODE_111_FAST:
		reg = 0x00000000;
		break;
	case JESD216_MODE_112:
		reg = 0x20000000;
		break;
	case JESD216_MODE_122:
		reg = 0x30000000;
		break;
	case JESD216_MODE_114:
		reg = 0x40000000;
		break;
	case JESD216_MODE_144:
		reg = 0x50000000;
		break;
	default:
		LOG_ERR("Unsupported io mode 0x08%x", mode);
		break;
	}

	return reg;
}

static uint32_t aspeed_spi_io_mode_user(uint32_t bus_width)
{
	uint32_t reg;

	switch (bus_width) {
	case 4:
		reg = 0x40000000;
		break;
	case 2:
		reg = 0x20000000;
		break;
	default:
		reg = 0x00000000;
		break;
	}

	return reg;
}

static uint32_t aspeed_spi_cal_dummy_cycle(
		uint32_t bus_width, uint32_t dummy_cycle)
{
	uint32_t dummy_byte = 0;

	dummy_byte = dummy_cycle / (8 / bus_width);

	return (((dummy_byte & 0x3) << 6) | (((dummy_byte & 0x4) >> 2) << 14));
}

static void aspeed_spi_read_data(uint32_t ahb_addr,
		uint8_t *read_arr, uint32_t read_cnt)
{
	int i = 0;
	uint32_t dword;
	uint32_t *read_ptr = (uint32_t *)read_arr;

	if (read_arr) {
		if (((uint32_t)read_ptr & 0xf) == 0) {
			for (i = 0; i < read_cnt; i += 4) {
				if (read_cnt - i < 4)
					break;
				*read_ptr = sys_read32(ahb_addr);
				read_ptr += 1;
			}
		}

		for (; i < read_cnt;) {
			dword = sys_read32(ahb_addr);
			if (i < read_cnt)
				read_arr[i] = dword & 0xff;
			i++;
			if (i < read_cnt)
				read_arr[i] = (dword >> 8) & 0xff;
			i++;
			if (i < read_cnt)
				read_arr[i] = (dword >> 16) & 0xff;
			i++;
			if (i < read_cnt)
				read_arr[i] = (dword >> 24) & 0xff;
			i++;
		}
#if defined(DEBUG)
		LOG_INF("read count: %d", read_cnt);
		for (i = 0; i < read_cnt; i++)
			LOG_INF("[%02x]", read_arr[i]);
#endif
	}
}

static void aspeed_spi_write_data(uint32_t ahb_addr,
		const uint8_t *write_arr, uint32_t write_cnt)
{
	int i;
	uint32_t dword;

	if (write_arr) {
#if defined(DEBUG)
		LOG_INF("write count: %d", write_cnt);
		for (i = 0; i < write_cnt; i++)
			LOG_INF("[%02x]", write_arr[i]);
#endif
		for (i = 0; i < write_cnt; i += 4) {
			if ((write_cnt - i) < 4)
				break;
			dword = write_arr[i];
			dword |= write_arr[i + 1] << 8;
			dword |= write_arr[i + 2] << 16;
			dword |= write_arr[i + 3] << 24;
			sys_write32(dword, ahb_addr);
		}

		for (; i < write_cnt; i++)
			sys_write8(write_arr[i], ahb_addr);
	}
}

static void aspeed_spi_start_tx(const struct device *dev)
{
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t cs = ctx->config->slave;

	if (!spi_context_tx_buf_on(ctx) && !spi_context_rx_buf_on(ctx)) {
		spi_context_complete(ctx, 0);
		return;
	}

	/* active cs */
	sys_write32(data->cmd_mode[cs].user | ASPEED_SPI_USER_INACTIVE,
			config->ctrl_base + SPI10_CE0_CTRL + cs * 4);
	sys_write32(data->cmd_mode[cs].user,
			config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	while (ctx->tx_buf && ctx->tx_len > 0) {
		aspeed_spi_write_data(data->decode_addr[cs].start,
			ctx->tx_buf, ctx->tx_len);
		spi_context_update_tx(ctx, 1, ctx->tx_len);
	};

	k_busy_wait(2);

	while (ctx->rx_buf && ctx->rx_len > 0) {
		aspeed_spi_read_data(data->decode_addr[cs].start,
			ctx->rx_buf, ctx->rx_len);
		spi_context_update_rx(ctx, 1, ctx->rx_len);
	};

	sys_write32(data->cmd_mode[cs].user | ASPEED_SPI_USER_INACTIVE,
			config->ctrl_base + SPI10_CE0_CTRL + cs * 4);
	sys_write32(data->cmd_mode[cs].normal_read,
			config->ctrl_base + SPI10_CE0_CTRL + cs * 4);
	spi_context_complete(ctx, 0);
}

static void aspeed_spi_nor_transceive_user(const struct device *dev,
						const struct spi_config *spi_cfg,
						struct spi_nor_op_info op_info)
{
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t cs = ctx->config->slave;
	uint8_t dummy[12] = {0};

#ifdef CONFIG_SPI_MONITOR_ASPEED
	/* change internal MUX */
	if (config->mux_ctrl.master_idx != 0) {
		cs = 0;
		spim_scu_ctrl_set(config->mux_ctrl.spi_monitor_common_ctrl,
				BIT(3), (config->mux_ctrl.master_idx - 1) << 3);
		spim_scu_ctrl_set(config->mux_ctrl.spi_monitor_common_ctrl,
				0x7, config->mux_ctrl.spim_output_base + ctx->config->slave);
	}
#endif

	sys_write32(data->cmd_mode[cs].user | ASPEED_SPI_USER_INACTIVE,
		config->ctrl_base + SPI10_CE0_CTRL + cs * 4);
	sys_write32(data->cmd_mode[cs].user, config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	/* cmd */
	sys_write32(data->cmd_mode[cs].user |
		aspeed_spi_io_mode_user(JESD216_GET_CMD_BUSWIDTH(op_info.mode)),
		config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	aspeed_spi_write_data(data->decode_addr[cs].start, &op_info.opcode, 1);

	/* addr */
	sys_write32(data->cmd_mode[cs].user |
		aspeed_spi_io_mode_user(JESD216_GET_ADDR_BUSWIDTH(op_info.mode)),
		config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	if (op_info.addr_len == 3)
		op_info.addr <<= 8;

	op_info.addr = sys_cpu_to_be32(op_info.addr);

	aspeed_spi_write_data(data->decode_addr[cs].start,
		(const uint8_t *)&op_info.addr, op_info.addr_len);

	/* dummy */
	aspeed_spi_write_data(data->decode_addr[cs].start,
		(const uint8_t *)&dummy,
		(op_info.dummy_cycle / (8 / JESD216_GET_ADDR_BUSWIDTH(op_info.mode))));

	/* data */
	sys_write32(data->cmd_mode[cs].user |
			aspeed_spi_io_mode_user(JESD216_GET_DATA_BUSWIDTH(op_info.mode)),
			config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	if (op_info.data_direct == SPI_NOR_DATA_DIRECT_IN) {
		/* read data */
		aspeed_spi_read_data(data->decode_addr[cs].start,
				op_info.buf, op_info.data_len);
	} else {
		/* write data */
		aspeed_spi_write_data(data->decode_addr[cs].start,
				op_info.buf, op_info.data_len);
	}

	sys_write32(data->cmd_mode[cs].user | ASPEED_SPI_USER_INACTIVE,
		config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	sys_write32(data->cmd_mode[cs].normal_read,
		config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

#ifdef CONFIG_SPI_MONITOR_ASPEED
	if (config->mux_ctrl.master_idx != 0)
		spim_scu_ctrl_clear(config->mux_ctrl.spi_monitor_common_ctrl, 0xf);
#endif

	spi_context_complete(ctx, 0);
}

#ifdef CONFIG_SPI_DMA_SUPPORT_ASPEED
static void aspeed_dma_irq_enable(const struct device *dev)
{
	const struct aspeed_spi_config *config = dev->config;
	uint32_t reg_val;

	reg_val = sys_read32(config->ctrl_base + SPI08_INTR_CTRL);
	reg_val |= SPI_DMA_IRQ_EN;
	sys_write32(reg_val, config->ctrl_base + SPI08_INTR_CTRL);
}

void aspeed_spi_dma_isr(const void *param)
{
	const struct device *dev = param;
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *const data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t cs = ctx->config->slave;
	uint32_t reg_val;

#ifdef CONFIG_SPI_MONITOR_ASPEED
	/* change internal MUX */
	if (config->mux_ctrl.master_idx != 0)
		cs = 0;
#endif

	if (!(sys_read32(config->ctrl_base + SPI08_INTR_CTRL) & SPI_DMA_STS)) {
		LOG_ERR("Fail in ISR");
	}

	/* disable IRQ */
	reg_val = sys_read32(config->ctrl_base + SPI08_INTR_CTRL);
	reg_val &= ~SPI_DMA_IRQ_EN;
	sys_write32(reg_val, config->ctrl_base + SPI08_INTR_CTRL);

	/* disable DMA */
	sys_write32(0x0, config->ctrl_base + SPI80_DMA_CTRL);
	sys_write32(SPI_DMA_DISCARD_REQ_MAGIC, config->ctrl_base + SPI80_DMA_CTRL);

#ifdef CONFIG_SPI_MONITOR_ASPEED
	if (config->mux_ctrl.master_idx != 0)
		spim_scu_ctrl_clear(config->mux_ctrl.spi_monitor_common_ctrl, 0xf);
#endif

	sys_write32(data->cmd_mode[cs].normal_read,
		config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	spi_context_complete(ctx, 0);
}

static void aspeed_spi_read_dma(const struct device *dev,
						const struct spi_config *spi_cfg,
						struct spi_nor_op_info op_info)
{
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t cs = ctx->config->slave;
	uint32_t ctrl_reg;

#ifdef CONFIG_SPI_MONITOR_ASPEED
	/* change internal MUX */
	if (config->mux_ctrl.master_idx != 0)
		cs = 0;
#endif

	if (op_info.data_len > data->decode_addr[cs].len) {
		LOG_WRN("Inavlid read len(0x%08x, 0x%08x)",
			op_info.data_len, data->decode_addr[cs].len);
		spi_context_complete(ctx, 0);
		return;
	}

	if ((op_info.addr % 4) != 0 || ((uint32_t)(op_info.buf) % 4) != 0) {
		LOG_WRN("Address should be 4-byte aligned");
		spi_context_complete(ctx, 0);
		return;
	}

#ifdef CONFIG_SPI_MONITOR_ASPEED
	/* change internal MUX */
	if (config->mux_ctrl.master_idx != 0) {
		spim_scu_ctrl_set(config->mux_ctrl.spi_monitor_common_ctrl,
				BIT(3), (config->mux_ctrl.master_idx - 1) << 3);
		spim_scu_ctrl_set(config->mux_ctrl.spi_monitor_common_ctrl,
				0x7, config->mux_ctrl.spim_output_base + ctx->config->slave);
	}
#endif

	ctrl_reg = data->cmd_mode[cs].normal_read & SPI_CTRL_FREQ_MASK;
	/* io mode */
	ctrl_reg |= aspeed_spi_io_mode(op_info.mode);
	/* cmd */
	ctrl_reg |= ((uint32_t)op_info.opcode) << 16;
	/* dummy cycle */
	ctrl_reg |= ((uint32_t)(op_info.dummy_cycle /
				(8 / JESD216_GET_ADDR_BUSWIDTH(op_info.mode)))) << 6;
	ctrl_reg |= ASPEED_SPI_NORMAL_READ;
	sys_write32(ctrl_reg, config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	sys_write32(SPI_DMA_GET_REQ_MAGIC, config->ctrl_base + SPI80_DMA_CTRL);
	if (sys_read32(config->ctrl_base + SPI80_DMA_CTRL) & SPI_DAM_REQUEST) {
		while (!(sys_read32(config->ctrl_base + SPI80_DMA_CTRL) & SPI_DAM_GRANT))
			;
	}

	sys_write32(data->decode_addr[cs].start + op_info.addr - SPI_DMA_FLASH_MAP_BASE,
	       config->ctrl_base + SPI84_DMA_FLASH_ADDR);
	sys_write32((uint32_t)(&((uint8_t *)op_info.buf)[0]) + SPI_DMA_RAM_MAP_BASE,
			config->ctrl_base + SPI88_DMA_RAM_ADDR);
	sys_write32(op_info.data_len - 1, config->ctrl_base + SPI8C_DMA_LEN);

	aspeed_dma_irq_enable(dev);

	sys_write32(SPI_DMA_ENABLE, config->ctrl_base + SPI80_DMA_CTRL);
}

static void aspeed_spi_write_dma(const struct device *dev,
						const struct spi_config *spi_cfg,
						struct spi_nor_op_info op_info)
{
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t cs = ctx->config->slave;
	uint32_t ctrl_reg;

#ifdef CONFIG_SPI_MONITOR_ASPEED
	/* change internal MUX */
	if (config->mux_ctrl.master_idx != 0)
		cs = 0;
#endif

	if (op_info.data_len > data->decode_addr[cs].len) {
		LOG_WRN("Inavlid write len(0x%08x, 0x%08x)",
			op_info.data_len, data->decode_addr[cs].len);
		spi_context_complete(ctx, 0);
		return;
	}

	if ((op_info.addr % 4) != 0 || ((uint32_t)(op_info.buf) % 4) != 0) {
		LOG_WRN("Address should be 4-byte aligned");
		spi_context_complete(ctx, 0);
		return;
	}

#ifdef CONFIG_SPI_MONITOR_ASPEED
	/* change internal MUX */
	if (config->mux_ctrl.master_idx != 0) {
		spim_scu_ctrl_set(config->mux_ctrl.spi_monitor_common_ctrl,
				BIT(3), (config->mux_ctrl.master_idx - 1) << 3);
		spim_scu_ctrl_set(config->mux_ctrl.spi_monitor_common_ctrl,
				0x7, config->mux_ctrl.spim_output_base + ctx->config->slave);
	}
#endif

	ctrl_reg = data->cmd_mode[cs].normal_write & SPI_CTRL_FREQ_MASK;
	/* io mode */
	ctrl_reg |= aspeed_spi_io_mode(op_info.mode);
	/* cmd */
	ctrl_reg |= ((uint32_t)op_info.opcode) << 16;
	/* dummy cycle */
	ctrl_reg |= ((uint32_t)(op_info.dummy_cycle /
				(8 / JESD216_GET_ADDR_BUSWIDTH(op_info.mode)))) << 6;
	ctrl_reg |= ASPEED_SPI_NORMAL_WRITE;
	sys_write32(ctrl_reg, config->ctrl_base + SPI10_CE0_CTRL + cs * 4);

	sys_write32(SPI_DMA_GET_REQ_MAGIC, config->ctrl_base + SPI80_DMA_CTRL);
	if (sys_read32(config->ctrl_base + SPI80_DMA_CTRL) & SPI_DAM_REQUEST) {
		while (!(sys_read32(config->ctrl_base + SPI80_DMA_CTRL) & SPI_DAM_GRANT))
			;
	}

	sys_write32(data->decode_addr[cs].start + op_info.addr - SPI_DMA_FLASH_MAP_BASE,
	       config->ctrl_base + SPI84_DMA_FLASH_ADDR);
	sys_write32((uint32_t)(&((uint8_t *)op_info.buf)[0]) + SPI_DMA_RAM_MAP_BASE,
			config->ctrl_base + SPI88_DMA_RAM_ADDR);
	sys_write32(op_info.data_len - 1, config->ctrl_base + SPI8C_DMA_LEN);

	aspeed_dma_irq_enable(dev);

	sys_write32(SPI_DMA_ENABLE | SPI_DMA_WRITE, config->ctrl_base + SPI80_DMA_CTRL);
}
#endif

static int aspeed_spi_transceive(const struct device *dev,
					    const struct spi_config *spi_cfg,
					    const struct spi_buf_set *tx_bufs,
					    const struct spi_buf_set *rx_bufs)
{
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret;

	spi_context_lock(ctx, false, NULL, spi_cfg);

	if (!spi_context_configured(ctx, spi_cfg))
		ctx->config = spi_cfg;

	spi_context_buffers_setup(ctx, tx_bufs, rx_bufs, 1);

	aspeed_spi_start_tx(dev);

	ret = spi_context_wait_for_completion(ctx);

	spi_context_release(ctx, ret);

	return ret;
}

uint32_t aspeed_get_spi_freq_div(uint32_t bus_clk, uint32_t max_freq)
{
	uint32_t div_arr[16] = {15, 7, 14, 6, 13, 5, 12, 4, 11, 3, 10, 2, 9, 1, 8, 0};
	uint32_t i, j;
	bool find = false;

	for (i = 0; i < 0xf; i++) {
		for (j = 0; j < 16; j++) {
			if (i == 0 && j == 0)
				continue;

			if (max_freq >= (bus_clk / (j + 1 + (i * 16)))) {
				find = true;
				break;
			}
		}

		if (find)
			break;
	}

	if (i == 0xf && j == 16) {
		LOG_ERR("%s %d cannot get correct frequence division.", __func__, __LINE__);
		return 0;
	}

	return ((i << 24) | (div_arr[j] << 8));
}

/*
 * Check whether the data is not all 0 or 1 in order to
 * avoid calibriate umount spi-flash.
 */
static bool aspeed_spi_calibriation_enable(const uint8_t *buf, uint32_t sz)
{
	const uint32_t *buf_32 = (const uint32_t *)buf;
	uint32_t i;
	uint32_t valid_count = 0;

	for (i = 0; i < (sz / 4); i++) {
		if (buf_32[i] != 0 && buf_32[i] != 0xffffffff)
			valid_count++;
		if (valid_count > 100)
			return true;
	}

	return false;
}

static uint32_t aspeed_spi_dma_checksum(const struct device *dev,
	uint32_t div, uint32_t delay)
{
	struct aspeed_spi_data *data = dev->data;
	const struct aspeed_spi_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t ctrl_reg = config->ctrl_base;
	uint32_t ctrl_val;
	uint32_t checksum;

	sys_write32(SPI_DMA_GET_REQ_MAGIC, ctrl_reg + SPI80_DMA_CTRL);
	if (sys_read32(ctrl_reg + SPI80_DMA_CTRL) & SPI_DAM_REQUEST) {
		while (!(sys_read32(ctrl_reg + SPI80_DMA_CTRL) & SPI_DAM_GRANT))
			;
	}

	sys_write32(data->decode_addr[ctx->config->slave].start,
	       ctrl_reg + SPI84_DMA_FLASH_ADDR);
	sys_write32(SPI_CALIB_LEN, ctrl_reg + SPI8C_DMA_LEN);

	ctrl_val = SPI_DMA_ENABLE | SPI_DMA_CALC_CKSUM | SPI_DMA_CALIB_MODE |
		   (delay << 8) | ((div & 0xf) << 16);
	sys_write32(ctrl_val, ctrl_reg + SPI80_DMA_CTRL);
	while (!(sys_read32(ctrl_reg + SPI08_INTR_CTRL) & SPI_DMA_STATUS))
		;

	checksum = sys_read32(ctrl_reg + SPI90_CHECKSUM_RESULT);

	sys_write32(0x0, ctrl_reg + SPI80_DMA_CTRL);
	sys_write32(SPI_DMA_DISCARD_REQ_MAGIC, ctrl_reg + SPI80_DMA_CTRL);

	return checksum;
}

static int aspeed_get_mid_point_of_longest_one(uint8_t *buf, uint32_t len)
{
	int i;
	int start = 0, mid_point = 0;
	int max_cnt = 0, cnt = 0;

	for (i = 0; i < len; i++) {
		if (buf[i] == 1) {
			cnt++;
		} else {
			cnt = 0;
			start = i;
		}

		if (max_cnt < cnt) {
			max_cnt = cnt;
			mid_point = start + (cnt / 2);
		}
	}

	/*
	 * In order to get a stable SPI read timing,
	 * abandon the result if the length of longest
	 * consecutive good points is too short.
	 */
	if (max_cnt < 4)
		return -1;

	return mid_point;
}

void aspeed_spi_timing_calibration(const struct device *dev)
{
	struct aspeed_spi_data *data = dev->data;
	const struct aspeed_spi_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t ctrl_reg = config->ctrl_base;
	uint32_t cs = ctx->config->slave;
	uint32_t max_freq = ctx->config->frequency;
	/* HCLK/2, ..., HCKL/5 */
	uint32_t hclk_masks[] = {7, 14, 6, 13};
	uint8_t *calib_res = NULL;
	uint8_t *check_buf = NULL;
	uint32_t reg_val;
	uint32_t checksum, gold_checksum;
	uint32_t i, hcycle, delay_ns, final_delay = 0;
	uint32_t hclk_div;
	bool pass;
	int calib_point;

	reg_val =
		sys_read32(ctrl_reg + SPI94_CE0_TIMING_CTRL + cs * 4);
	if (reg_val != 0) {
		LOG_DBG("Already executed calibration.");
		goto no_calib;
	}

	if (config->mux_ctrl.master_idx != 0 && cs != 0)
		goto no_calib;

	LOG_DBG("Calculate timing compensation:");
	/*
	 * use the related low frequency to get check calibration data
	 * and get golden data.
	 */
	reg_val = sys_read32(ctrl_reg + SPI10_CE0_CTRL + cs * 4);
	reg_val &= (~SPI_CTRL_FREQ_MASK);
	sys_write32(reg_val, ctrl_reg + SPI10_CE0_CTRL + cs * 4);

	check_buf = k_malloc(SPI_CALIB_LEN);
	if (!check_buf) {
		LOG_ERR("Insufficient buffer for calibration.");
		goto no_calib;
	}

	LOG_DBG("reg_val = 0x%x", reg_val);
	memcpy(check_buf, (uint8_t *)data->decode_addr[cs].start, SPI_CALIB_LEN);

	if (!aspeed_spi_calibriation_enable(check_buf, SPI_CALIB_LEN)) {
		LOG_INF("Flash data is monotonous, skip calibration.");
		goto no_calib;
	}

	gold_checksum = aspeed_spi_dma_checksum(dev, 0, 0);

	/*
	 * allocate a space to record calibration result for
	 * different timing compensation with fixed
	 * HCLK division.
	 */
	calib_res = k_malloc(6 * 17);
	if (!calib_res) {
		LOG_ERR("Insufficient buffer for calibration result.");
		goto no_calib;
	}

	/* From HCLK/2 to HCLK/5 */
	for (i = 0; i < ARRAY_SIZE(hclk_masks); i++) {
		if (max_freq < data->hclk / (i + 2)) {
			LOG_DBG("skipping freq %d", data->hclk / (i + 2));
			continue;
		}
		max_freq = data->hclk / (i + 2);

		checksum = aspeed_spi_dma_checksum(dev, hclk_masks[i], 0);
		pass = (checksum == gold_checksum);
		LOG_DBG("HCLK/%d, no timing compensation: %s", i + 2,
			pass ? "PASS" : "FAIL");

		memset(calib_res, 0x0, 6 * 17);

		for (hcycle = 0; hcycle <= 5; hcycle++) {
			/* increase DI delay by the step of 0.5ns */
			LOG_DBG("Delay Enable : hcycle %x", hcycle);
			for (delay_ns = 0; delay_ns <= 0xf; delay_ns++) {
				checksum = aspeed_spi_dma_checksum(dev, hclk_masks[i],
					BIT(3) | hcycle | (delay_ns << 4));
				pass = (checksum == gold_checksum);
				calib_res[hcycle * 17 + delay_ns] = pass;
				LOG_DBG("HCLK/%d, %d HCLK cycle, %d delay_ns : %s",
					i + 2, hcycle, delay_ns,
					pass ? "PASS" : "FAIL");
			}
		}

		calib_point = aspeed_get_mid_point_of_longest_one(calib_res, 6 * 17);
		if (calib_point < 0) {
			LOG_INF("cannot get good calibration point.");
			continue;
		}

		hcycle = calib_point / 17;
		delay_ns = calib_point % 17;
		LOG_DBG("final hcycle: %d, delay_ns: %d", hcycle, delay_ns);

		final_delay = (BIT(3) | hcycle | (delay_ns << 4)) << (i * 8);
		sys_write32(final_delay, ctrl_reg + SPI94_CE0_TIMING_CTRL + cs * 4);
		break;
	}

no_calib:

	hclk_div = aspeed_get_spi_freq_div(data->hclk, max_freq);

	/* configure SPI clock frequency */
	reg_val = sys_read32(ctrl_reg + SPI10_CE0_CTRL + cs * 4);
	reg_val = (reg_val & (~SPI_CTRL_FREQ_MASK)) | hclk_div;
	sys_write32(reg_val, ctrl_reg + SPI10_CE0_CTRL + cs * 4);

	data->cmd_mode[cs].normal_read =
		(data->cmd_mode[cs].normal_read & (~SPI_CTRL_FREQ_MASK)) | hclk_div;

	data->cmd_mode[cs].normal_write =
		(data->cmd_mode[cs].normal_write & (~SPI_CTRL_FREQ_MASK)) | hclk_div;

	data->cmd_mode[cs].user =
		(data->cmd_mode[cs].user & (~SPI_CTRL_FREQ_MASK)) | hclk_div;

	/* add clock setting info for CE ctrl setting */
	LOG_DBG("freq: %dMHz", max_freq / 1000000);

	if (check_buf)
		k_free(check_buf);
	if (calib_res)
		k_free(calib_res);
}

static int aspeed_spi_nor_transceive(const struct device *dev,
						const struct spi_config *spi_cfg,
						struct spi_nor_op_info op_info)
{
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	int ret = 0;

	spi_context_lock(ctx, false, NULL, spi_cfg);
	if (!spi_context_configured(ctx, spi_cfg))
		ctx->config = spi_cfg;

#ifdef CONFIG_SPI_DMA_SUPPORT_ASPEED
	if (op_info.data_direct == SPI_NOR_DATA_DIRECT_IN) {
		if (op_info.data_len > SPI_DMA_TRIGGER_LEN)
			aspeed_spi_read_dma(dev, spi_cfg, op_info);
		else
			aspeed_spi_nor_transceive_user(dev, spi_cfg, op_info);
	} else if (op_info.data_direct == SPI_NOR_DATA_DIRECT_OUT) {
		if (op_info.data_len > SPI_DMA_TRIGGER_LEN)
			aspeed_spi_write_dma(dev, spi_cfg, op_info);
		else
			aspeed_spi_nor_transceive_user(dev, spi_cfg, op_info);
	}
#else
	aspeed_spi_nor_transceive_user(dev, spi_cfg, op_info);
#endif

	ret = spi_context_wait_for_completion(ctx);
	spi_context_release(ctx, ret);

	return ret;
}

static int aspeed_spi_decode_range_reinit(const struct device *dev,
						uint32_t flash_sz)
{
	uint32_t cs, tmp;
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *data = dev->data;
	struct spi_context *ctx = &data->ctx;
	uint32_t decode_sz_arr[ASPEED_MAX_CS] = {0};
	uint32_t total_decode_range = 0;
	uint32_t start_addr, end_addr, pre_end_addr = 0;

	/* record original decode range */
	for (cs = 0; cs < config->max_cs; cs++) {
		tmp = sys_read32(config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 4);
		if (tmp == 0)
			decode_sz_arr[cs] = 0;
		else
			decode_sz_arr[cs] = data->segment_end(tmp) - data->segment_start(tmp) + 1;
		LOG_DBG("ori decode range 0x%08x", decode_sz_arr[cs]);
		total_decode_range += decode_sz_arr[cs];
	}

	/* prepare new decode sz array */
	if (total_decode_range - decode_sz_arr[ctx->config->slave]
		+ flash_sz < ASPEED_SPI_SZ_256M) {
		decode_sz_arr[ctx->config->slave] = flash_sz;
	} else {
		/* do nothing, otherwise, decode range will be larger than 256MB */
		LOG_WRN("decode size out of range 0x%08x", flash_sz);
		return 0;
	}

	/* modify decode range */
	for (cs = 0; cs < config->max_cs; cs++) {
		if (decode_sz_arr[cs] == 0)
			continue;

		if (cs == 0)
			start_addr = config->spi_mmap_base;
		else
			start_addr = pre_end_addr;

		end_addr = start_addr + decode_sz_arr[cs] - 1;
		LOG_DBG("start: 0x%x end: 0x%x (0x%x)", start_addr, end_addr, decode_sz_arr[cs]);

		sys_write32(data->segment_value(start_addr, end_addr),
			config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 4);
		data->decode_addr[cs].start = start_addr;
		if (cs == ctx->config->slave)
			data->decode_addr[cs].len = flash_sz;

		pre_end_addr = end_addr + 1;
	}

	LOG_DBG("decode reg: <0x%08x, 0x%08x, 0x%08x>",
			sys_read32(config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 0),
			sys_read32(config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 4),
			sys_read32(config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 8));

	return 0;
}

static int aspeed_spi_nor_read_init(const struct device *dev,
						const struct spi_config *spi_cfg,
						struct spi_nor_op_info op_info)
{
	int ret = 0;
	struct aspeed_spi_data *data = dev->data;
	const struct aspeed_spi_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t reg_val;


	spi_context_lock(ctx, false, NULL, spi_cfg);
	if (!spi_context_configured(ctx, spi_cfg))
		ctx->config = spi_cfg;

	LOG_DBG("[%s] mode %08x, cmd: %x, dummy: %d, frequency: %d",
		__func__, op_info.mode, op_info.opcode, op_info.dummy_cycle,
		ctx->config->frequency);

	/* If internal MUX is used, don't reinit decoded address. */
	if (config->mux_ctrl.master_idx == 0) {
		ret = aspeed_spi_decode_range_reinit(dev, op_info.data_len);
		if (ret != 0)
			goto end;
	}

	data->cmd_mode[ctx->config->slave].normal_read =
		ASPEED_SPI_CTRL_VAL(aspeed_spi_io_mode(op_info.mode),
			op_info.opcode,
			aspeed_spi_cal_dummy_cycle(JESD216_GET_ADDR_BUSWIDTH(op_info.mode),
				op_info.dummy_cycle)) | ASPEED_SPI_NORMAL_READ;
	sys_write32(data->cmd_mode[ctx->config->slave].normal_read,
			config->ctrl_base + SPI10_CE0_CTRL + ctx->config->slave * 4);

	/* set controller to 4-byte mode */
	if (op_info.addr_len == 4) {
		sys_write32(sys_read32(config->ctrl_base + SPI04_CE_CTRL) |
			(0x11 << ctx->config->slave), config->ctrl_base + SPI04_CE_CTRL);
	}

	/* config for SAFS */
	if (config->ctrl_type == HOST_SPI) {
		reg_val = sys_read32(config->ctrl_base + SPIR6C_HOST_DIRECT_ACCESS_CMD_CTRL4);
		if (op_info.addr_len == 4)
			reg_val = (reg_val & 0xffff00ff) | (op_info.opcode << 8);
		else
			reg_val = (reg_val & 0xffffff00) | op_info.opcode;

		reg_val = (reg_val & 0x0fffffff) | aspeed_spi_io_mode(op_info.mode);
		sys_write32(reg_val, config->ctrl_base + SPIR6C_HOST_DIRECT_ACCESS_CMD_CTRL4);
	}

	aspeed_spi_timing_calibration(dev);

end:

	spi_context_release(ctx, ret);

	return ret;
}

static int aspeed_spi_nor_write_init(const struct device *dev,
						const struct spi_config *spi_cfg,
						struct spi_nor_op_info op_info)
{
	int ret = 0;
	struct aspeed_spi_data *data = dev->data;
	const struct aspeed_spi_config *config = dev->config;
	struct spi_context *ctx = &data->ctx;
	uint32_t reg_val;

	spi_context_lock(ctx, false, NULL, spi_cfg);

	data->cmd_mode[ctx->config->slave].normal_write =
			ASPEED_SPI_CTRL_VAL(aspeed_spi_io_mode(op_info.mode),
				op_info.opcode, 0) | ASPEED_SPI_NORMAL_WRITE;

	if (config->ctrl_type == HOST_SPI) {
		reg_val = sys_read32(config->ctrl_base + SPIR6C_HOST_DIRECT_ACCESS_CMD_CTRL4);
		reg_val = (reg_val & 0xf0ffffff) | (aspeed_spi_io_mode(op_info.mode) >> 8);
		sys_write32(reg_val, config->ctrl_base + SPIR6C_HOST_DIRECT_ACCESS_CMD_CTRL4);
		reg_val = sys_read32(config->ctrl_base + SPIR74_HOST_DIRECT_ACCESS_CMD_CTRL2);
		if (op_info.addr_len == 4)
			reg_val = (reg_val & 0xffff00ff) | (op_info.opcode << 8);
		else
			reg_val = (reg_val & 0xffffff00) | op_info.opcode;

		sys_write32(reg_val, config->ctrl_base + SPIR74_HOST_DIRECT_ACCESS_CMD_CTRL2);
	}

	spi_context_release(ctx, ret);

	return ret;

}

static int aspeed_spi_release(const struct device *dev,
				const struct spi_config *spi_cfg)
{
	struct aspeed_spi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);

	return 0;
}

void aspeed_segment_function_init(
		const struct aspeed_spi_config *config,
		struct aspeed_spi_data *data)

{
	switch (config->platform) {
	case 2600:
	case 2620:
	case 2605:
	case 2625:
		data->segment_start = ast2600_segment_addr_start;
		data->segment_end = ast2600_segment_addr_end;
		data->segment_value = ast2600_segment_addr_val;
		break;

	case 1030:
	case 1060:
		if (config->ctrl_type == BOOT_SPI) {
			data->segment_start = ast1030_fmc_segment_addr_start;
			data->segment_end = ast1030_fmc_segment_addr_end;
			data->segment_value = ast1030_fmc_segment_addr_val;
		} else {
			data->segment_start = ast1030_spi_segment_addr_start;
			data->segment_end = ast1030_spi_segment_addr_end;
			data->segment_value = ast1030_spi_segment_addr_val;
		}

		break;

	default:
		LOG_ERR("undefine ast platform %d\n", config->platform);
		__ASSERT_NO_MSG(0);

	}
}

void aspeed_decode_range_pre_init(
		const struct aspeed_spi_config *config,
		struct aspeed_spi_data *data)
{
	uint32_t cs;
	uint32_t unit_sz = ASPEED_SPI_SZ_2M; /* init 2M for each cs */
	uint32_t start_addr, end_addr, pre_end_addr = 0;
	uint32_t max_cs = config->max_cs;


	/* For PFR device, SPI controller always accesses flash by CS0. */
	if (config->mux_ctrl.master_idx != 0) {
		max_cs = 1;
		unit_sz = ASPEED_SPI_SZ_256M;
	}

	for (cs = 0; cs < max_cs; cs++) {
		if (cs == 0)
			start_addr = config->spi_mmap_base;
		else
			start_addr = pre_end_addr;

		end_addr = start_addr + unit_sz - 1;

		/* the maximum decode size is 256MB */
		if (config->spi_mmap_base + ASPEED_SPI_SZ_256M <= end_addr) {
			LOG_DBG("%s %d smc decode address overflow\n", __func__, __LINE__);
			sys_write32(0, config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 4);
			continue;
		}

		LOG_DBG("cs: %d start: 0x%08x, end: 0x%08x (%08x)\n",
			cs, start_addr, end_addr, data->segment_value(start_addr, end_addr));

		sys_write32(data->segment_value(start_addr, end_addr),
			config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 4);
		LOG_DBG("cs: %d 0x%08x\n", cs,
			sys_read32(config->ctrl_base + SPI30_CE0_ADDR_DEC + cs * 4));

		data->decode_addr[cs].start = start_addr;
		data->decode_addr[cs].len = unit_sz;
		pre_end_addr = end_addr + 1;
	}
}

static int aspeed_spi_init(const struct device *dev)
{
	const struct aspeed_spi_config *config = dev->config;
	struct aspeed_spi_data *data = dev->data;
	uint32_t cs;
	uint32_t reg_val;
	int ret;

	for (cs = 0; cs < config->max_cs; cs++) {
		reg_val = sys_read32(config->ctrl_base + SPI00_CE_TYPE_SETTING);
		sys_write32(reg_val | BIT(16 + cs), config->ctrl_base + SPI00_CE_TYPE_SETTING);

		data->cmd_mode[cs].user = ASPEED_SPI_USER;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clk_id,
			       &data->hclk);
	if (ret != 0)
		return ret;

	aspeed_segment_function_init(config, data);
	aspeed_decode_range_pre_init(config, data);

	spi_context_unlock_unconditionally(&data->ctx);

#ifdef CONFIG_SPI_DMA_SUPPORT_ASPEED
	/* irq init */
	irq_connect_dynamic(config->irq_num, config->irq_priority,
		aspeed_spi_dma_isr, dev, 0);
	irq_enable(config->irq_num);
#endif

	if (config->mux_ctrl.master_idx != 0 &&
		(config->mux_ctrl.spim_output_base == 0 ||
		 config->mux_ctrl.spi_monitor_common_ctrl == NULL)) {
		LOG_ERR("[%s]Invaild dts setting for SPI internal mux master",
				dev->name);
		return -EINVAL;
	}

	return 0;
}

static const struct spi_nor_ops aspeed_spi_nor_ops = {
	.transceive = aspeed_spi_nor_transceive,
	.read_init = aspeed_spi_nor_read_init,
	.write_init = aspeed_spi_nor_write_init,
};

static const struct spi_driver_api aspeed_spi_driver_api = {
	.transceive = aspeed_spi_transceive,
	.release = aspeed_spi_release,
	.spi_nor_op = &aspeed_spi_nor_ops,
};

#define ASPEED_SPI_INIT(n)						\
	static struct aspeed_spi_config aspeed_spi_config_##n = { \
		.ctrl_base = DT_INST_REG_ADDR_BY_NAME(n, ctrl_reg),		\
		.spi_mmap_base = DT_INST_REG_ADDR_BY_NAME(n, spi_mmap),	\
		.max_cs = DT_INST_PROP(n, num_cs),	\
		.platform = DT_INST_PROP(n, ast_platform), \
		.ctrl_type = DT_ENUM_IDX(DT_INST(n, DT_DRV_COMPAT), ctrl_type),	\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),	\
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clk_id),	\
		.irq_num = DT_INST_IRQN(n),		\
		.irq_priority = DT_INST_IRQ(n, priority),	\
		.mux_ctrl.master_idx = DT_INST_PROP_OR(n, internal_mux_master, 0),	\
		.mux_ctrl.spim_output_base = DT_INST_PROP_OR(n, spi_monitor_output_base, 0),	\
		.mux_ctrl.spi_monitor_common_ctrl =	\
			COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(n, DT_DRV_COMPAT),	\
				spi_monitor_common_ctrl),	\
			    DEVICE_DT_GET(DT_INST_PHANDLE_BY_IDX(n, spi_monitor_common_ctrl, 0)),	\
				NULL),	\
	};								\
									\
	static struct aspeed_spi_data aspeed_spi_data_##n = {	\
		SPI_CONTEXT_INIT_LOCK(aspeed_spi_data_##n, ctx),	\
		SPI_CONTEXT_INIT_SYNC(aspeed_spi_data_##n, ctx),	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &aspeed_spi_init,			\
			    NULL,					\
			    &aspeed_spi_data_##n,			\
			    &aspeed_spi_config_##n, POST_KERNEL,	\
			    75,		\
			    &aspeed_spi_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(ASPEED_SPI_INIT)
