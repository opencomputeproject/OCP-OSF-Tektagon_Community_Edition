/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _IPMI_BT_ASPEED_H
#define _IPMI_BT_ASPEED_H

int bt_aspeed_read(const struct device *dev,
		   uint8_t *buf, uint32_t buf_sz);
int bt_aspeed_write(const struct device *dev,
		    uint8_t *buf, uint32_t buf_sz);

#endif
