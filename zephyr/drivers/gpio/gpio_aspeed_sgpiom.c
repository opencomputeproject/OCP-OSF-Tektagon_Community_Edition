/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_sgpiom

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/pinmux.h>
#include <drivers/clock_control.h>
#include <soc.h>
#include "gpio_utils.h"

#include "gpio_aspeed_sgpiom.h"

#define LOG_LEVEL CONFIG_SGPIOM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(sgpiom_aspeed);

/* Driver config */
struct device_array {
	const struct device *dev;
};
struct sgpiom_aspeed_parent_config {
	/* SGPIOM controller base address */
	struct sgpiom_register_s *base;
	const struct device *clk_dev;
	const clock_control_subsys_t clk_id;
	uint32_t irq_num;
	uint32_t irq_prio;
	struct device_array *child_dev;
	uint32_t child_num;
	uint32_t bus_freq;
	uint32_t ngpios;
};
struct sgpiom_aspeed_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* Parent device handler for shared resource */
	const struct device *parent;
	/* SGPIOM controller base address */
	struct sgpiom_register_s *base;
	uint8_t pin_offset;
};

/* Driver data */
struct sgpiom_aspeed_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
};

uint16_t gpio_offset_gather[] = {
	[0] = offsetof(struct sgpiom_register_s, gather0),
	[1] = offsetof(struct sgpiom_register_s, gather1),
	[2] = offsetof(struct sgpiom_register_s, gather2),
	[3] = offsetof(struct sgpiom_register_s, gather3),
};

/* Driver convenience defines */
#define DEV_PARENT_CFG(dev) ((const struct sgpiom_aspeed_parent_config *)(dev)->config)
#define DEV_CFG(dev) ((const struct sgpiom_aspeed_config *)(dev)->config)
#define DEV_DATA(dev) ((struct sgpiom_aspeed_data *)(dev)->data)

static void sgpiom_aspeed_isr(const void *arg)
{
	const struct device *parent = arg;
	const struct sgpiom_aspeed_parent_config *cfg;
	const struct device *dev;
	struct sgpiom_aspeed_data *data;
	uint32_t index, group_idx;
	uint32_t gpio_pin, int_pendding;
	struct sgpiom_gather_register_s *gather_reg;
	union sgpiom_int_status_register_s *int_sts;

	cfg = DEV_PARENT_CFG(parent);
	for (index = 0; index < cfg->child_num; index++) {
		dev = cfg->child_dev[index].dev;
		data = DEV_DATA(dev);
		group_idx = DEV_CFG(dev)->pin_offset >> 5;
		gather_reg = (struct sgpiom_gather_register_s *)((uint32_t)DEV_CFG(dev)->base +
								 gpio_offset_gather[group_idx]);
		int_sts = &gather_reg->int_status;
		int_pendding = int_sts->value;
		gpio_pin = 0;
		while (int_pendding) {
			if (int_pendding & 0x1) {
				gpio_fire_callbacks(&data->cb, dev, BIT(gpio_pin));
				int_sts->value = BIT(gpio_pin);
			}
			gpio_pin++;
			int_pendding >>= 1;
		}
	}
}

/* GPIO api functions */
static int sgpiom_aspeed_port_get_raw(const struct device *dev,
				      gpio_port_value_t *value)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	uint32_t group_idx = pin_offset >> 5;
	struct sgpiom_gather_register_s *gather_reg;

	gather_reg = (struct sgpiom_gather_register_s *)((uint32_t)DEV_CFG(dev)->base +
							 gpio_offset_gather[group_idx]);
	*value = gather_reg->data.value;
	return 0;
}

static int sgpiom_aspeed_port_set_masked_raw(const struct device *dev,
					     gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	struct sgpiom_register_s *gpio_reg = DEV_CFG(dev)->base;
	uint32_t group_idx = pin_offset >> 5;
	struct sgpiom_gather_register_s *gather_reg;
	uint32_t port_val;

	gather_reg = (struct sgpiom_gather_register_s *)((uint32_t)DEV_CFG(dev)->base +
							 gpio_offset_gather[group_idx]);
	port_val = gpio_reg->wr_latch[group_idx].value;
	port_val = (port_val & ~mask) | (mask & value);
	gather_reg->data.value = port_val;
	return 0;
}

static int sgpiom_aspeed_port_set_bits_raw(const struct device *dev,
					   gpio_port_value_t mask)
{
	return sgpiom_aspeed_port_set_masked_raw(dev, mask, mask);
}

static int sgpiom_aspeed_port_clear_bits_raw(const struct device *dev,
					     gpio_port_value_t mask)
{
	return sgpiom_aspeed_port_set_masked_raw(dev, mask, 0);
}

