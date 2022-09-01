/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_gpio

#include <kernel.h>
#include <device.h>
#include <drivers/gpio.h>
#include <drivers/pinmux.h>
#include <drivers/clock_control.h>
#include <soc.h>

#include "gpio_utils.h"
#include "gpio_aspeed.h"
#include "sig_id.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_aspeed);

/* Driver config */
struct device_array {
	const struct device *dev;
};
struct gpio_aspeed_parent_config {
	/* GPIO controller base address */
	gpio_register_t *base;
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
	uint32_t irq_num;
	uint32_t irq_prio;
	struct device_array *child_dev;
	uint32_t child_num;
	uint32_t deb_interval;
};
struct gpio_aspeed_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* Parent device handler for shared resource */
	const struct device *parent;
	/* GPIO controller base address */
	gpio_register_t *base;
	uint8_t pin_offset;
	uint8_t gpio_master;
	uint8_t group_dedicated;
	uint32_t persist_maps;
};

/* Driver data */
struct gpio_aspeed_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
	/* pinmux device */
	const struct device *pinmux;
};

uint16_t gpio_offset_data[] = {
	[0] = offsetof(gpio_register_t, group0_data),
	[1] = offsetof(gpio_register_t, group1_data),
	[2] = offsetof(gpio_register_t, group2_data),
	[3] = offsetof(gpio_register_t, group3_data),
	[4] = offsetof(gpio_register_t, group4_data),
	[5] = offsetof(gpio_register_t, group5_data),
	[6] = offsetof(gpio_register_t, group6_data),
};

uint16_t gpio_offset_write_latch[] = {
	[0] = offsetof(gpio_register_t, group0_rd_data),
	[1] = offsetof(gpio_register_t, group1_rd_data),
	[2] = offsetof(gpio_register_t, group2_rd_data),
	[3] = offsetof(gpio_register_t, group3_rd_data),
	[4] = offsetof(gpio_register_t, group4_rd_data),
	[5] = offsetof(gpio_register_t, group5_rd_data),
	[6] = offsetof(gpio_register_t, group6_rd_data),
};

uint16_t gpio_offset_int_status[] = {
	[0] = offsetof(gpio_register_t, group0_int_status),
	[1] = offsetof(gpio_register_t, group1_int_status),
	[2] = offsetof(gpio_register_t, group2_int_status),
	[3] = offsetof(gpio_register_t, group3_int_status),
	[4] = offsetof(gpio_register_t, group4_int_status),
	[5] = offsetof(gpio_register_t, group5_int_status),
	[6] = offsetof(gpio_register_t, group6_int_status),
};

uint16_t gpio_offset_wr_permit[] = {
	[0] = offsetof(gpio_register_t, group0_write_cmd_src),
	[1] = offsetof(gpio_register_t, group1_write_cmd_src),
	[2] = offsetof(gpio_register_t, group2_write_cmd_src),
	[3] = offsetof(gpio_register_t, group3_write_cmd_src),
	[4] = offsetof(gpio_register_t, group4_write_cmd_src),
	[5] = offsetof(gpio_register_t, group5_write_cmd_src),
	[6] = offsetof(gpio_register_t, group6_write_cmd_src),
};

uint16_t gpio_offset_rd_permit[] = {
	[0] = offsetof(gpio_register_t, group0_read_cmd_src),
	[1] = offsetof(gpio_register_t, group1_read_cmd_src),
	[2] = offsetof(gpio_register_t, group2_read_cmd_src),
	[3] = offsetof(gpio_register_t, group3_read_cmd_src),
	[4] = offsetof(gpio_register_t, group4_read_cmd_src),
	[5] = offsetof(gpio_register_t, group5_read_cmd_src),
	[6] = offsetof(gpio_register_t, group6_read_cmd_src),
};

