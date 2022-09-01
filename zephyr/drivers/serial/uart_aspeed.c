/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_uart

#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
#include <drivers/uart.h>
#include <drivers/clock_control.h>
LOG_MODULE_REGISTER(uart, LOG_LEVEL_ERR);

/* UART registers */
#define UART_THR 0x00
#define UART_RBR 0x00
#define UART_DLL 0x00
#define UART_DLH 0x04
#define UART_IER 0x04
#define UART_IIR 0x08
#define UART_FCR 0x08
#define UART_LCR 0x0c
#define UART_MCR 0x10
#define UART_LSR 0x14
#define UART_MSR 0x18

/* UART_IER */
#define UART_IER_EDSSI	BIT(3)
#define UART_IER_ELSI	BIT(2)
#define UART_IER_ETBEI	BIT(1)
#define UART_IER_ERBFI	BIT(0)

/* UART_IIR */
#define UART_IIR_AI_MASK	GENMASK(3, 1)
#define UART_IIR_AI_SHIFT	1
#define UART_IIR_IPN		BIT(0)

enum UART_IIR_ACTIVE_INT {
	UART_IIR_AI_MS,
	UART_IIR_AI_THRE,
	UART_IIR_AI_RDA,
	UART_IIR_AI_RLS,
	UART_IIR_AI_TMOUT = 0x6,
};

/* UART_FCR */
#define UART_FCR_TRIG_MASK      GENMASK(7, 6)
#define UART_FCR_TRIG_SHIFT     6
#define UART_FCR_TX_RST         BIT(2)
#define UART_FCR_RX_RST         BIT(1)
#define UART_FCR_EN             BIT(0)

/* UART_LCR */
#define UART_LCR_DLAB           BIT(7)
#define UART_LCR_PARITY_MODE    BIT(4)
#define UART_LCR_PARITY_EN      BIT(3)
#define UART_LCR_STOP           BIT(2)
#define UART_LCR_CLS_MASK       GENMASK(1, 0)
#define UART_LCR_CLS_SHIFT      0

/* UART_LSR */
#define UART_LSR_TEMT   BIT(6)
#define UART_LSR_THRE   BIT(5)
#define UART_LSR_BI     BIT(4)
#define UART_LSR_FE     BIT(3)
#define UART_LSR_PE     BIT(2)
#define UART_LSR_OE     BIT(1)
#define UART_LSR_DR     BIT(0)

/* VUART registers */
#define VUART_GCRA      0x20
#define VUART_GCRB      0x24
#define VUART_ADDRL     0x28
#define VUART_ADDRH     0x2c

/* VUART_GCRA */
#define VUART_GCRA_DISABLE_HOST_TX_DISCARD      BIT(5)
#define VUART_GCRA_SIRQ_POLARITY                BIT(1)
#define VUART_GCRA_VUART_EN                     BIT(0)

/* VUART_GCRB */
#define VUART_GCRB_HOST_SIRQ_MASK               GENMASK(7, 4)
#define VUART_GCRB_HOST_SIRQ_SHIFT              4

/* UDMA registers */
#define UDMA_TX_DMA_EN          0x00
#define UDMA_RX_DMA_EN          0x04
#define UDMA_MISC               0x08
#define UDMA_TIMEOUT_TIMER      0x0c
#define UDMA_TX_DMA_RST         0x20
#define UDMA_RX_DMA_RST         0x24
#define UDMA_TX_DMA_INT_EN	0x30
#define UDMA_TX_DMA_INT_STS	0x34
#define UDMA_RX_DMA_INT_EN	0x38
#define UDMA_RX_DMA_INT_STS	0x3c
#define UDMA_CHX_OFF(x)         ((x) * 0x20)
#define UDMA_CHX_TX_RD_PTR(x)   (0x40 + UDMA_CHX_OFF(x))
#define UDMA_CHX_TX_WR_PTR(x)   (0x44 + UDMA_CHX_OFF(x))
#define UDMA_CHX_TX_BUF_BASE(x) (0x48 + UDMA_CHX_OFF(x))
#define UDMA_CHX_TX_CTRL(x)     (0x4c + UDMA_CHX_OFF(x))
#define UDMA_CHX_RX_RD_PTR(x)   (0x50 + UDMA_CHX_OFF(x))
#define UDMA_CHX_RX_WR_PTR(x)   (0x54 + UDMA_CHX_OFF(x))
#define UDMA_CHX_RX_BUF_BASE(x) (0x58 + UDMA_CHX_OFF(x))
#define UDMA_CHX_RX_CTRL(x)     (0x5c + UDMA_CHX_OFF(x))

