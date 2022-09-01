/*
 * Copyright (c) 2021 Aspeed Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT aspeed_i2c_filter

#include <sys/util.h>
#include <sys/slist.h>
#include <kernel.h>
#include <errno.h>
#include <soc.h>
#include <device.h>
#include <string.h>
#include <stdlib.h>
#include <drivers/i2c/pfr/i2c_filter.h>

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(i2c_pfr_filter);

/* #define ASPEED_I2C_FJ_DUMP */
#define ASPEED_I2C_FW_DUMP

#ifdef ASPEED_I2C_FJ_DUMP
#define I2C_W_R(value, addr) LOG_INF("  dw %x %x", addr, value);
#define I2C_LW_R(value, addr) LOG_INF("  dw %x %lx", addr, value);
#else
#ifdef ASPEED_I2C_FW_DUMP
#define I2C_W_R(value, addr) LOG_INF("  dw %x %x", addr, value); sys_write32(value, addr);
#define I2C_LW_R(value, addr) LOG_INF("  dw %x %lx", addr, value); sys_write32(value, addr);
#else
#define I2C_W_R(value, addr) sys_write32(value, addr);
#define I2C_LW_R(value, addr) sys_write32(value, addr);
#endif
#endif

/* i2c filter buf */
struct ast_i2c_f_tbl filter_tbl[AST_I2C_F_COUNT] NON_CACHED_BSS_ALIGN16;

/* Filter child config */
struct ast_i2c_filter_child_config {
	char		*filter_dev_name; /* i2c filter device name */
	const struct device *parent; /* Parent device handler */
	uint8_t	index; /* i2c filter index */
	uint32_t	clock; /* i2c filter clock select */
};

struct ast_i2c_filter_child_data {
	uint32_t	filter_dev_base;	/* i2c filter device base */
	uint8_t	filter_dev_en;	/* i2cflt04 : filter enable */
	uint8_t	filter_en;		/* i2cflt0c : white list */
	uint8_t	filter_idx[AST_I2C_F_REMAP_SIZE];
	uint8_t	*filter_buf;
};

struct filter_array {
	const struct device *dev;
};

struct ast_i2c_filter_config {
	uint32_t	filter_g_base;
	struct filter_array *filter_child;
	uint32_t filter_child_num;
	void (*irq_config_func)(const struct device *dev);
};

/* convenience defines */
#define DEV_CFG(dev)				     \
	((const struct ast_i2c_filter_config *const) \
	 (dev)->config)
#define DEV_C_CFG(dev)				     \
		((const struct ast_i2c_filter_child_config *const) \
		 (dev)->config)
#define DEV_C_DATA(dev) \
	((struct ast_i2c_filter_child_data *const)(dev)->data)

/* i2c filter interrupt service routine */
void ast_i2c_filter_isr(const struct device *dev)
{
	const struct ast_i2c_filter_config *cfg = DEV_CFG(dev);

	uint8_t index = 0, i, infowp, inforp;
	uint32_t stsg = sys_read32(cfg->filter_g_base + AST_I2C_F_G_INT_STS);
	uint32_t filter_dev_base = 0, dev_base = 0, stsl = 0, sts = 0;
	uint32_t value = 0, count = 0;
	dev_base = cfg->filter_g_base + AST_I2C_F_D_BASE;

	/* find which one local filter interrupt status */
	for (index = 0; index < AST_I2C_F_COUNT; index++) {
		/* local filter */
		if (stsg & (1 << index)) {
			LOG_INF("%d flt block occur!", index);
			filter_dev_base = (dev_base + (index * AST_I2C_F_D_OFFSET));
			stsl = sys_read32(filter_dev_base + AST_I2C_F_INT_STS);

			if (stsl) {
				sts = sys_read32(filter_dev_base + AST_I2C_F_STS);

				infowp = (sts & 0xF000) >> 12;
				inforp = (sts & 0x0F00) >> 8;

				/* calculte the information count */
				if (infowp > inforp)
					count = infowp - inforp;
				else
					count = (infowp + 0x10) - inforp;

				/* read back from  */
				for (i = 0; i < count; i++) {
					value = sys_read32(filter_dev_base + AST_I2C_F_INFO);
					LOG_INF(" flt block %d info %x", i, value);
				}

				/* clear status */
				I2C_W_R(stsl, (filter_dev_base + AST_I2C_F_INT_STS));

			}
		}
	}
}


