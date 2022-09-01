/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <tc_util.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <drivers/uart.h>
#include "ast_test.h"

#define LOG_MODULE_NAME usb_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Max packet size for endpoints */
#define BULK_EP_MPS		64
#define ENDP_BULK_IN		0x81
#define VALID_EP		ENDP_BULK_IN
#define INVALID_EP		0x20

#define RX_BUFF_SIZE		64
#define RING_BUF_SIZE		256

RING_BUF_DECLARE(ringbuf, RING_BUF_SIZE);

K_SEM_DEFINE(usb_sem, 0, 1);
static const char *golden_str = "usb_test";

static int data_handle(void)
{
	uint8_t buff[RX_BUFF_SIZE];
	int data_print = 1;
	int rb_len;
	int i;

	rb_len = ring_buf_get(&ringbuf, buff, sizeof(buff));
	if (data_print) {
		LOG_INF("Receive Data: ");
		for (i = 0; i < rb_len; i++)
			LOG_INF("0x%x ", buff[i]);
	}

	/* Check golden pattern */
	rb_len = strlen(golden_str);
	for (i = 0; i < rb_len; i++) {
		if (golden_str[i] != buff[i]) {
			LOG_DBG("cmp to %d, buff: %s\n", i, buff);
			return -EIO;
		}
	}

	return TC_PASS;
}

static void interrupt_handler(const struct device *dev, void *user_data)
{
	uint8_t rx_buff[RX_BUFF_SIZE];
	int total_len = 0;
	int recv_len;
	int rb_len;

	ARG_UNUSED(user_data);

	while (uart_irq_is_pending(dev) && uart_irq_rx_ready(dev)) {
		recv_len = uart_fifo_read(dev, rx_buff, sizeof(rx_buff));
		total_len += recv_len;

		if (recv_len) {
			rb_len = ring_buf_put(&ringbuf, rx_buff, recv_len);
			if (rb_len < recv_len) {
				LOG_DBG("Drop %u bytes\n", recv_len - rb_len);
			}
		} else {
			break;
		}

		LOG_DBG("total transfer len: 0x%x\n", total_len);
		k_sem_give(&usb_sem);
	}
}

static void test_usb_comm(void)
{
	const struct device *dev;
	int count = 0;
	int ret;

	dev = device_get_binding("CDC_ACM_0");
	ast_zassert_not_null(dev, "CDC ACM device is not found");

	uart_irq_callback_set(dev, interrupt_handler);

	/* Enable rx interrupts */
	uart_irq_rx_enable(dev);

	LOG_INF("Wait for host side data, expect to receive \"%s\"...\n",
			golden_str);

	while (count < 100) {
		ret = k_sem_take(&usb_sem, K_NO_WAIT);
		if (ret) {
			count++;
			k_sleep(K_MSEC(100));
		} else {
			break;
		}
	}

	ast_zassert_equal(data_handle(), TC_PASS, "Unexpected received data");
}

/* Test USB Device Cotnroller API */
static void test_usb_dc_api(void)
{
	/* Control endpoins are configured */
	ast_zassert_equal(usb_dc_ep_mps(0x0), 64,
		      "usb_dc_ep_mps(0x00) failed");
	ast_zassert_equal(usb_dc_ep_mps(0x80), 64,
		      "usb_dc_ep_mps(0x80) failed");

	/* Bulk EP is not configured yet */
	ast_zassert_equal(usb_dc_ep_mps(ENDP_BULK_IN), 0,
		      "usb_dc_ep_mps(ENDP_BULK_IN) not configured");
}

/* Test USB Device Cotnroller API for invalid parameters */
static void test_usb_dc_api_invalid(void)
{
	uint32_t size;
	uint8_t byte;

	/* Set stall to invalid EP */
	ast_zassert_not_equal(usb_dc_ep_set_stall(INVALID_EP), TC_PASS,
			  "usb_dc_ep_set_stall(INVALID_EP)");

	/* Clear stall to invalid EP */
	ast_zassert_not_equal(usb_dc_ep_clear_stall(INVALID_EP), TC_PASS,
			  "usb_dc_ep_clear_stall(INVALID_EP)");

	/* Check if the selected endpoint is stalled */
	ast_zassert_not_equal(usb_dc_ep_is_stalled(INVALID_EP, &byte), TC_PASS,
			  "usb_dc_ep_is_stalled(INVALID_EP, stalled)");
	ast_zassert_not_equal(usb_dc_ep_is_stalled(VALID_EP, NULL), TC_PASS,
			  "usb_dc_ep_is_stalled(VALID_EP, NULL)");

	/* Halt invalid EP */
	ast_zassert_not_equal(usb_dc_ep_halt(INVALID_EP), TC_PASS,
			  "usb_dc_ep_halt(INVALID_EP)");

	/* Enable invalid EP */
	ast_zassert_not_equal(usb_dc_ep_enable(INVALID_EP), TC_PASS,
			  "usb_dc_ep_enable(INVALID_EP)");

	/* Disable invalid EP */
	ast_zassert_not_equal(usb_dc_ep_disable(INVALID_EP), TC_PASS,
			  "usb_dc_ep_disable(INVALID_EP)");

	/* Flush invalid EP */
	ast_zassert_not_equal(usb_dc_ep_flush(INVALID_EP), TC_PASS,
			  "usb_dc_ep_flush(INVALID_EP)");

	/* Set callback to invalid EP */
	ast_zassert_not_equal(usb_dc_ep_set_callback(INVALID_EP, NULL), TC_PASS,
			  "usb_dc_ep_set_callback(INVALID_EP, NULL)");

	/* Write to invalid EP */
	ast_zassert_not_equal(usb_dc_ep_write(INVALID_EP, &byte, sizeof(byte),
					  &size),
			  TC_PASS, "usb_dc_ep_write(INVALID_EP)");

	/* Read invalid EP */
	ast_zassert_not_equal(usb_dc_ep_read(INVALID_EP, &byte, sizeof(byte),
					 &size),
			  TC_PASS, "usb_dc_ep_read(INVALID_EP)");
	ast_zassert_not_equal(usb_dc_ep_read_wait(INVALID_EP, &byte, sizeof(byte),
					      &size),
			  TC_PASS, "usb_dc_ep_read_wait(INVALID_EP)");
	ast_zassert_not_equal(usb_dc_ep_read_continue(INVALID_EP), TC_PASS,
			  "usb_dc_ep_read_continue(INVALID_EP)");
}

static void test_usb_dc_api_read_write(void)
{
	uint32_t size;
	uint8_t byte;

	/* Read invalid EP */
	ast_zassert_not_equal(usb_read(INVALID_EP, &byte, sizeof(byte), &size),
			  TC_PASS, "usb_read(INVALID_EP)");

	/* Write to invalid EP */
	ast_zassert_not_equal(usb_write(INVALID_EP, &byte, sizeof(byte), &size),
			  TC_PASS, "usb_write(INVALID_EP)");
}

static void test_usb_enable(void)
{
	ast_zassert_equal(usb_enable(NULL), 0, "usb_enable() failed");
}

int test_usb(int count, enum aspeed_test_type type)
{
	LOG_INF("%s, count: %d, type: %d", __func__, count, type);

	if (type != AST_TEST_CI)
		return AST_TEST_PASS;

	test_usb_enable();
	test_usb_dc_api();
	test_usb_dc_api_read_write();
	test_usb_dc_api_invalid();
	test_usb_comm();

	return ast_ztest_result();
}