/* Driver convenience defines */
#define DEV_PARENT_CFG(dev) ((const struct gpio_aspeed_parent_config *)(dev)->config)
#define DEV_CFG(dev) ((const struct gpio_aspeed_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_aspeed_data *)(dev)->data)

static int gpio_aspeed_cmd_src_set(const struct device *dev, gpio_pin_t pin, uint8_t cmd_src)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;

#if CONFIG_GPIO_ASPEED_PERMIT_LOCK
	gpio_new_cmd_src_t *new_cmd_src, temp;
	gpio_set_permit_t permit;
	uint32_t group_idx = pin_offset >> 5;
	uint8_t byte_idx;
#endif
	volatile gpio_register_t *gpio_reg = DEV_CFG(dev)->base;
	gpio_index_register_t index;

	if (pin >= 32) {
		LOG_ERR("Invalid gpio pin #%d", pin);
		return -EINVAL;
	}
#if CONFIG_GPIO_ASPEED_PERMIT_LOCK
	byte_idx = pin >> 3;
	pin += pin_offset;
	permit.byte = BIT(cmd_src);
	permit.bits.mode_sel = 1;
	permit.bits.lock = 1;
	new_cmd_src = (gpio_new_cmd_src_t *)((uint32_t)DEV_CFG(dev)->base +
					     gpio_offset_wr_permit[group_idx]);
	temp.value = new_cmd_src->value;
	temp.fields.sets[byte_idx] = permit.byte;
	new_cmd_src->value = temp.value;
	new_cmd_src = (gpio_new_cmd_src_t *)((uint32_t)DEV_CFG(dev)->base +
					     gpio_offset_rd_permit[group_idx]);
	temp.value = new_cmd_src->value;
	temp.fields.sets[byte_idx] = permit.byte;
	new_cmd_src->value = temp.value;
#endif
	pin += pin_offset;
	index.value = 0;
	index.fields.index_type = ASPEED_GPIO_CMD_SRC;
	index.fields.index_command = ASPEED_GPIO_INDEX_WRITE;
	index.fields.index_number = pin;
	index.fields.index_data = cmd_src;
	gpio_reg->index.value = index.value;
	return 0;
}

static int gpio_aspeed_init_cmd_src(const struct device *dev)
{
	uint32_t group_dedicated = DEV_CFG(dev)->group_dedicated;
	uint32_t gpio_group_index;
	int ret;

	for (gpio_group_index = 0; gpio_group_index < 4; gpio_group_index++) {
		if (group_dedicated & BIT(gpio_group_index)) {
			ret = gpio_aspeed_cmd_src_set(dev, gpio_group_index * 8,
						      DEV_CFG(dev)->gpio_master);
			if (ret) {
				return ret;
			}
		}
	}
	return 0;
}

static int gpio_aspeed_set_direction(const struct device *dev, gpio_pin_t pin, int direct)
{
	volatile gpio_register_t *gpio_reg = DEV_CFG(dev)->base;
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	gpio_index_register_t index;

	if (pin >= 32) {
		LOG_ERR("Invalid gpio pin #%d", pin);
		return -EINVAL;
	}
	pin += pin_offset;
	index.value = 0;
	index.fields.index_type = ASPEED_GPIO_DIRECTION;
	index.fields.index_command = ASPEED_GPIO_INDEX_WRITE;
	index.fields.index_data = direct;
	index.fields.index_number = pin;
	LOG_DBG("gpio index = 0x%08x\n", index.value);
	gpio_reg->index.value = index.value;
	return 0;
}

