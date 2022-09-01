/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_peci

#include <stdlib.h>
#include <errno.h>
#include <device.h>
#include <drivers/peci.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/reset_control.h>
#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
#include <portability/cmsis_os2.h>
#endif

#include "peci_aspeed.h"
#define LOG_LEVEL CONFIG_PECI_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(peci_aspeed);


#define ASPEED_PECI_INT_MASK  GENMASK(4, 0)

struct peci_aspeed_config {
	peci_register_t *base;
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	const reset_control_subsys_t rst_id;
	bool byte_mode_64;
	uint8_t rd_sampling_point;
	uint32_t cmd_timeout_ms;
};

struct peci_aspeed_data {
#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
	osEventFlagsId_t evt_id;
#endif
	uint32_t clk_src;
	uint32_t freq;
};

/* Driver convenience defines */
#define DEV_CFG(dev) ((const struct peci_aspeed_config *)(dev)->config)
#define DEV_DATA(dev) ((struct peci_aspeed_data *)(dev)->data)
#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
static void peci_aspeed_isr(const struct device *dev)
{
	struct peci_aspeed_data *peci_data = DEV_DATA(dev);
	peci_register_t *peci_register = DEV_CFG(dev)->base;
	uint32_t int_index = 0, int_pending;

	int_pending = peci_register->interrupt_status.value & ASPEED_PECI_INT_MASK;
	do {
		if (int_pending & 0x1) {
			peci_register->interrupt_status.value = BIT(int_index);
			peci_register->interrupt_status.value;
			LOG_DBG("int_pedding = 0x%08x\n", (uint32_t)BIT(int_index));
			osEventFlagsSet(peci_data->evt_id, BIT(int_index));
		}
		int_index++;
		int_pending >>= 1;
	} while (int_pending);
}
#endif

static int peci_aspeed_init(const struct device *dev)
{
	LOG_DBG("%s", __FUNCTION__);
	peci_register_t *peci_register = DEV_CFG(dev)->base;
	const struct device *reset_dev = device_get_binding(ASPEED_RST_CTRL_NAME);
	control_register_t control;

#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
	DEV_DATA(dev)->evt_id = osEventFlagsNew(NULL);
#endif
	clock_control_get_rate(DEV_CFG(dev)->clock_dev, DEV_CFG(dev)->clk_id,
			       &DEV_DATA(dev)->clk_src);
	/* Reset PECI interface */
	reset_control_assert(reset_dev, DEV_CFG(dev)->rst_id);
	reset_control_deassert(reset_dev, DEV_CFG(dev)->rst_id);
	k_busy_wait(1000);

	/* Read sampling point and byte mode setting */
	control.value = peci_register->control.value;
	control.fields.read_sampling_point_selection = DEV_CFG(dev)->rd_sampling_point;
	control.fields.enable_64_byte_mode = DEV_CFG(dev)->byte_mode_64;
	peci_register->control.value = control.value;

#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
	/* Disable interrupts */
	peci_register->interrupt.value &= ~(ASPEED_PECI_INT_MASK);
	/* Clear interrupts */
	peci_register->interrupt_status.value = ASPEED_PECI_INT_MASK;
	/* Direct NVIC */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    peci_aspeed_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
#endif
	return 0;
}

static int peci_aspeed_configure(const struct device *dev, uint32_t bitrate)
{
	peci_register_t *peci_register = DEV_CFG(dev)->base;
	timing_negotiation_register_t timing_negotiation;
	control_register_t control;
	uint32_t freq = bitrate * 1000;

	uint32_t msg_timing = 1, addr_timing = 1;
	uint32_t clk_div_val = 0;
	uint32_t msg_timing_idx, clk_div_val_idx;
	uint32_t clk_divisor, clk_divisor_tmp;
	uint32_t bus_clk_rate;
	int delta_value, delta_tmp;

	if (freq > ASPEED_PECI_BUS_FREQ_MAX || freq < ASPEED_PECI_BUS_FREQ_MIN) {
		LOG_WRN("Invalid clock-frequency : %u, Use default : %u\n", freq,
			ASPEED_PECI_BUS_FREQ_DEFAULT);
		freq = ASPEED_PECI_BUS_FREQ_DEFAULT;
	}
	/*
	 * PECI bus clock = (Bus clk rate) / (1 << PECI00[10:8])
	 * PECI operation clock = (PECI bus clock)/ 4*(PECI04[15:8]*4+1)
	 * (1 << PECI00[10:8]) * (PECI04[15:8]*4+1) =
	 * (Bus clk rate) / (4 * PECI operation clock)
	 */
	bus_clk_rate = DEV_DATA(dev)->clk_src;
	clk_divisor = bus_clk_rate / (4 * freq);
	delta_value = clk_divisor;
	/* Find the closest divisor for clock-frequency */
	for (clk_div_val_idx = 0; clk_div_val_idx < 7; clk_div_val_idx++) {
		msg_timing_idx = ((clk_divisor >> clk_div_val_idx) - 1) >> 2;
		if (msg_timing_idx >= 1 && msg_timing_idx <= 255) {
			clk_divisor_tmp = (1 << clk_div_val_idx) * (msg_timing_idx * 4 + 1);
			delta_tmp = abs(clk_divisor - clk_divisor_tmp);
			if (delta_tmp < delta_value) {
				delta_value = delta_tmp;
				msg_timing = msg_timing_idx;
				clk_div_val = clk_div_val_idx;
			}
		}
	}
	addr_timing = msg_timing;
	DEV_DATA(dev)->freq =
		(bus_clk_rate >> (2 + clk_div_val)) / (msg_timing * 4 + 1);
	LOG_DBG("clk_div = %d, Timing Negotiation = %d\n", clk_div_val,
		msg_timing);
	LOG_DBG("Expect freq: %d Real freq is about: %d\n", freq,
		DEV_DATA(dev)->freq);
	/*
	 * Timing negotiation period setting.
	 * The unit of the programmed value is 4 times of PECI clock period.
	 * TN first bit 0 will run PECI04[7:0] clock setting.
	 * TN second bit 0 will run PECI04[15:8] clock setting.
	 * After that all of the signal will follow this rate tbit-a == tbit-m.
	 */
	timing_negotiation.value = peci_register->timing_negotiation.value;
	timing_negotiation.fields.tbit_a_2_and_tbit_m = msg_timing;
	timing_negotiation.fields.tbit_a_1 = addr_timing;
	peci_register->timing_negotiation.value = timing_negotiation.value;

	control.value = peci_register->control.value;
	control.fields.clock_source_selection = ASPEED_PECI_CLK_SOURCE_HCLK;
	control.fields.peci_clock_divider = clk_div_val;
	peci_register->control.value =  control.value;

	return 0;
}

