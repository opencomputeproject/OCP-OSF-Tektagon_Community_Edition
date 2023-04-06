/*
 *  Copyright (c) 2022 AMI
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <drivers/watchdog.h>
#include <watchdog/watchdog_aspeed.h>
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

int watchdog_init(const struct device *dev, struct watchdog_config *wdt_config)
{
	int ret = 0;
	struct wdt_timeout_cfg init_wdt_cfg;

	init_wdt_cfg.window.min = wdt_config->wdt_cfg.window.min;
	init_wdt_cfg.window.max = wdt_config->wdt_cfg.window.max;
	init_wdt_cfg.callback = wdt_config->wdt_cfg.callback;
	ret = wdt_install_timeout(dev, &init_wdt_cfg);
	if (ret != 0) {
		printk("watchdog_init error: fail to install dev timeout. \n");
		return ret;
	}

	ret = wdt_setup(dev, wdt_config->reset_option);
	if (ret != 0) {
		printk("watchdog_init error: fail to setup dev timeout. \n");
		return ret;
	}

	return ret;
}

/**
 *   Feed specified watchdog timeout.
 *
 *   @param dev Pointer to the device structure for the driver instance. the possible value is wdt1/wdt2/wdt3/wdt4 .
 *   @param channel_id Index of the fed channel.
 *
 *   @retval 0 If successful or an error code.
 */
int watchdog_feed(const struct device *dev, int channel_id)
{
	int ret = 0;

	ret = wdt_feed(dev, channel_id);
	return ret;
}

/**
 *   Disable watchdog instance.
 *
 *
 *   @param dev Pointer to the device structure for the driver instance, the possible value is wdt1/wdt2/wdt3/wdt4 .
 *
 *   @retval 0 If successful or an error code.
 *
 */
int watchdog_disable(const struct device *dev)
{
	int ret = 0;

	ret = wdt_disable(dev);
	return ret;
}