/* UDMA_MISC */
#define UDMA_MISC_RX_BUFSZ_MASK         GENMASK(3, 2)
#define UDMA_MISC_RX_BUFSZ_SHIFT        2
#define UDMA_MISC_TX_BUFSZ_MASK         GENMASK(1, 0)
#define UDMA_MISC_TX_BUFSZ_SHIFT        0

/* UDMA_CHX_TX_CTRL */
#define UDMA_TX_CTRL_TMOUT_DISABLE      BIT(4)
#define UDMA_TX_CTRL_BUFSZ_MASK         GENMASK(3, 0)
#define UDMA_TX_CTRL_BUFSZ_SHIFT        0

/* UDMA_CHX_RX_CTRL */
#define UDMA_RX_CTRL_TMOUT_DISABLE      BIT(4)
#define UDMA_RX_CTRL_BUFSZ_MASK         GENMASK(3, 0)
#define UDMA_RX_CTRL_BUFSZ_SHIFT        0

enum udma_buffer_size_code {
	UDMA_BUFSZ_1KB,
	UDMA_BUFSZ_4KB,
	UDMA_BUFSZ_16KB,
	UDMA_BUFSZ_64KB,

	/* supported only for VUART */
	UDMA_BUFSZ_128KB,
	UDMA_BUFSZ_256KB,
	UDMA_BUFSZ_512KB,
	UDMA_BUFSZ_1024KB,
	UDMA_BUFSZ_2048KB,
	UDMA_BUFSZ_4096KB,
	UDMA_BUFSZ_8192KB,
	UDMA_BUFSZ_16384KB,
};

#define UDMA_MAX_CHANNEL        14
#define UDMA_TX_RBSZ            0x400
#define UDMA_RX_RBSZ            0x400

/* UART DMA */
static uintptr_t udma_base = DT_REG_ADDR_BY_IDX(DT_INST(0, aspeed_udma), 0);
static bool udma_init;
static const struct device *udma_udev[UDMA_MAX_CHANNEL];
static uint8_t udma_tx_rb[UDMA_MAX_CHANNEL][UDMA_TX_RBSZ] NON_CACHED_BSS_ALIGN16;
static uint8_t udma_rx_rb[UDMA_MAX_CHANNEL][UDMA_RX_RBSZ] NON_CACHED_BSS_ALIGN16;
static struct k_spinlock udma_lock;

struct uart_aspeed_config {
	uint32_t dev_idx;

	uintptr_t base;
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;

	bool virt;
	uint32_t virt_port;
	uint32_t virt_sirq;
	uint32_t virt_sirq_pol;

	bool dma;
	uint32_t dma_ch;

	void (*irq_config_func)(const struct device *dev);
};

struct uart_aspeed_data {
	struct k_spinlock lock;

	struct uart_config uart_cfg;

	uint8_t *tx_rb;
	uintptr_t tx_rb_addr;
	uint8_t *rx_rb;
	uintptr_t rx_rb_addr;

	uart_irq_callback_user_data_t cb;
	void *cb_data;

	uint32_t iir_cache;
};

static int uart_aspeed_poll_in(const struct device *dev, unsigned char *c)
{
	int rc = -1;
	uint32_t rptr, wptr;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (dev_cfg->dma) {
		rptr = sys_read32(udma_base + UDMA_CHX_RX_RD_PTR(dev_cfg->dma_ch));
		wptr = sys_read32(udma_base + UDMA_CHX_RX_WR_PTR(dev_cfg->dma_ch));

		if (rptr != wptr) {
			*c = data->rx_rb[rptr];
			rc = 0;
			sys_write32((rptr + 1) % UDMA_RX_RBSZ, udma_base + UDMA_CHX_RX_RD_PTR(dev_cfg->dma_ch));
		}
	} else {
		if (sys_read32(dev_cfg->base + UART_LSR) & UART_LSR_DR) {
			*c = (unsigned char)sys_read32(dev_cfg->base + UART_RBR);
			rc = 0;
		}
	}

	k_spin_unlock(&data->lock, key);

	return rc;
}