static int peci_aspeed_disable(const struct device *dev)
{
	peci_register_t *peci_register = DEV_CFG(dev)->base;
	control_register_t control;

	/* PECI disable */
	control.value = peci_register->control.value;
	control.fields.enable_peci_clock = 0;
	control.fields.enable_peci = 0;
	peci_register->control.value = control.value;

#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
	/* Disable interrupts */
	peci_register->interrupt.value &= ~(ASPEED_PECI_INT_MASK);
#endif
	return 0;
}

static int peci_aspeed_enable(const struct device *dev)
{
	peci_register_t *peci_register = DEV_CFG(dev)->base;
	control_register_t control;

	/* PECI enable */
	control.value = peci_register->control.value;
	control.fields.enable_peci_clock = 1;
	control.fields.enable_peci = 1;
	peci_register->control.value = control.value;

#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
	/* Enable interrupts */
	peci_register->interrupt.value |= ASPEED_PECI_INT_MASK;
#endif
	return 0;
}

static int peci_aspeed_need_aw_fcs(enum peci_command_code cmd_code)
{
	switch (cmd_code) {
	case PECI_CMD_WR_PCI_CFG0:
	case PECI_CMD_WR_PCI_CFG1:
	case PECI_CMD_WR_PKG_CFG0:
	case PECI_CMD_WR_PKG_CFG1:
	case PECI_CMD_WR_IAMSR0:
	case PECI_CMD_WR_IAMSR1:
	case PECI_CMD_WR_PCI_CFG_LOCAL0:
	case PECI_CMD_WR_PCI_CFG_LOCAL1:
		return 1;
	default:
		return 0;
	}
}