static int sgpiom_aspeed_port_toggle_bits(const struct device *dev,
					  gpio_port_value_t mask)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	struct sgpiom_register_s *gpio_reg = DEV_CFG(dev)->base;
	uint32_t group_idx = pin_offset >> 5;
	struct sgpiom_gather_register_s *gather_reg;
	uint32_t port_val;

	gather_reg = (struct sgpiom_gather_register_s *)((uint32_t)DEV_CFG(dev)->base +
							 gpio_offset_gather[group_idx]);
	port_val = gpio_reg->wr_latch[group_idx].value;
	port_val ^= mask;
	gather_reg->data.value = port_val;
	return 0;
}

static int sgpiom_aspeed_pin_interrupt_configure(const struct device *dev,
						 gpio_pin_t pin,
						 enum gpio_int_mode mode,
						 enum gpio_int_trig trig)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	uint32_t group_idx = pin_offset >> 5;
	struct sgpiom_gather_register_s *gather_reg;
	uint8_t int_type;
	bool int_en;
	uint32_t bit = BIT(pin);

	if (pin >= 32) {
		LOG_ERR("Invalid gpio pin #%d", pin);
		return -EINVAL;
	}
	gather_reg = (struct sgpiom_gather_register_s *)((uint32_t)DEV_CFG(dev)->base +
							 gpio_offset_gather[group_idx]);
	if (mode == GPIO_INT_MODE_DISABLED) {
		int_en = 0;
	} else {
		int_en = 1;
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_LOW) {
				int_type = ASPEED_SGPIOM_LEVEL_LOW;
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				int_type = ASPEED_SGPIOM_LEVEL_HIGH;
			} else {
				return -ENOTSUP;
			}
		} else {
			if (trig == GPIO_INT_TRIG_LOW) {
				int_type = ASPEED_SGPIOM_FALLING_EDGE;
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				int_type = ASPEED_SGPIOM_RAISING_EDGE;
			} else {
				int_type = ASPEED_SGPIOM_DUAL_EDGE;
			}
		}
	}

	if (int_en) {
		gather_reg->int_en.value |= bit;
		gather_reg->int_sens_type[0].value &= ~(bit);
		gather_reg->int_sens_type[1].value &= ~(bit);
		gather_reg->int_sens_type[2].value &= ~(bit);
		gather_reg->int_sens_type[0].value |= (int_type & 0x1) ? bit : 0;
		gather_reg->int_sens_type[1].value |= (int_type & 0x2) ? bit : 0;
		gather_reg->int_sens_type[2].value |= (int_type & 0x4) ? bit : 0;
	} else {
		gather_reg->int_en.value &= ~(bit);
	}
	return 0;
}

static int sgpiom_aspeed_manage_callback(const struct device *dev,
					 struct gpio_callback *callback, bool set)
{
	struct sgpiom_aspeed_data *data = DEV_DATA(dev);

	return gpio_manage_callback(&data->cb, callback, set);
}

static int sgpiom_aspeed_config(const struct device *dev,
				gpio_pin_t pin, gpio_flags_t flags)
{
	int ret = 0;
	uint32_t io_flags;

	/* Does not support disconnected pin */
	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if (io_flags == GPIO_DISCONNECTED) {
		return -ENOTSUP;
	}

	/* Does not support pull-up/pull-down */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		/* Set output pin initial value */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			ret = sgpiom_aspeed_port_set_bits_raw(dev, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			ret = sgpiom_aspeed_port_clear_bits_raw(dev, BIT(pin));
		}
	} else if (flags & GPIO_INPUT) { /* Input */
		/* do nothing */
	}
	return ret;
}

/* GPIO driver registration */
static const struct gpio_driver_api sgpiom_aspeed_driver = {
	.pin_configure = sgpiom_aspeed_config,
	.port_get_raw = sgpiom_aspeed_port_get_raw,
	.port_set_masked_raw = sgpiom_aspeed_port_set_masked_raw,
	.port_set_bits_raw = sgpiom_aspeed_port_set_bits_raw,
	.port_clear_bits_raw = sgpiom_aspeed_port_clear_bits_raw,
	.port_toggle_bits = sgpiom_aspeed_port_toggle_bits,
	.pin_interrupt_configure = sgpiom_aspeed_pin_interrupt_configure,
	.manage_callback = sgpiom_aspeed_manage_callback,
};

int sgpiom_aspeed_init(const struct device *device)
{
	return 0;
}


