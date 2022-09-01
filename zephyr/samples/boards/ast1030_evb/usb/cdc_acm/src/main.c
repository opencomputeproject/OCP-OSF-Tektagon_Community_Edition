/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample data transfer for CDC ACM class
 *
 * Sample app for USB CDC ACM class driver. The received data will be printed.
 */

#include <device.h>
#include <drivers/uart.h>
#include <usb/usb_device.h>
#include <logging/log.h>
#include <sys/ring_buffer.h>

LOG_MODULE_REGISTER(cdc_acm, LOG_LEVEL_INF);

#define RX_BUFF_SIZE	64
#define RING_BUF_SIZE	KB(100)

uint8_t ring_buffer[RING_BUF_SIZE];

struct ring_buf ringbuf;
int total_len;

static void interrupt_handler(const struct device *dev, void *user_data)
{
	uint8_t rx_buff[RX_BUFF_SIZE];
	int recv_len, rb_len;

	ARG_UNUSED(user_data);

	while (uart_irq_is_pending(dev)) {
		uart_irq_rx_disable(dev);

		recv_len = uart_fifo_read(dev, rx_buff, sizeof(rx_buff));
		if (!recv_len)
			goto done;

		LOG_INF("recv_len: 0x%x", recv_len);

		rb_len = ring_buf_put(&ringbuf, rx_buff, recv_len);
		if (rb_len < recv_len) {
			LOG_ERR("Drop %u bytes", recv_len - rb_len);
		}

		total_len += recv_len;

		rb_len = ring_buf_get(&ringbuf, rx_buff, sizeof(rx_buff));

#ifdef RX_BUFF_PRINT
		int i;

		for (i = 0; i < rb_len; i++)
			LOG_INF("rx_buff: 0x%x", rx_buff[i]);
#endif
	}

	LOG_INF("total transfer len: 0x%x", total_len);

done:
	uart_irq_rx_enable(dev);
}

void main(void)
{
	const struct device *dev;
	int ret;

	dev = device_get_binding("CDC_ACM_0");
	if (!dev) {
		LOG_ERR("CDC ACM device not found");
		return;
	}

	ret = usb_enable(NULL);
	if (ret) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);

	uart_irq_callback_set(dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);
}
