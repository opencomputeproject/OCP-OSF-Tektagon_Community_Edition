/*
 * Copyright (c) 2022 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_PCC_ASPEED_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_PCC_ASPEED_H_

int pcc_aspeed_read(const struct device *dev, uint8_t *out, bool blocking);

#endif