static void gpio_aspeed_isr(const void *arg)
{
	const struct device *parent = arg;
	const struct gpio_aspeed_parent_config *cfg;
	const struct device *dev;
	struct gpio_aspeed_data *data;
	uint32_t index, group_idx;
	uint32_t gpio_pin, int_pendding;
	gpio_int_status_register_t *int_reg;

	cfg = DEV_PARENT_CFG(parent);
	for (index = 0; index < cfg->child_num; index++) {
		dev = cfg->child_dev[index].dev;
		data = DEV_DATA(dev);
		group_idx = DEV_CFG(dev)->pin_offset >> 5;
		int_reg = (gpio_int_status_register_t *)((uint32_t)DEV_CFG(dev)->base +
							 gpio_offset_int_status[group_idx]);
		int_pendding = int_reg->value;
		gpio_pin = 0;
		while (int_pendding) {
			if (int_pendding & 0x1) {
				gpio_fire_callbacks(&data->cb,
						    dev, BIT(gpio_pin));
				int_reg->value = BIT(gpio_pin);
			}
			gpio_pin++;
			int_pendding >>= 1;
		}
	}
}

/* GPIO api functions */
static int gpio_aspeed_port_get_raw(const struct device *dev,
				    gpio_port_value_t *value)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	uint32_t group_idx = pin_offset >> 5;

	*value = sys_read32((uint32_t)DEV_CFG(dev)->base + gpio_offset_data[group_idx]);
	return 0;
}

static int gpio_aspeed_port_set_masked_raw(const struct device *dev,
					   gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	uint32_t group_idx = pin_offset >> 5;
	uint32_t port_val;

	port_val = sys_read32((uint32_t)DEV_CFG(dev)->base + gpio_offset_write_latch[group_idx]);
	port_val = (port_val & ~mask) | (mask & value);
	sys_write32(port_val, (uint32_t)DEV_CFG(dev)->base + gpio_offset_data[group_idx]);
	return 0;
}

static int gpio_aspeed_port_set_bits_raw(const struct device *dev,
					 gpio_port_value_t mask)
{
	return gpio_aspeed_port_set_masked_raw(dev, mask, mask);
}

static int gpio_aspeed_port_clear_bits_raw(const struct device *dev,
					   gpio_port_value_t mask)
{
	return gpio_aspeed_port_set_masked_raw(dev, mask, 0);
}

static int gpio_aspeed_port_toggle_bits(const struct device *dev,
					gpio_port_value_t mask)
{
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	uint32_t group_idx = pin_offset >> 5;
	uint32_t port_val;

	port_val = sys_read32((uint32_t)DEV_CFG(dev)->base + gpio_offset_write_latch[group_idx]);
	port_val ^= mask;
	sys_write32(port_val, (uint32_t)DEV_CFG(dev)->base + gpio_offset_data[group_idx]);
	return 0;
}

static int gpio_aspeed_pin_interrupt_configure(const struct device *dev,
					       gpio_pin_t pin,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	volatile gpio_register_t *gpio_reg = DEV_CFG(dev)->base;
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	gpio_index_register_t index;
	uint8_t int_type, index_data;

	if (pin >= 32) {
		LOG_ERR("Invalid gpio pin #%d", pin);
		return -EINVAL;
	}
	pin += pin_offset;
	index.value = 0;

	if (mode == GPIO_INT_MODE_DISABLED) {
		index_data = 0;
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig == GPIO_INT_TRIG_LOW) {
				int_type = ASPEED_GPIO_LEVEL_LOW;
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				int_type = ASPEED_GPIO_LEVEL_HIGH;
			} else {
				return -ENOTSUP;
			}
		} else {
			if (trig == GPIO_INT_TRIG_LOW) {
				int_type = ASPEED_GPIO_FALLING_EDGE;
			} else if (trig == GPIO_INT_TRIG_HIGH) {
				int_type = ASPEED_GPIO_RAISING_EDGE;
			} else {
				int_type = ASPEED_GPIO_DUAL_EDGE;
			}
		}
		index_data = BIT(0) | (int_type << 1);
	}

	index.fields.index_type = ASPEED_GPIO_INTERRUPT;
	index.fields.index_command = ASPEED_GPIO_INDEX_WRITE;
	index.fields.index_data = index_data;
	index.fields.index_number = pin;
	LOG_DBG("gpio index = 0x%08x\n", index.value);
	gpio_reg->index.value = index.value;

	return 0;
}