static void uart_aspeed_poll_out(const struct device *dev,
				 unsigned char c)
{
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	uint32_t rptr, wptr;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (dev_cfg->dma) {
		do {
			rptr = sys_read32(udma_base + UDMA_CHX_TX_RD_PTR(dev_cfg->dma_ch));
			wptr = sys_read32(udma_base + UDMA_CHX_TX_WR_PTR(dev_cfg->dma_ch));
		} while (((wptr + 1) % UDMA_TX_RBSZ) == rptr);

		data->tx_rb[wptr] = c;
		sys_write32((wptr + 1) % UDMA_TX_RBSZ, udma_base + UDMA_CHX_TX_WR_PTR(dev_cfg->dma_ch));
	} else {
		while (!(sys_read32(dev_cfg->base + UART_LSR) & UART_LSR_THRE))
			;
		sys_write32(c, dev_cfg->base + UART_THR);
	}

	k_spin_unlock(&data->lock, key);
}

static int uart_aspeed_err_check(const struct device *dev)
{
	int check;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	check = sys_read32(dev_cfg->base + UART_LSR) &
		(UART_LSR_BI | UART_LSR_FE | UART_LSR_PE | UART_LSR_OE | UART_LSR_DR);

	k_spin_unlock(&data->lock, key);

	return (check >> 1);
}

static int uart_aspeed_configure(const struct device *dev,
				 const struct uart_config *uart_cfg)
{
	int rc = 0;
	uint32_t reg;
	uint32_t clk_rate, divisor;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* set divisor for baudrate */
	clock_control_get_rate(dev_cfg->clock_dev,
			       dev_cfg->clk_id, &clk_rate);
	divisor = clk_rate / (16 * uart_cfg->baudrate);

	reg = sys_read32(dev_cfg->base + UART_LCR);
	reg |= UART_LCR_DLAB;
	sys_write32(reg, dev_cfg->base + UART_LCR);

	sys_write32(divisor & 0xf, dev_cfg->base + UART_DLL);
	sys_write32(divisor >> 8, dev_cfg->base + UART_DLH);

	reg &= ~(UART_LCR_DLAB | UART_LCR_CLS_MASK | UART_LCR_STOP);

	switch (uart_cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		reg |= ((0x0 << UART_LCR_CLS_SHIFT) & UART_LCR_CLS_MASK);
		break;
	case UART_CFG_DATA_BITS_6:
		reg |= ((0x1 << UART_LCR_CLS_SHIFT) & UART_LCR_CLS_MASK);
		break;
	case UART_CFG_DATA_BITS_7:
		reg |= ((0x2 << UART_LCR_CLS_SHIFT) & UART_LCR_CLS_MASK);
		break;
	case UART_CFG_DATA_BITS_8:
		reg |= ((0x3 << UART_LCR_CLS_SHIFT) & UART_LCR_CLS_MASK);
		break;
	default:
		rc = -ENOTSUP;
		goto out;
	}

	switch (uart_cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		reg &= ~(UART_LCR_STOP);
		break;
	case UART_CFG_STOP_BITS_2:
		reg |= UART_LCR_STOP;
		break;
	default:
		rc = -ENOTSUP;
		goto out;
	}

	switch (uart_cfg->parity) {
	case UART_CFG_PARITY_NONE:
		reg &= ~(UART_LCR_PARITY_EN);
		break;
	case UART_CFG_PARITY_ODD:
		reg |= UART_LCR_PARITY_EN;
		break;
	case UART_CFG_PARITY_EVEN:
		reg |= (UART_LCR_PARITY_EN | UART_LCR_PARITY_MODE);
		break;
	default:
		rc = -ENOTSUP;
		goto out;
	}

	sys_write32(reg, dev_cfg->base + UART_LCR);

	/*
	 * enable FIFO, generate the interrupt at 8th byte
	 */
	reg = ((0x2 << UART_FCR_TRIG_SHIFT) & UART_FCR_TRIG_MASK) |
	      UART_FCR_TX_RST |
	      UART_FCR_RX_RST |
	      UART_FCR_EN;
	sys_write32(reg, dev_cfg->base + UART_FCR);

	data->uart_cfg = *uart_cfg;
out:
	k_spin_unlock(&data->lock, key);

	return rc;
};

