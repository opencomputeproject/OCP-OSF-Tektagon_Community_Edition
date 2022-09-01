/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_SNOOP_ASPEED_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_SNOOP_ASPEED_H_

#define SNOOP_CHANNEL_NUM	2

int snoop_aspeed_read(const struct device *dev, uint32_t ch, uint8_t *out, bool blocking);

#endif