static int gpio_aspeed_manage_callback(const struct device *dev,
				       struct gpio_callback *callback, bool set)
{
	struct gpio_aspeed_data *data = DEV_DATA(dev);

	return gpio_manage_callback(&data->cb, callback, set);
}

static int gpio_aspeed_deb_en(const struct device *dev, gpio_pin_t pin, bool en)
{
	volatile gpio_register_t *gpio_reg = DEV_CFG(dev)->base;
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	gpio_index_register_t index;

	if (pin >= 32) {
		LOG_ERR("Invalid gpio pin #%d", pin);
		return -EINVAL;
	}
	pin += pin_offset;
	index.value = 0;
	index.fields.index_type = ASPEED_GPIO_DEBOUCE;
	index.fields.index_command = ASPEED_GPIO_INDEX_WRITE;
	index.fields.index_data = en ? 2 : 0;
	index.fields.index_number = pin;
	LOG_DBG("gpio index = 0x%08x\n", index.value);
	gpio_reg->index.value = index.value;

	return 0;
}

static int gpio_aspeed_config(const struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	int ret;
	uint32_t io_flags;
	struct gpio_aspeed_data *data = DEV_DATA(dev);
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;

	ret = pinmux_pin_set(data->pinmux, pin_offset + pin, SIG_GPIO);
	if (ret) {
		return ret;
	}
	/* Does not support disconnected pin, and
	 * not supporting both input/output at same time.
	 */
	io_flags = flags & (GPIO_INPUT | GPIO_OUTPUT);
	if ((io_flags == GPIO_DISCONNECTED)
	    || (io_flags == (GPIO_INPUT | GPIO_OUTPUT))) {
		return -ENOTSUP;
	}

	/* Does not support pull-up/pull-down */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	if (flags & GPIO_SINGLE_ENDED) {
		if (flags & GPIO_LINE_OPEN_DRAIN) {
			/* connect to ground or disconnected */
			ret = gpio_aspeed_port_set_bits_raw(dev, BIT(pin));
		} else {
			/* connect to power supply or disconnected */
			ret = gpio_aspeed_port_clear_bits_raw(dev, BIT(pin));
		}
	} else {
		/* do nothing */
	}

	if (flags & GPIO_OUTPUT) {
		/* Set output pin initial value */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			ret = gpio_aspeed_port_set_bits_raw(dev, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			ret = gpio_aspeed_port_clear_bits_raw(dev, BIT(pin));
		}
		/* Set pin direction to output */
		ret = gpio_aspeed_set_direction(dev, pin, 1);
	} else if (flags & GPIO_INPUT) { /* Input */
		if (flags & GPIO_INT_DEBOUNCE) {
			ret = gpio_aspeed_deb_en(dev, pin, true);
		} else {
			ret = gpio_aspeed_deb_en(dev, pin, false);
		}
		/* Set pin direction to input */
		ret = gpio_aspeed_set_direction(dev, pin, 0);
	}
	return ret;
}

/* GPIO driver registration */
static const struct gpio_driver_api gpio_aspeed_driver = {
	.pin_configure = gpio_aspeed_config,
	.port_get_raw = gpio_aspeed_port_get_raw,
	.port_set_masked_raw = gpio_aspeed_port_set_masked_raw,
	.port_set_bits_raw = gpio_aspeed_port_set_bits_raw,
	.port_clear_bits_raw = gpio_aspeed_port_clear_bits_raw,
	.port_toggle_bits = gpio_aspeed_port_toggle_bits,
	.pin_interrupt_configure = gpio_aspeed_pin_interrupt_configure,
	.manage_callback = gpio_aspeed_manage_callback,
};

