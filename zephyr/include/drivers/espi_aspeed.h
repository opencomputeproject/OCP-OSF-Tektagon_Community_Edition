/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_ESPI_ASPEED_H_
#define ZEPHYR_INCLUDE_DRIVERS_ESPI_ASPEED_H_

struct espi_aspeed_ioc {
	uint32_t pkt_len;
	uint8_t *pkt;
};

int espi_aspeed_perif_pc_get_rx(const struct device *dev, struct espi_aspeed_ioc *ioc,
				bool blocking);
int espi_aspeed_perif_pc_put_tx(const struct device *dev, struct espi_aspeed_ioc *ioc);
int espi_aspeed_perif_np_put_tx(const struct device *dev, struct espi_aspeed_ioc *ioc);
int espi_aspeed_oob_get_rx(const struct device *dev, struct espi_aspeed_ioc *ioc, bool blocking);
int espi_aspeed_oob_put_tx(const struct device *dev, struct espi_aspeed_ioc *ioc);
int espi_aspeed_flash_get_rx(const struct device *dev, struct espi_aspeed_ioc *ioc, bool blocking);
int espi_aspeed_flash_put_tx(const struct device *dev, struct espi_aspeed_ioc *ioc);

#endif
