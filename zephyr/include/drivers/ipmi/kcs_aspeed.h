/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _IPMI_KCS_ASPEED_H
#define _IPMI_KCS_ASPEED_H

int kcs_aspeed_read(const struct device *dev,
		    uint8_t *buf, uint32_t buf_sz);
int kcs_aspeed_write(const struct device *dev,
		     uint8_t *buf, uint32_t buf_sz);

#endif