static int gpio_aspeed_persist_set(const struct device *dev, gpio_pin_t pin, bool en)
{
	volatile gpio_register_t *gpio_reg = DEV_CFG(dev)->base;
	uint8_t pin_offset = DEV_CFG(dev)->pin_offset;
	gpio_index_register_t index;

	if (pin >= 32) {
		LOG_ERR("Invalid gpio pin #%d", pin);
		return -EINVAL;
	}
	pin += pin_offset;
	index.value = 0;
	index.fields.index_type = ASPEED_GPIO_TOLERANCE;
	index.fields.index_command = ASPEED_GPIO_INDEX_WRITE;
	index.fields.index_data = en;
	index.fields.index_number = pin;
	LOG_DBG("gpio index = 0x%08x\n", index.value);
	gpio_reg->index.value = index.value;

	return 0;
}

static int gpio_aspeed_persist_init(const struct device *dev)
{
	uint32_t persist_maps = DEV_CFG(dev)->persist_maps;
	uint32_t pin = 0;
	int ret;

	for (pin = 0; pin < 32; pin++) {
		ret = gpio_aspeed_persist_set(dev, pin, persist_maps & BIT(pin) ? 1 : 0);
		if (ret) {
			return ret;
		}
	}
	return 0;
}

int gpio_aspeed_init(const struct device *dev)
{
	struct gpio_aspeed_data *data = DEV_DATA(dev);
	int ret;

	data->pinmux = DEVICE_DT_GET(DT_NODELABEL(pinmux));
	ret = gpio_aspeed_init_cmd_src(dev);
	if (ret) {
		return ret;
	}
	ret = gpio_aspeed_persist_init(dev);

	return ret;
}

static void gpio_aspeed_init_cmd_src_sel(const struct device *parent)
{
	volatile gpio_register_t *gpio_reg = DEV_PARENT_CFG(parent)->base;
	gpio_cmd_src_sel_t cmd_src_sel;

	cmd_src_sel.value = gpio_reg->cmd_src_sel.value;
	cmd_src_sel.fields.mst1 = ASPEED_GPIO_SEL_PRI;
	cmd_src_sel.fields.mst2 = ASPEED_GPIO_SEL_LPC;
	cmd_src_sel.fields.mst3 = ASPEED_GPIO_SEL_SSP;
	cmd_src_sel.fields.mst4 = ASPEED_GPIO_SEL_PRI;
	cmd_src_sel.fields.mst5 = ASPEED_GPIO_SEL_PRI;
	cmd_src_sel.fields.lock = 1;
	gpio_reg->cmd_src_sel.value = cmd_src_sel.value;
}

static int gpio_aspeed_usec_to_cycles(const struct device *parent, uint32_t us, uint32_t *cycles)
{
	uint32_t clk_rate;
	uint64_t temp_64;

	clock_control_get_rate(DEV_PARENT_CFG(parent)->clock_dev, DEV_PARENT_CFG(parent)->clk_id,
			       &clk_rate);
	temp_64 = ceiling_fraction(((uint64_t)us * clk_rate), USEC_PER_SEC);
	if (temp_64 > GENMASK(23, 0)) {
		return -ERANGE;
	}
	*cycles = (uint32_t)temp_64;

	return 0;
}

static int gpio_aspeed_deb_init(const struct device *parent, uint32_t us)
{
	volatile gpio_register_t *gpio_reg = DEV_PARENT_CFG(parent)->base;
	uint32_t cycles;
	int ret;

	ret = gpio_aspeed_usec_to_cycles(parent, us, &cycles);
	if (ret) {
		LOG_ERR("Err: %d", ret);
		return ret;
	}
	LOG_DBG("Init debounce timer for waiting %dus(%d cycles)", us, cycles);
	gpio_reg->debounce_time[0].fields.debounce_time = cycles;

	return 0;
}

int gpio_aspeed_parent_init(const struct device *parent)
{
	const struct gpio_aspeed_parent_config *cfg = DEV_PARENT_CFG(parent);
	int ret;

	gpio_aspeed_init_cmd_src_sel(parent);
	ret = gpio_aspeed_deb_init(parent, cfg->deb_interval);
	irq_connect_dynamic(cfg->irq_num, cfg->irq_prio, gpio_aspeed_isr, parent, 0);
	irq_enable(cfg->irq_num);

	return ret;
}

