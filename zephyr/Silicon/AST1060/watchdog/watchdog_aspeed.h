
/*
 *  Copyright (c) 2022 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_WATCHDOG_API_MIDLETER_H_
#define ZEPHYR_INCLUDE_WATCHDOG_API_MIDLETER_H_

#include <drivers/watchdog.h>
#include <device.h>

struct watchdog_config {
	struct wdt_timeout_cfg wdt_cfg;
	uint8_t reset_option; // WDT_FLAG_RESET_SHIFT , WDT_FLAG_RESET_CPU_CORE , WDT_FLAG_RESET_SOC
};

static char *WDT_Devices_List[4] = {
	"wdt1",
	"wdt2",
	"wdt3",
	"wdt4"
};
/**
 *  Initial watchdog timer and configure timeout configuration.
 *
 *   @param dev Pointer to the device structure for the driver instance, the possible value is wdt1/wdt2/wdt3/wdt4 .
 *   @param wdt_cfg Watchdog timeout configuration struct , refer to drivers/watchdog.h .
 *   @param reset_option Configuration options , the possible value is WDT_FLAG_RESET_NONE/WDT_FLAG_RESET_CPU_CORE/WDT_FLAG_RESET_SOC, refer to drivers/watchdog.h .
 *
 *
 *   @retval 0 if successfully or an error code.
 *
 */
// int watchdog_init(const struct device *dev, const struct wdt_timeout_cfg *wdt_cfg, uint8_t reset_option);
int watchdog_init(const struct device *dev, struct watchdog_config *wdt_cfg);

/**
 *   Feed specified watchdog timeout.
 *
 *   @param dev Pointer to the device structure for the driver instance. the possible value is wdt1/wdt2/wdt3/wdt4 .
 *   @param channel_id Index of the fed channel.
 *
 *   @retval 0 If successful or an error code.
 */
int watchdog_feed(const struct device *dev, int channel_id);


/**
 *   Disable watchdog instance.
 *
 *
 *   @param dev Pointer to the device structure for the driver instance, the possible value is wdt1/wdt2/wdt3/wdt4 .
 *
 *   @retval 0 If successful or an error code.
 *
 */
int watchdog_disable(const struct device *dev);


#endif
