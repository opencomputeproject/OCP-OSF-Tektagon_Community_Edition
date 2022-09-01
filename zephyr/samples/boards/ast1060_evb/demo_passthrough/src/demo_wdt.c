/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <zephyr/types.h>
#include <sys/printk.h>
#include <drivers/misc/aspeed/pfr_aspeed.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <sys/util.h>
#include <errno.h>
#include <drivers/watchdog.h>
#include <soc.h>

#if CONFIG_WDT_DEMO_ASPEED
static void demo_wdt_callback(const struct device *dev, int channel_id)
{
	printk("[%s] timeout is triggered.\n", dev->name);
}

void demo_wdt(void)
{
	struct wdt_timeout_cfg wdt_config;
	const struct device *wdt3_dev;
	const struct device *wdt4_dev;
	int ret;
	uint32_t count = 0;

	printk("wdt demo started.\n");

	wdt3_dev = device_get_binding("wdt3");
	if (!wdt3_dev) {
		printk("demo_err: cannot find wdt3 device.\n");
		return;
	}

	wdt4_dev = device_get_binding("wdt4");
	if (!wdt3_dev) {
		printk("demo_err: cannot find wdt4 device.\n");
		return;
	}

	wdt_config.window.min = 0U;
	wdt_config.window.max = 1000; /* 1 s */
	wdt_config.callback = demo_wdt_callback;
	ret = wdt_install_timeout(wdt3_dev, &wdt_config);
	if (ret != 0) {
		printk("demo_err: fail to install wdt3 timeout.\n");
		return;
	}

	wdt_config.window.min = 0U;
	wdt_config.window.max = 3000; /* 3s */
	wdt_config.callback = NULL;
	ret = wdt_install_timeout(wdt4_dev, &wdt_config);
	if (ret != 0) {
		printk("demo_err: fail to install wdt4 timeout.\n");
		return;
	}

	ret = wdt_setup(wdt3_dev, WDT_FLAG_RESET_NONE);
	if (ret != 0) {
		printk("demo_err: fail to setup wdt3.\n");
		return;
	}

	ret = wdt_setup(wdt4_dev, WDT_FLAG_RESET_CPU_CORE);
	if (ret != 0) {
		printk("demo_err: fail to setup wdt4.\n");
		return;
	}

	while (1) {
		if (count == 10) {
			printk("wdt_demo getting stuck...\n");
			k_sleep(K_FOREVER);
		}

		printk("wdt_demo ... %d\n", count);

		wdt_feed(wdt3_dev, 0);
		wdt_feed(wdt4_dev, 0);
		if (count < 5)
			k_sleep(K_MSEC(500));
		else
			k_sleep(K_MSEC(2000));

		count++;
	}
}
#endif

static void demo_wdt_background_thread(void)
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
#if CONFIG_WDT_DEMO_ASPEED
		printk("wdt_demo thread still alive...\n");
#endif
		wdt_feed(wdt2_dev, 0);
		k_sleep(K_MSEC(3000));
	}
}

K_THREAD_DEFINE(wdt_background, 1024, demo_wdt_background_thread,
	NULL, NULL, NULL, -1, 0, 1000);