static int uart_aspeed_config_get(const struct device *dev,
				  struct uart_config *uart_cfg)
{
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;

	uart_cfg->baudrate = data->uart_cfg.baudrate;
	uart_cfg->parity = data->uart_cfg.parity;
	uart_cfg->stop_bits = data->uart_cfg.stop_bits;
	uart_cfg->data_bits = data->uart_cfg.data_bits;
	uart_cfg->flow_ctrl = data->uart_cfg.flow_ctrl;

	return 0;
}

static int uart_aspeed_fifo_fill(const struct device *dev,
				  const uint8_t *tx_data,
				  int size)
{
	int i;
	uint32_t rptr = 0, wptr = 0;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		rptr = sys_read32(udma_base + UDMA_CHX_TX_RD_PTR(dev_cfg->dma_ch));
		wptr = sys_read32(udma_base + UDMA_CHX_TX_WR_PTR(dev_cfg->dma_ch));

		for (i = 0; i < size; ++i) {
			if (((wptr + 1) % UDMA_TX_RBSZ) == rptr)
				break;

			data->tx_rb[wptr] = tx_data[i];
			wptr = (wptr + 1) % UDMA_TX_RBSZ;
		}

		if (i)
			sys_write32(wptr, udma_base + UDMA_CHX_TX_WR_PTR(dev_cfg->dma_ch));

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		for (i = 0; i < size; ++i) {
			if (!(sys_read32(dev_cfg->base + UART_LSR) & UART_LSR_THRE))
				break;

			sys_write32(tx_data[i], dev_cfg->base + UART_THR);
		}

		k_spin_unlock(&data->lock, key);
	}

	return i;
}

static int uart_aspeed_fifo_read(const struct device *dev, uint8_t *rx_data,
				  const int size)
{
	int i;
	uint32_t rptr = 0, wptr = 0;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		rptr = sys_read32(udma_base + UDMA_CHX_RX_RD_PTR(dev_cfg->dma_ch));
		wptr = sys_read32(udma_base + UDMA_CHX_RX_WR_PTR(dev_cfg->dma_ch));

		for (i = 0; i < size; ++i) {
			if (rptr == wptr)
				break;

			rx_data[i] = data->rx_rb[rptr];
			rptr = (rptr + 1) % UDMA_RX_RBSZ;
		}

		if (i)
			sys_write32(rptr, udma_base + UDMA_CHX_RX_RD_PTR(dev_cfg->dma_ch));

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		for (i = 0; i < size; ++i) {
			if (!(sys_read32(dev_cfg->base + UART_LSR) & UART_LSR_DR))
				break;

			rx_data[i] = (uint8_t)sys_read32(dev_cfg->base + UART_RBR);
		}

		k_spin_unlock(&data->lock, key);
	}

	return i;
}

static void uart_aspeed_irq_tx_enable(const struct device *dev)
{
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		reg = sys_read32(udma_base + UDMA_TX_DMA_INT_EN) | (0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_TX_DMA_INT_EN);

		k_spin_unlock(&udma_lock, key);

		/*
		 * The generic ns16550 device continuously generates interrupts
		 * if the transmitter is empty and the corresponding interrupt
		 * control is enabled. However, ASPEED UDMA generates interrupt
		 * once only when the transmitter status changes from non-empty
		 * to empty.
		 *
		 * The difference cause the upper layer Shell application has
		 * different reaction to interactive console input. Uncomment
		 * the following lines to mimic the ns16550 interrupt behavior
		 * for Shell implementation.
		 *
		 * NOTE that this is dedicated only to the combination using
		 * Zephyr shell with UDMA. It is NOT recommended to apply UDMA
		 * on interactive shell though.
		 *
		 * if (uart_aspeed_irq_tx_complete(dev))
		 *	data->cb(dev, data->cb_data);
		 */
	} else {
		key = k_spin_lock(&data->lock);

		reg = sys_read32(dev_cfg->base + UART_IER) | UART_IER_ETBEI;
		sys_write32(reg, dev_cfg->base + UART_IER);

		k_spin_unlock(&data->lock, key);
	}
}

