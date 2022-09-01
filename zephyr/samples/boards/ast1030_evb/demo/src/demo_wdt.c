/*
 * Copyright 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <soc.h>
#include <drivers/watchdog.h>

void wdt_demo_background_thread(void)
{
	int ret = 0;
	const struct device *wdt2_dev = NULL;
	struct wdt_timeout_cfg wdt_config;

	wdt2_dev = device_get_binding("wdt2");
	if (!wdt2_dev) {
		printk("demo_err: cannot find wdt2 device.\n");
		return;
	}

	wdt_config.window.min = 0U;
	wdt_config.window.max = 5000; /* 5s */
	wdt_config.callback = NULL;
	ret = wdt_install_timeout(wdt2_dev, &wdt_config);
	if (ret != 0) {
		printk("demo_err: fail to install wdt2 timeout.\n");
		return;
	}

	ret = wdt_setup(wdt2_dev, WDT_FLAG_RESET_CPU_CORE);
	if (ret != 0) {
		printk("demo_err: fail to setup wdt2.\n");
		return;
	}

	while (1) {
		wdt_feed(wdt2_dev, 0);
		k_sleep(K_MSEC(3000));
	}
}

K_THREAD_DEFINE(wdt_background, 1024, wdt_demo_background_thread,
	NULL, NULL, NULL, -1, 0, 1000);