/* i2c filter default */
int ast_i2c_filter_default(const struct device *dev, uint8_t pass)
{
	const struct ast_i2c_filter_child_config *cfg = DEV_C_CFG(dev);

	uint8_t i;
	uint32_t value = 0;

	struct ast_i2c_f_tbl *dev_wl_tbl = &(filter_tbl[(cfg->index)]);
	struct ast_i2c_f_bitmap *bmp_buf = &(dev_wl_tbl->filter_tbl[0]);

	/* check parameter valid */
	if (!cfg->filter_dev_name) {
		LOG_ERR("i2c filter not found");
		return -EINVAL;
	}

	/* transation will be all pass */
	if (pass)
		value = 0xFFFFFFFF;

	/* fill pass or block bitmap table */
	for (i = 0; i < AST_I2C_F_ELEMENT_SIZE; i++) {
		bmp_buf->element[i] = value;
	}

	return 0;
}


/* i2c filter update */
int ast_i2c_filter_update(const struct device *dev, uint8_t idx, uint8_t addr,
struct ast_i2c_f_bitmap *table)
{
	struct ast_i2c_filter_child_data *data = DEV_C_DATA(dev);
	const struct ast_i2c_filter_child_config *cfg = DEV_C_CFG(dev);

	uint8_t i;
	uint8_t offset = idx >> 2;
	uint32_t *list_index = (uint32_t *)(data->filter_idx);

	struct ast_i2c_f_tbl *dev_wl_tbl = &(filter_tbl[(cfg->index)]);
	struct ast_i2c_f_bitmap *bmp_buf = &(dev_wl_tbl->filter_tbl[0]);

	/* check parameter valid */
	if (!cfg->filter_dev_name) {
		LOG_ERR("i2c filter not found");
		return -EINVAL;
	} else if (!data->filter_dev_base) {
		LOG_ERR("i2c filter not be initial");
		return -EINVAL;
	} else if (idx > 15) {
		LOG_ERR("i2c filter index invalid");
		return -EINVAL;
	} else if (table == NULL) {
		LOG_ERR("i2c filter bitmap table is NULL");
		return -EINVAL;
	}

	/* fill re-map table and find the value pointer */
	data->filter_idx[idx] = addr;
	list_index += offset;

	switch (offset) {
	case 0:
		I2C_W_R((*list_index), (data->filter_dev_base + AST_I2C_F_MAP0));
		break;
	case 1:
		I2C_W_R((*list_index), (data->filter_dev_base + AST_I2C_F_MAP1));
		break;
	case 2:
		I2C_W_R((*list_index), (data->filter_dev_base + AST_I2C_F_MAP2));
		break;
	case 3:
		I2C_W_R((*list_index), (data->filter_dev_base + AST_I2C_F_MAP3));
		break;
	default:
		LOG_ERR("i2c filter invalid re-map index");
		return -EINVAL;
	}

	/* fill pass or block bitmap table */
	bmp_buf += (idx + 1);
	for (i = 0; i < AST_I2C_F_ELEMENT_SIZE; i++) {
		bmp_buf->element[i] = table->element[i];
	}

	return 0;
}