static void uart_aspeed_irq_tx_disable(const struct device *dev)
{
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		reg = sys_read32(udma_base + UDMA_TX_DMA_INT_EN) & ~(0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_TX_DMA_INT_EN);

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		reg = sys_read32(dev_cfg->base + UART_IER) & ~UART_IER_ETBEI;
		sys_write32(reg, dev_cfg->base + UART_IER);

		k_spin_unlock(&data->lock, key);
	}
}

static int uart_aspeed_irq_tx_ready(const struct device *dev)
{
	int ret;
	uint32_t iir_ai;
	uint32_t sts, rptr, wptr;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		sts = sys_read32(udma_base + UDMA_TX_DMA_INT_STS);
		rptr = sys_read32(udma_base + UDMA_CHX_TX_RD_PTR(dev_cfg->dma_ch));
		wptr = sys_read32(udma_base + UDMA_CHX_TX_WR_PTR(dev_cfg->dma_ch));
		ret = ((sts & (0x1 << dev_cfg->dma_ch)) ? 1 : 0) | (rptr == wptr);

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		iir_ai = (data->iir_cache & UART_IIR_AI_MASK) >> UART_IIR_AI_SHIFT;
		ret = (iir_ai == UART_IIR_AI_THRE) ? 1 : 0;

		k_spin_unlock(&data->lock, key);
	}

	return ret;
}

static int uart_aspeed_irq_tx_complete(const struct device *dev)
{
	int ret;
	uint32_t lsr;
	uint32_t rptr, wptr;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		rptr = sys_read32(udma_base + UDMA_CHX_TX_RD_PTR(dev_cfg->dma_ch));
		wptr = sys_read32(udma_base + UDMA_CHX_TX_WR_PTR(dev_cfg->dma_ch));
		ret = (rptr == wptr) ? 1 : 0;

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		lsr = sys_read32(dev_cfg->base + UART_LSR);
		ret = (lsr & (UART_LSR_TEMT | UART_LSR_THRE)) ==
		       (UART_LSR_TEMT | UART_LSR_THRE) ? 1 : 0;

		k_spin_unlock(&data->lock, key);
	}

	return ret;
}

static void uart_aspeed_irq_rx_enable(const struct device *dev)
{
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		reg = sys_read32(udma_base + UDMA_RX_DMA_INT_EN) | (0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_RX_DMA_INT_EN);

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		reg = sys_read32(dev_cfg->base + UART_IER) | UART_IER_ERBFI;
		sys_write32(reg, dev_cfg->base + UART_IER);

		k_spin_unlock(&data->lock, key);
	}
}

static void uart_aspeed_irq_rx_disable(const struct device *dev)
{
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma) {
		key = k_spin_lock(&udma_lock);

		reg = sys_read32(udma_base + UDMA_RX_DMA_INT_EN) & ~(0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_RX_DMA_INT_EN);

		k_spin_unlock(&udma_lock, key);
	} else {
		key = k_spin_lock(&data->lock);

		reg = sys_read32(dev_cfg->base + UART_IER) & ~UART_IER_ERBFI;
		sys_write32(reg, dev_cfg->base + UART_IER);

		k_spin_unlock(&data->lock, key);
	}
}