static uint16_t sgpiom_aspeed_get_clk_div(const struct device *parent)
{
	uint32_t clk_rate;
	uint32_t target_freq = DEV_PARENT_CFG(parent)->bus_freq;

	clock_control_get_rate(DEV_PARENT_CFG(parent)->clk_dev, DEV_PARENT_CFG(parent)->clk_id,
			       &clk_rate);
	return ((clk_rate >> 1) / target_freq) - 1;
}

int sgpiom_aspeed_parent_init(const struct device *parent)
{
	struct sgpiom_register_s *sgpiom_reg = DEV_PARENT_CFG(parent)->base;
	const struct sgpiom_aspeed_parent_config *cfg = DEV_PARENT_CFG(parent);
	union sgpiom_config_register_s config;

	config.value = sgpiom_reg->config.value;
	config.fields.enable = 1;
	config.fields.numbers = DIV_ROUND_UP(cfg->ngpios, 8);
	config.fields.division = sgpiom_aspeed_get_clk_div(parent);
	sgpiom_reg->config.value = config.value;
	irq_connect_dynamic(cfg->irq_num, cfg->irq_prio, sgpiom_aspeed_isr, parent, 0);
	irq_enable(cfg->irq_num);

	return 0;
}

struct device_cont {
	const struct sgpiom_aspeed_config *cfg;
	struct sgpiom_aspeed_data *data;
};

#define SGPIOM_ENUM(node_id) node_id,
#define SGPIOM_ASPEED_DEV_DATA(node_id) {},
#define SGPIOM_ASPEED_DEV_CFG(node_id) {					     \
		.common = {							     \
			.port_pin_mask =					     \
				GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id)	     \
		},								     \
		.parent = DEVICE_DT_GET(DT_PARENT(node_id)),			     \
		.base = (struct sgpiom_register_s *)DT_REG_ADDR(DT_PARENT(node_id)), \
		.pin_offset = DT_PROP(node_id, pin_offset),			     \
},
#define SGPIOM_ASPEED_DT_DEFINE(node_id)                                                           \
	DEVICE_DT_DEFINE(node_id, sgpiom_aspeed_init, NULL, &DT_PARENT(node_id).data[node_id],     \
			 &DT_PARENT(node_id).cfg[node_id], POST_KERNEL, 0, &sgpiom_aspeed_driver);

#define SGPIOM_ASPEED_DEV_DECLARE(node_id) { .dev = DEVICE_DT_GET(node_id) },

#define ASPEED_SGPIOM_DEVICE_INIT(inst)                                                            \
	static struct device_array child_dev_##inst[] = { DT_FOREACH_CHILD(                        \
		DT_DRV_INST(inst), SGPIOM_ASPEED_DEV_DECLARE) };                                   \
	static const struct sgpiom_aspeed_parent_config sgpiom_aspeed_parent_cfg_##inst = {        \
		.base = (struct sgpiom_register_s *)DT_INST_REG_ADDR(inst),                        \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clk_id),               \
		.irq_num = DT_INST_IRQN(inst),                                                     \
		.irq_prio = DT_INST_IRQ(inst, priority),                                           \
		.child_dev = child_dev_##inst,                                                     \
		.child_num = ARRAY_SIZE(child_dev_##inst),                                         \
		.ngpios = DT_INST_PROP(inst, ngpios),                                              \
		.bus_freq = DT_INST_PROP(inst, aspeed_bus_freq),                                   \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, sgpiom_aspeed_parent_init, NULL, NULL,                         \
			      &sgpiom_aspeed_parent_cfg_##inst, POST_KERNEL,                       \
			      CONFIG_GPIO_ASPEED_SGPIOM_INIT_PRIORITY, NULL);                      \
	static const struct sgpiom_aspeed_config sgpiom_aspeed_cfg_##inst[] = { DT_FOREACH_CHILD(  \
		DT_DRV_INST(inst), SGPIOM_ASPEED_DEV_CFG) };                                       \
	static struct sgpiom_aspeed_data sgpiom_aspeed_data_##inst[] = { DT_FOREACH_CHILD(         \
		DT_DRV_INST(inst), SGPIOM_ASPEED_DEV_DATA) };                                      \
	static const struct device_cont DT_DRV_INST(inst) = {                                      \
		.cfg = sgpiom_aspeed_cfg_##inst,                                                   \
		.data = sgpiom_aspeed_data_##inst,                                                 \
	};                                                                                         \
	enum { DT_FOREACH_CHILD(DT_DRV_INST(inst), SGPIOM_ENUM) };                                 \
	DT_FOREACH_CHILD(DT_DRV_INST(inst), SGPIOM_ASPEED_DT_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(ASPEED_SGPIOM_DEVICE_INIT)