struct device_cont {
	const struct gpio_aspeed_config *cfg;
	struct gpio_aspeed_data *data;
};

#define GPIO_ENUM(node_id) node_id,

#define GPIO_ASPEED_DEV_DATA(node_id) {},
#define GPIO_ASPEED_DEV_CFG(node_id) {					      \
		.common = {						      \
			.port_pin_mask =				      \
				GPIO_PORT_PIN_MASK_FROM_DT_NODE(node_id) &    \
				~(DT_PROP_OR(node_id, gpio_reserved, 0)),     \
		},							      \
		.parent = DEVICE_DT_GET(DT_PARENT(node_id)),		      \
		.base = (gpio_register_t *)DT_REG_ADDR(DT_PARENT(node_id)),   \
		.pin_offset = DT_PROP(node_id, pin_offset),		      \
		.gpio_master = DT_PROP(node_id, aspeed_cmd_src),	      \
		.group_dedicated = DT_PROP(node_id, aspeed_group_dedicated) & \
				   GENMASK(3, 0),			      \
		.persist_maps = DT_PROP(node_id, aspeed_persist_maps),	      \
},

#define GPIO_ASPEED_DT_DEFINE(node_id)                                                             \
	DEVICE_DT_DEFINE(node_id, gpio_aspeed_init, NULL, &DT_PARENT(node_id).data[node_id],       \
			 &DT_PARENT(node_id).cfg[node_id], POST_KERNEL, 0, &gpio_aspeed_driver);

#define GPIO_ASPEED_DEV_DECLARE(node_id) { .dev = DEVICE_DT_GET(node_id) },

#define ASPEED_GPIO_DEVICE_INIT(inst)                                                              \
	static struct device_array child_dev_##inst[] = { DT_FOREACH_CHILD(                        \
		DT_DRV_INST(inst), GPIO_ASPEED_DEV_DECLARE) };                                     \
	static const struct gpio_aspeed_parent_config gpio_aspeed_parent_cfg_##inst = {            \
		.base = (gpio_register_t *)DT_INST_REG_ADDR(inst),                                 \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clk_id),               \
		.irq_num = DT_INST_IRQN(inst),                                                     \
		.irq_prio = DT_INST_IRQ(inst, priority),                                           \
		.child_dev = child_dev_##inst,                                                     \
		.child_num = ARRAY_SIZE(child_dev_##inst),                                         \
		.deb_interval = DT_INST_PROP(inst, aspeed_deb_interval_us),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_aspeed_parent_init, NULL, NULL,                           \
			      &gpio_aspeed_parent_cfg_##inst, POST_KERNEL,                         \
			      CONFIG_GPIO_ASPEED_INIT_PRIORITY, NULL);                             \
	static const struct gpio_aspeed_config gpio_aspeed_cfg_##inst[] = { DT_FOREACH_CHILD(      \
		DT_DRV_INST(inst), GPIO_ASPEED_DEV_CFG) };                                         \
	static struct gpio_aspeed_data gpio_aspeed_data_##inst[] = { DT_FOREACH_CHILD(             \
		DT_DRV_INST(inst), GPIO_ASPEED_DEV_DATA) };                                        \
	static const struct device_cont DT_DRV_INST(inst) = {                                      \
		.cfg = gpio_aspeed_cfg_##inst,                                                     \
		.data = gpio_aspeed_data_##inst,                                                   \
	};                                                                                         \
	enum { DT_FOREACH_CHILD(DT_DRV_INST(inst), GPIO_ENUM) };                                   \
	DT_FOREACH_CHILD(DT_DRV_INST(inst), GPIO_ASPEED_DT_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(ASPEED_GPIO_DEVICE_INIT)