static int uart_aspeed_irq_rx_ready(const struct device *dev)
{
	int ret;
	uint32_t iir_ai, sts;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (dev_cfg->dma) {
		sts = sys_read32(udma_base + UDMA_RX_DMA_INT_STS);
		ret = (sts & (0x1 << dev_cfg->dma_ch)) ? 1 : 0;
	} else {
		iir_ai = (data->iir_cache & UART_IIR_AI_MASK) >> UART_IIR_AI_SHIFT;
		ret = (iir_ai == UART_IIR_AI_RDA || iir_ai == UART_IIR_AI_TMOUT) ? 1 : 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static void uart_aspeed_irq_err_enable(const struct device *dev)
{
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma)
		return;

	key = k_spin_lock(&data->lock);

	reg = sys_read32(dev_cfg->base + UART_IER) | UART_IER_ELSI;
	sys_write32(reg, dev_cfg->base + UART_IER);

	k_spin_unlock(&data->lock, key);
}

static void uart_aspeed_irq_err_disable(const struct device *dev)
{
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma)
		return;

	key = k_spin_lock(&data->lock);

	reg = sys_read32(dev_cfg->base + UART_IER) & ~UART_IER_ELSI;
	sys_write32(reg, dev_cfg->base + UART_IER);

	k_spin_unlock(&data->lock, key);
}