static int peci_aspeed_transfer(const struct device *dev, struct peci_msg *msg)
{
	struct peci_aspeed_data *peci_data = DEV_DATA(dev);
	peci_register_t *peci_register = DEV_CFG(dev)->base;
	read_write_length_register_t peci_header;
	uint32_t tx_len, rx_len;
	uint32_t reg_index, byte_index;
	write_data_register_t write_val;
	uint32_t status;

	status = peci_register->command.value & ~(0x1);
	if (status != 0) {
		LOG_ERR("peci status = %x\n", status);
		return -EBUSY;
	}
	LOG_DBG("msg header addr= 0x%08x, tx_len = 0x%08x, rx_len = 0x%08x , cmd_code = 0x%08x, flag = 0x%08x\n",
		msg->addr, msg->tx_buffer.len, msg->rx_buffer.len,
		msg->cmd_code, msg->flags);
	/* set peci header */
	peci_header.value = peci_register->peci_header.value;
	peci_header.fields.target_address = msg->addr;
	peci_header.fields.enable_aw_fcs_cycle = 0;
	peci_header.fields.write_data_length = msg->tx_buffer.len;
	peci_header.fields.read_data_length = msg->rx_buffer.len;
	if (peci_aspeed_need_aw_fcs(msg->cmd_code)) {
		/* hardware will auto replace the last data with awfcs */
		peci_header.fields.enable_aw_fcs_cycle = 1;
	}
	peci_register->peci_header.value = peci_header.value;
	if (msg->tx_buffer.len) {
		/* set write data */
		LOG_DBG("tx data:\n");
		for (tx_len = 0; tx_len < msg->tx_buffer.len; tx_len++) {
			byte_index = tx_len & 0x3;
			if (tx_len == 0) {
				write_val.data[byte_index] = msg->cmd_code;
			} else {
				write_val.data[byte_index] = msg->tx_buffer.buf[tx_len - 1];
			}
			LOG_DBG("%x\n", write_val.value);
			if (DEV_CFG(dev)->byte_mode_64) {
				reg_index = tx_len >> 2;
				peci_register->write_data_64_mode[reg_index].value =
					write_val.value;
			} else {
				reg_index = (tx_len & 0xf) >> 2;
				if (tx_len <= 15) {
					peci_register->write_data_32_mode_low[reg_index].value =
						write_val.value;
				} else {
					peci_register->write_data_32_mode_up[reg_index].value =
						write_val.value;
				}
			}
		}
	}
	/* Toggle command fire */
	peci_register->command.value |= BIT(0);
#ifdef CONFIG_PECI_ASPEED_INTERRUPT_DRIVEN
	int ret;
	ret = osEventFlagsWait(peci_data->evt_id, BIT(PECI_INT_CMD_DONE), osFlagsWaitAll,
			       DEV_CFG(dev)->cmd_timeout_ms);
	peci_register->command.value &= ~BIT(0);
	if (ret != BIT(PECI_INT_CMD_DONE)) {
		if (ret < 0) {
			if (ret == osFlagsErrorTimeout) {
				osEventFlagsDelete(peci_data->evt_id);
				peci_aspeed_init(dev);
				peci_aspeed_configure(dev,
						      DEV_DATA(dev)->freq / 1000);
				peci_aspeed_enable(dev);
				return -ETIMEDOUT;
			}
			LOG_ERR("osError: %d\n", ret);
		} else {
			osEventFlagsClear(peci_data->evt_id, ret);
			if (ret & BIT(PECI_INT_W_FCS_ABORT)) {
				LOG_ERR("FCS Abort\n");
				return -EOPNOTSUPP;
			} else if (ret & BIT(PECI_INT_W_FCS_BAD)) {
				LOG_ERR("FCS Bad\n");
				return -EILSEQ;
			}
		}
		LOG_ERR("No valid response: 0x%08x!\n", ret);
		return -EIO;
	}
#else
	while (!peci_register->interrupt_status.fields.peci_done_interrupt_status)
		;
	peci_register->interrupt_status.value |= BIT(PECI_INT_CMD_DONE);
#endif
	LOG_DBG("rx data:\n");
	/* get read data */
	for (rx_len = 0; rx_len < msg->rx_buffer.len; rx_len++) {
		byte_index = rx_len & 0x3;
		if (DEV_CFG(dev)->byte_mode_64) {
			reg_index = rx_len >> 2;
			msg->rx_buffer.buf[rx_len] =
				peci_register->read_data_64_mode[reg_index].data[byte_index];
		} else {
			reg_index = (rx_len & 0xf) >> 2;
			if (rx_len <= 15) {
				msg->rx_buffer.buf[rx_len] =
					peci_register->read_data_32_mode_low[reg_index]
					.data[byte_index];
			} else {
				msg->rx_buffer.buf[rx_len] =
					peci_register->read_data_32_mode_up[reg_index]
					.data[byte_index];
			}
		}
		LOG_DBG("%x\n", msg->rx_buffer.buf[rx_len]);
	}
	msg->rx_buffer.buf[msg->rx_buffer.len] =
		peci_register->captured_fcs_data.fields.captured_read_fcs;
	return 0;
}

static const struct peci_driver_api peci_aspeed_driver = {
	.config = peci_aspeed_configure,
	.enable = peci_aspeed_enable,
	.disable = peci_aspeed_disable,
	.transfer = peci_aspeed_transfer,
};

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error "Not yet to support two peci controller"
#endif

#define ASPEED_PECI_DEVICE_INIT(inst)						     \
	static const struct peci_aspeed_config peci_aspeed_cfg_##inst = {	     \
		.base = (peci_register_t *)DT_INST_REG_ADDR(inst),		     \
		.clock_dev =  DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),		     \
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clk_id), \
		.rst_id = (reset_control_subsys_t)DT_INST_RESETS_CELL(inst, rst_id), \
		.rd_sampling_point = DT_INST_PROP(inst, rd_sampling_point),	     \
		.byte_mode_64 = DT_INST_PROP(inst, byte_mode_64),		     \
		.cmd_timeout_ms = DT_INST_PROP(inst, cmd_timeout_ms),		     \
	};									     \
										     \
	static struct peci_aspeed_data peci_aspeed_data_##inst;			     \
										     \
	DEVICE_DT_INST_DEFINE(inst, peci_aspeed_init, NULL,			     \
			      &peci_aspeed_data_##inst,				     \
			      &peci_aspeed_cfg_##inst, POST_KERNEL,		     \
			      CONFIG_PECI_INIT_PRIORITY,			     \
			      &peci_aspeed_driver);

DT_INST_FOREACH_STATUS_OKAY(ASPEED_PECI_DEVICE_INIT)