/* i2c filter enable */
int ast_i2c_filter_en(const struct device *dev, uint8_t filter_en, uint8_t wlist_en,
uint8_t clr_idx, uint8_t clr_tbl)
{
	struct ast_i2c_filter_child_data *data = DEV_C_DATA(dev);
	const struct ast_i2c_filter_child_config *cfg = DEV_C_CFG(dev);

	/* check parameter valid */
	if (!cfg->filter_dev_name) {
		LOG_ERR("i2c filter not found");
		return -EINVAL;
	} else if (!data->filter_dev_base) {
		LOG_ERR("i2c filter not be initial");
		return -EINVAL;
	}

	data->filter_dev_en = filter_en;
	data->filter_en = wlist_en;

	LOG_DBG("i2c filter tbl : %x", (uint32_t)(&(filter_tbl[(cfg->index)])));

	/* set white list buffer into device */
	if ((data->filter_dev_en) && (data->filter_en)) {
		I2C_LW_R(TO_PHY_ADDR(&(filter_tbl[(cfg->index)])),
		(data->filter_dev_base+AST_I2C_F_BUF));
	}

	/* clear re-map index */
	if (clr_idx) {
		I2C_W_R(0x0, (data->filter_dev_base+AST_I2C_F_MAP0));
		I2C_W_R(0x0, (data->filter_dev_base+AST_I2C_F_MAP1));
		I2C_W_R(0x0, (data->filter_dev_base+AST_I2C_F_MAP2));
		I2C_W_R(0x0, (data->filter_dev_base+AST_I2C_F_MAP3));
	}

	/* clear white list table */
	if (clr_tbl) {
		I2C_W_R(0, (data->filter_dev_base+AST_I2C_F_BUF));
		data->filter_en = 0x0;
	}

	/* apply filter setting */
	I2C_W_R(data->filter_dev_en, (data->filter_dev_base+AST_I2C_F_EN));
	I2C_W_R(data->filter_en, (data->filter_dev_base+AST_I2C_F_CFG));

	return 0;
}

/* i2c filter initial */
int ast_i2c_filter_init(const struct device *dev)
{
	const struct ast_i2c_filter_child_config *cfg = DEV_C_CFG(dev);
	struct ast_i2c_filter_child_data *data = DEV_C_DATA(dev);
	const struct ast_i2c_filter_config *gcfg = DEV_CFG(cfg->parent);
	uint32_t ginten;

	/* check parameter valid */
	if (!cfg->filter_dev_name) {
		LOG_ERR("i2c filter not found");
		return -EINVAL;
	} else if (!data->filter_dev_base) {
		LOG_ERR("i2c filter not be initial");
		return -EINVAL;
	}

	/* close filter first and fill initial timing setting */
	data->filter_dev_en = 0;
	I2C_W_R(0, (data->filter_dev_base+AST_I2C_F_EN));
	data->filter_en = 0;
	I2C_W_R(0, (data->filter_dev_base+AST_I2C_F_CFG));

	if (cfg->clock == 100) {
		I2C_W_R(AST_I2C_F_100_TIMING_VAL,
		(data->filter_dev_base+AST_I2C_F_TIMING));
	} else if (cfg->clock == 400) {
		I2C_W_R(AST_I2C_F_400_TIMING_VAL,
		(data->filter_dev_base+AST_I2C_F_TIMING));
	} else {
		LOG_ERR("i2c filter bad clock setting");
		return -EINVAL;
	}

	/* clear and enable local interrupt */
	I2C_W_R(0x1, (data->filter_dev_base+AST_I2C_F_INT_STS));
	I2C_W_R(0x1, (data->filter_dev_base+AST_I2C_F_INT_EN));

	/* enable global interrupt */
	ginten = sys_read32(gcfg->filter_g_base + AST_I2C_F_G_INT_EN);
	ginten |= (0x1 << (cfg->index));
	I2C_W_R(ginten, (gcfg->filter_g_base + AST_I2C_F_G_INT_EN));

	return 0;
}

/* i2c filter child initial */
int ast_i2c_filter_child_init(const struct device *dev)
{
	const struct ast_i2c_filter_child_config *cfg = DEV_C_CFG(dev);
	struct ast_i2c_filter_child_data *data = DEV_C_DATA(dev);
	const struct ast_i2c_filter_config *gcfg = DEV_CFG(cfg->parent);
	int i = 0;

	/* assign filter device base */
	data->filter_dev_base = (gcfg->filter_g_base + AST_I2C_F_D_BASE);
	data->filter_dev_base += (cfg->index * AST_I2C_F_D_OFFSET);

	LOG_DBG("Filter Base is %x", data->filter_dev_base);
	LOG_DBG("Filter Index is %d", cfg->index);

	/* clear filter re-map index table */
	for (i = 0; i < AST_I2C_F_REMAP_SIZE; i++) {
		data->filter_idx[i] = 0x0;
	}

	/* initial filter buffer */
	data->filter_buf = NULL;

	return 0;
}