static int uart_aspeed_irq_is_pending(const struct device *dev)
{
	int ret;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (dev_cfg->dma) {
		ret = ((sys_read32(udma_base + UDMA_TX_DMA_INT_STS) |
		       sys_read32(udma_base + UDMA_RX_DMA_INT_STS)) &
		       dev_cfg->dma_ch) ? 1 : 0;
	} else {
		ret = (!(data->iir_cache & UART_IIR_IPN)) ? 1 : 0;
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int uart_aspeed_irq_update(const struct device *dev)
{
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	k_spinlock_key_t key;

	if (dev_cfg->dma)
		return 1;

	key = k_spin_lock(&data->lock);

	data->iir_cache = sys_read32(dev_cfg->base + UART_IIR);

	k_spin_unlock(&data->lock, key);

	return 1;
}

static void uart_aspeed_irq_callback_set(const struct device *dev,
					 uart_irq_callback_user_data_t cb,
					 void *cb_data)
{
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->cb = cb;
	data->cb_data = cb_data;

	k_spin_unlock(&data->lock, key);
}

static void udma_aspeed_isr(void *null_ptr)
{
	int i;
	uint32_t sts, tx_sts, rx_sts;
	struct uart_aspeed_data *data;
	k_spinlock_key_t key = k_spin_lock(&udma_lock);

	/* invoke registered callback if any TX/RX interrupt */
	tx_sts = sys_read32(udma_base + UDMA_TX_DMA_INT_STS);
	rx_sts = sys_read32(udma_base + UDMA_RX_DMA_INT_STS);
	sts = tx_sts | rx_sts;

	for (i = 0; i < UDMA_MAX_CHANNEL; ++i) {
		if (!(sts & (0x1 << i)))
			continue;

		if (!udma_udev[i])
			continue;

		data = (struct uart_aspeed_data *)udma_udev[i]->data;
		if (data->cb)
			data->cb(udma_udev[i], data->cb_data);
	}

	sys_write32(tx_sts, udma_base + UDMA_TX_DMA_INT_STS);
	sys_write32(rx_sts, udma_base + UDMA_RX_DMA_INT_STS);

	k_spin_unlock(&udma_lock, key);
}

static int udma_aspeed_init(void)
{
	int i;
	uint32_t reg;

	if (udma_init)
		return 0;

	sys_write32(0x0, udma_base + UDMA_TX_DMA_EN);
	sys_write32(0x0, udma_base + UDMA_RX_DMA_EN);

	/*
	 * For legacy design.
	 *  - TX ring buffer size: 1KB
	 *  - RX ring buffer size: 1KB
	 */
	reg = ((UDMA_BUFSZ_1KB << UDMA_MISC_TX_BUFSZ_SHIFT) & UDMA_MISC_TX_BUFSZ_MASK) |
	      ((UDMA_BUFSZ_1KB << UDMA_MISC_RX_BUFSZ_SHIFT) & UDMA_MISC_RX_BUFSZ_MASK);
	sys_write32(reg, udma_base + UDMA_MISC);

	for (i = 0; i < UDMA_MAX_CHANNEL; ++i) {
		sys_write32(0, udma_base + UDMA_CHX_TX_WR_PTR(i));
		sys_write32(0, udma_base + UDMA_CHX_RX_RD_PTR(i));
	}

	sys_write32(0xffffffff, udma_base + UDMA_TX_DMA_INT_STS);
	sys_write32(0xffffffff, udma_base + UDMA_TX_DMA_RST);
	sys_write32(0x0, udma_base + UDMA_TX_DMA_RST);

	sys_write32(0xffffffff, udma_base + UDMA_RX_DMA_INT_STS);
	sys_write32(0xffffffff, udma_base + UDMA_RX_DMA_RST);
	sys_write32(0x0, udma_base + UDMA_RX_DMA_RST);

	sys_write32(0x200, udma_base + UDMA_TIMEOUT_TIMER);

	IRQ_CONNECT(DT_IRQ_BY_IDX(DT_INST(0, aspeed_udma), 0, irq),
		    DT_IRQ_BY_IDX(DT_INST(0, aspeed_udma), 0, priority),
		    udma_aspeed_isr, NULL, 0);

	irq_enable(DT_IRQ_BY_IDX(DT_INST(0, aspeed_udma), 0, irq));

	udma_init = true;

	return 0;
}

static void uart_aspeed_isr(const struct device *dev)
{
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;

	if (data->cb)
		data->cb(dev, data->cb_data);
}

static int uart_aspeed_init(const struct device *dev)
{
	int rc = 0;
	uint32_t reg;
	struct uart_aspeed_data *data = (struct uart_aspeed_data *)dev->data;
	struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;
	struct uart_config *uart_cfg = &data->uart_cfg;

	data->iir_cache = 0;
	data->cb = NULL;
	data->cb_data = NULL;

	clock_control_on(dev_cfg->clock_dev, dev_cfg->clk_id);

	if (dev_cfg->dma) {
		udma_aspeed_init();

		udma_udev[dev_cfg->dma_ch] = dev;

		/* TX DMA init */
		data->tx_rb = udma_tx_rb[dev_cfg->dma_ch];
		data->tx_rb_addr = TO_PHY_ADDR(data->tx_rb);
		sys_write32(data->tx_rb_addr, udma_base + UDMA_CHX_TX_BUF_BASE(dev_cfg->dma_ch));

		reg = sys_read32(udma_base + UDMA_CHX_TX_CTRL(dev_cfg->dma_ch));
		reg |= (UDMA_BUFSZ_1KB << UDMA_TX_CTRL_BUFSZ_SHIFT) & UDMA_TX_CTRL_BUFSZ_MASK;
		sys_write32(reg, udma_base + UDMA_CHX_TX_CTRL(dev_cfg->dma_ch));

		reg = sys_read32(udma_base + UDMA_TX_DMA_INT_EN) | (0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_TX_DMA_INT_EN);

		reg = sys_read32(udma_base + UDMA_TX_DMA_EN) | (0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_TX_DMA_EN);

		/* RX DMA init */
		data->rx_rb = udma_rx_rb[dev_cfg->dma_ch];
		data->rx_rb_addr = TO_PHY_ADDR(data->rx_rb);
		sys_write32(data->rx_rb_addr, udma_base + UDMA_CHX_RX_BUF_BASE(dev_cfg->dma_ch));

		reg = sys_read32(udma_base + UDMA_CHX_RX_CTRL(dev_cfg->dma_ch));
		reg |= (UDMA_BUFSZ_1KB << UDMA_RX_CTRL_BUFSZ_SHIFT) & UDMA_RX_CTRL_BUFSZ_MASK;
		sys_write32(reg, udma_base + UDMA_CHX_RX_CTRL(dev_cfg->dma_ch));

		reg = sys_read32(udma_base + UDMA_RX_DMA_INT_EN) | (0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_RX_DMA_INT_EN);

		reg = sys_read32(udma_base + UDMA_RX_DMA_EN) | (0x1 << dev_cfg->dma_ch);
		sys_write32(reg, udma_base + UDMA_RX_DMA_EN);
	}

	if (dev_cfg->virt) {
		sys_write32((dev_cfg->virt_port >> 0), dev_cfg->base + VUART_ADDRL);
		sys_write32((dev_cfg->virt_port >> 8), dev_cfg->base + VUART_ADDRH);

		reg = sys_read32(dev_cfg->base + VUART_GCRB) & ~VUART_GCRB_HOST_SIRQ_MASK;
		reg |= ((dev_cfg->virt_sirq << VUART_GCRB_HOST_SIRQ_SHIFT) &
			VUART_GCRB_HOST_SIRQ_MASK);
		sys_write32(reg, dev_cfg->base + VUART_GCRB);

		reg = sys_read32(dev_cfg->base + VUART_GCRA) |
				 VUART_GCRA_DISABLE_HOST_TX_DISCARD |
				 VUART_GCRA_VUART_EN |
				 ((dev_cfg->virt_sirq_pol) ? VUART_GCRA_SIRQ_POLARITY : 0);
		sys_write32(reg, dev_cfg->base + VUART_GCRA);
	}

	uart_cfg->baudrate = 115200;
	uart_cfg->parity = UART_CFG_PARITY_NONE;
	uart_cfg->stop_bits = UART_CFG_STOP_BITS_1;
	uart_cfg->data_bits = UART_CFG_DATA_BITS_8;
	uart_cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	rc = uart_aspeed_configure(dev, uart_cfg);

	dev_cfg->irq_config_func(dev);

	return rc;
}

static const struct uart_driver_api uart_aspeed_driver_api = {
	.poll_in = uart_aspeed_poll_in,
	.poll_out = uart_aspeed_poll_out,
	.err_check = uart_aspeed_err_check,
	.configure = uart_aspeed_configure,
	.config_get = uart_aspeed_config_get,
	.fifo_fill = uart_aspeed_fifo_fill,
	.fifo_read = uart_aspeed_fifo_read,
	.irq_tx_enable = uart_aspeed_irq_tx_enable,
	.irq_tx_disable = uart_aspeed_irq_tx_disable,
	.irq_tx_ready = uart_aspeed_irq_tx_ready,
	.irq_tx_complete = uart_aspeed_irq_tx_complete,
	.irq_rx_enable = uart_aspeed_irq_rx_enable,
	.irq_rx_disable = uart_aspeed_irq_rx_disable,
	.irq_rx_ready = uart_aspeed_irq_rx_ready,
	.irq_err_enable = uart_aspeed_irq_err_enable,
	.irq_err_disable = uart_aspeed_irq_err_disable,
	.irq_is_pending = uart_aspeed_irq_is_pending,
	.irq_update = uart_aspeed_irq_update,
	.irq_callback_set = uart_aspeed_irq_callback_set,
};

#define UART_ASPEED_INIT(n)									\
												\
	static struct uart_aspeed_data uart_aspeed_data_##n;					\
												\
	static void uart_aspeed_irq_config_func_##n(const struct device *dev)			\
	{											\
		struct uart_aspeed_config *dev_cfg = (struct uart_aspeed_config *)dev->config;	\
												\
		if (dev_cfg->dma)								\
			return;									\
												\
		IRQ_CONNECT(DT_INST_IRQN(n),							\
			    DT_INST_IRQ(n, priority),						\
			    uart_aspeed_isr, DEVICE_DT_INST_GET(n), 0);				\
												\
		irq_enable(DT_INST_IRQN(n));							\
	}											\
												\
	static const struct uart_aspeed_config uart_aspeed_config_##n = {			\
		.dev_idx = n,									\
		.base = DT_INST_REG_ADDR(n),							\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clk_id),		\
		.virt = DT_INST_PROP_OR(n, virtual, 0),						\
		.virt_port = DT_INST_PROP_OR(n, virtual_port, 0),				\
		.virt_sirq = DT_INST_PROP_OR(n, virtual_sirq, 0),				\
		.virt_sirq_pol = DT_INST_PROP_OR(n, virtual_sirq_polarity, 0),			\
		.dma = DT_INST_PROP_OR(n, dma, 0),						\
		.dma_ch = DT_INST_PROP_OR(n, dma_channel, 0),					\
		.irq_config_func = uart_aspeed_irq_config_func_##n,				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n,								\
			      &uart_aspeed_init,						\
			      NULL,								\
			      &uart_aspeed_data_##n, &uart_aspeed_config_##n,			\
			      PRE_KERNEL_1,							\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,				\
			      &uart_aspeed_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_ASPEED_INIT)
