/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_timer

#include <drivers/clock_control.h>
#include <drivers/timer/aspeed_timer.h>
#include <device.h>
#include <kernel.h>
#include <soc.h>
#include <errno.h>
#include <string.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(timer_aspeed, CONFIG_LOG_DEFAULT_LEVEL);


/* global control registers */
#define TIMER_GR_OFFSET		0x30
#define TIMER_GR_CTRL			0x0
#define   TIMER_GR_CTRL_WDT_RESET(n)	BIT((4 * n) + 3)
#define   TIMER_GR_CTRL_OVFL_INT(n)	BIT((4 * n) + 2)
#define   TIMER_GR_CTRL_CLKSEL(n)	BIT((4 * n) + 1)
#define   TIMER_GR_CTRL_EN(n)		BIT((4 * n) + 0)
#define TIMER_GR_INTR_STATUS		0x4
#define   TIMER_GR_INTR_STATUS_TM(n)	BIT(n)
#define TIMER_GR_CTRL_CLR		0xc

/* timer registers */
#define TIMER_COUNTER_STATUS		0x0
#define TIMER_RELOAD_VALUE		0x4
#define TIMER_1ST_MATCHING		0x8
#define TIMER_2ND_MATCHING		0xc

static const uintptr_t cr_base_lookup[8] = {
	0x0, 0x10, 0x20, 0x40, 0x50, 0x60, 0x70, 0x80
};

struct timer_aspeed_obj {
	uintptr_t gr_base;		/* global control registers */
	uintptr_t cr_base;		/* counter registers */
	uint32_t index;
	uint32_t tick_per_microsec;
	void (*callback)(void *user_data);
	void *user_data;
	bool auto_reload;
};

struct timer_aspeed_config {
	uintptr_t parent_base;
	uint32_t index;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_id;
};

#define DEV_CFG(dev)			((struct timer_aspeed_config *)(dev)->config)
#define DEV_DATA(dev)			((struct timer_aspeed_obj *)(dev)->data)

int timer_aspeed_stop(const struct device *dev)
{
	struct timer_aspeed_obj *obj = DEV_DATA(dev);

	/* Write CTRL_CLR to clear CTRL bits */
	sys_write32(TIMER_GR_CTRL_EN(obj->index), obj->gr_base + TIMER_GR_CTRL_CLR);

	/* clear reload value then the HW will clear counter automatically */
	sys_write32(0, obj->cr_base + TIMER_RELOAD_VALUE);

	return 0;
}

int timer_aspeed_start(const struct device *dev, struct aspeed_timer_user_config *user_config)
{
	struct timer_aspeed_obj *obj = DEV_DATA(dev);
	uint32_t reload, reg;

	/* suspend timer */
	sys_write32(TIMER_GR_CTRL_EN(obj->index), obj->gr_base + TIMER_GR_CTRL_CLR);

	/* configure timer, not support 1st & 2nd matching value for now */
	reload = user_config->millisec * obj->tick_per_microsec * 1000;
	sys_write32(reload, obj->cr_base + TIMER_RELOAD_VALUE);
	sys_write32(0xffffffff, obj->cr_base + TIMER_1ST_MATCHING);
	sys_write32(0xffffffff, obj->cr_base + TIMER_2ND_MATCHING);
	LOG_DBG("reload: %d ms, %d tick\n", user_config->millisec, reload);

	obj->callback = user_config->callback;
	obj->user_data = user_config->user_data;
	obj->auto_reload = (user_config->timer_type == ASPEED_TIMER_TYPE_ONE_SHOT) ? 0 : 1;

	/* enable timer */
	reg = TIMER_GR_CTRL_EN(obj->index) | TIMER_GR_CTRL_OVFL_INT(obj->index);
	sys_write32(reg, obj->gr_base + TIMER_GR_CTRL);

	return 0;
}

int timer_aspeed_query(const struct device *dev)
{
	struct timer_aspeed_obj *obj = DEV_DATA(dev);

	return sys_read32(obj->cr_base + TIMER_COUNTER_STATUS);
}

/*
 * if (1st matching && 2nd matching)
 *         overflow interrupt is controlled by TMR30
 * else
 *         overflow interrupt is enabled
 */
static void timer_aspeed_isr(const struct device *dev)
{
	struct timer_aspeed_obj *obj = DEV_DATA(dev);

	sys_write32(TIMER_GR_INTR_STATUS_TM(obj->index),
		    obj->gr_base + TIMER_GR_INTR_STATUS);

	/* disable timer if not auto-reload */
	if (!obj->auto_reload) {
		timer_aspeed_stop(dev);
	}

	if (obj->callback) {
		obj->callback(obj->user_data);
	}
}

static int timer_aspeed_init(const struct device *dev)
{
	struct timer_aspeed_config *config = DEV_CFG(dev);
	struct timer_aspeed_obj *obj = DEV_DATA(dev);
	uint32_t clock_freq;

	obj->cr_base = config->parent_base + cr_base_lookup[config->index];
	obj->gr_base = config->parent_base + TIMER_GR_OFFSET;
	obj->index = config->index;

	clock_control_get_rate(config->clock_dev, config->clock_id, &clock_freq);
	obj->tick_per_microsec = clock_freq / 1000000;

	return 0;
}

#define TIMER_ASPEED_INIT(n)                                                                       \
	static int timer_aspeed_config_func_##n(const struct device *dev);                         \
	static const struct timer_aspeed_config timer_aspeed_config_##n = {                        \
		.parent_base = DT_REG_ADDR(DT_PARENT(DT_DRV_INST(n))),                             \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, clk_id),                \
		.index = DT_INST_PROP(n, index),                                                   \
	};                                                                                         \
												   \
	static struct timer_aspeed_obj timer_aspeed_obj_##n;                                       \
												   \
	DEVICE_DT_INST_DEFINE(n, &timer_aspeed_config_func_##n, NULL, &timer_aspeed_obj_##n,       \
			      &timer_aspeed_config_##n, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);                           \
												   \
	static int timer_aspeed_config_func_##n(const struct device *dev)                          \
	{                                                                                          \
		timer_aspeed_init(dev);                                                            \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), timer_aspeed_isr,           \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
		return 0;                                                                          \
	}

DT_INST_FOREACH_STATUS_OKAY(TIMER_ASPEED_INIT)