/* i2c filter initial */
int ast_i2c_filter_global_init(const struct device *dev)
{
	const struct ast_i2c_filter_config *cfg = DEV_CFG(dev);

	/* hook interrupt routine*/
	cfg->irq_config_func(dev);

	return 0;
}

struct filter_info {
	const struct ast_i2c_filter_child_config *cfg;
	struct ast_i2c_filter_child_data *data;
};

#define FILTER_ENUM(node_id) node_id,
#define ASPEED_FILTER_CHILD_DATA(node_id) {},
#define ASPEED_FILTER_CHILD_CFG(node_id) {			\
		.filter_dev_name = DT_PROP(node_id, label),	\
		.index = DT_PROP(node_id, index),	\
		.clock = DT_PROP(node_id, clock),	\
		.parent = DEVICE_DT_GET(DT_PARENT(node_id)),	\
},
#define ASPEED_FILTER_CHILD_DEFINE(node_id)				\
	DEVICE_DT_DEFINE(node_id, ast_i2c_filter_child_init, NULL, \
	&DT_PARENT(node_id).data[node_id],		\
	&DT_PARENT(node_id).cfg[node_id], POST_KERNEL, 0, NULL);

#define ASPEED_FILTER_CHILD_DEV(node_id) { .dev = DEVICE_DT_GET(node_id) },

#define I2C_FILTER_INIT(inst)				 \
	static struct filter_array child_filter_##inst[] = { DT_FOREACH_CHILD( \
		DT_DRV_INST(inst), ASPEED_FILTER_CHILD_DEV) }; \
	static const struct ast_i2c_filter_child_config \
	aspeed_filter_child_cfg_##inst[] = { DT_FOREACH_CHILD( \
	DT_DRV_INST(inst), ASPEED_FILTER_CHILD_CFG) }; \
	static struct ast_i2c_filter_child_data \
	aspeed_filter_child_data_##inst[] = { DT_FOREACH_CHILD( \
	DT_DRV_INST(inst), ASPEED_FILTER_CHILD_DATA) }; \
	\
	static const struct filter_info DT_DRV_INST(inst) = {  \
		.cfg = aspeed_filter_child_cfg_##inst,             \
		.data = aspeed_filter_child_data_##inst,           \
	};										\
	static void ast_i2c_filter_cfg_##inst(const struct device *dev);	 \
	\
	static const struct ast_i2c_filter_config		 \
		ast_i2c_filter_##inst##_cfg = {		 \
		.filter_g_base = DT_INST_REG_ADDR(inst),	 \
		.filter_child = child_filter_##inst,	 \
		.filter_child_num = ARRAY_SIZE(child_filter_##inst), \
		.irq_config_func = ast_i2c_filter_cfg_##inst,	 \
	};\
	\
	DEVICE_DT_INST_DEFINE(inst, \
			      &ast_i2c_filter_global_init, \
			      NULL, \
			      NULL, \
			      &ast_i2c_filter_##inst##_cfg, \
			      POST_KERNEL, \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      NULL); \
	enum { DT_FOREACH_CHILD(DT_DRV_INST(inst), FILTER_ENUM) }; \
	DT_FOREACH_CHILD(DT_DRV_INST(inst), ASPEED_FILTER_CHILD_DEFINE)\
	\
	static void ast_i2c_filter_cfg_##inst(const struct device *dev) \
	{									 \
		ARG_UNUSED(dev);					 \
	\
		IRQ_CONNECT(DT_INST_IRQN(inst),	 \
				DT_INST_IRQ(inst, priority),	 \
				ast_i2c_filter_isr, DEVICE_DT_INST_GET(inst), 0); \
	\
		irq_enable(DT_INST_IRQN(inst));		 \
	}

DT_INST_FOREACH_STATUS_OKAY(I2C_FILTER_INIT)
