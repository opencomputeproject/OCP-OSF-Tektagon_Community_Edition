/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DEMO_GPIO_H__
#define __DEMO_GPIO_H__

#include <zephyr.h>
#include <drivers/gpio.h>
#include <sys/util.h>

#if DT_NODE_HAS_STATUS(DT_INST(0, demo_gpio_basic_api), okay)
#define GPIOO3_DEV_NAME DT_GPIO_LABEL_BY_IDX(DT_INST(0, demo_gpio_basic_api), in_gpios, 0)
#define GPIOO3_PIN_IN DT_GPIO_PIN_BY_IDX(DT_INST(0, demo_gpio_basic_api), in_gpios, 0)
#else
#error Unsupported board
#endif

#endif /* __DEMO_GPIO_H__ */
