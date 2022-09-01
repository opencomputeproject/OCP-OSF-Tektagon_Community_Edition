// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the WatchDog Handling functions
 */
#include <watchdog/watchdog_aspeed.h>
/**
 *  WatchDog timer init;
 *
 *   @param dev Pointer to the device structure for the driver instance, the possible value is wdt1/wdt2/wdt3/wdt4 .
 *   @param wdt_cfg Watchdog timeout configuration struct , refer to drivers/watchdog.h .
 *   @param reset_option Configuration options , the possible value is WDT_FLAG_RESET_NONE/WDT_FLAG_RESET_CPU_CORE/WDT_FLAG_RESET_SOC, refer to drivers/watchdog.h .
 *
 *
 *   @retval 0 if successfully or an error code.
 *
 */
int WatchDogInit(const struct device *dev, struct watchdog_config *wdt_config)
{
	return watchdog_init(dev, wdt_config);
}

/**
 *   Feed specified watchdog timeout wrapper, points to silicon base
 *
 *   @retval 0 If successful or an error code.
 */
int WatchDogFeed(const struct device *dev, int channel_id)
{
	return watchdog_feed(dev, channel_id);
}

/**
 *   Disable watchdog wrapper points to Silicon base
 *
 *
 *   @param dev Pointer to the device structure for the driver instance, the possible value is wdt1/wdt2/wdt3/wdt4 .
 *
 *   @retval
 *
 */
int WatchDogDisable(const struct device *dev)
{
	return watchdog_disable(dev);
}

