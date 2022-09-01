/*
 * Copyright (c) ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ecdsa
#include <drivers/clock_control.h>
#include <drivers/reset_control.h>
#include <string.h>
#include <logging/log.h>
#include <crypto/ecdsa.h>
#include <stdint.h>
#include <soc.h>
#include "ecdsa_aspeed_priv.h"
LOG_MODULE_REGISTER(ecdsa_aspeed, CONFIG_LOG_DEFAULT_LEVEL);

#define ASPEED_SEC_STS		0x14
#define ASPEED_ECDSA_CMD	0xbc
#define ASPEED_ECDSA_PAR_GX	0xa00
#define ASPEED_ECDSA_PAR_GY	0xa40
#define ASPEED_ECDSA_PAR_P	0xa80
#define ASPEED_ECDSA_PAR_N	0xac0

static uintptr_t ecdsa_base;
static uintptr_t sram_base;
#define SEC_RD(reg) sys_read32(ecdsa_base + (reg))
#define SEC_WR(val, reg) sys_write32(val, ecdsa_base + (reg))
#define SRAM_WR(val, reg) sys_write32(val, sram_base + (reg))

/* Device config */
struct ecdsa_config {
	uintptr_t base; /* ECDSA engine base address */
	uintptr_t sbase; /* ECDSA engine sram base address */
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
};

#define DEV_CFG(dev)				 \
	((const struct ecdsa_config *const) \
	 (dev)->config)

static struct aspeed_ecdsa_drv_state drv_state NON_CACHED_BSS_ALIGN16;

static int aspeed_ecdsa_verify_trigger(const struct device *dev,
									   char *m, char *r, char *s,
									   char *qx, char *qy)
{
	uint32_t sts;
	int i;
	int ret;

	SEC_WR(0x0100f00b, 0x7c);
	SEC_WR(0x1, 0xb4);
	k_usleep(1000);

	/* Gx */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(SEC_RD(ASPEED_ECDSA_PAR_GX + i), 0x2000 + i);

	/* Gy */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(SEC_RD(ASPEED_ECDSA_PAR_GY + i), 0x2040 + i);

	/* p */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(SEC_RD(ASPEED_ECDSA_PAR_P + i), 0x2100 + i);

	/* n */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(SEC_RD(ASPEED_ECDSA_PAR_N + i), 0x2180 + i);

	/* a = 0 */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(0, 0x2140 + i);

	SEC_WR(0x0300f00b, 0x7c);

	/* Public key, Qx */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(*(uint32_t *)(qx + i), 0x2080 + i);

	/* Public key, Qy */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(*(uint32_t *)(qy + i), 0x20c0 + i);

	/* signature, r */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(*(uint32_t *)(r + i), 0x21c0 + i);

	/* signature, s */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(*(uint32_t *)(s + i), 0x2200 + i);

	/* sha384, m */
	for (i = 0; i < 48; i += 4)
		SRAM_WR(*(uint32_t *)(m + i), 0x2240 + i);

	SEC_WR(0x0, 0x7c);

	/* write ECDSA instruction command */
	SRAM_WR(1, 0x23c0);

	/* trigger ECDSA engine */
	SEC_WR(2, ASPEED_ECDSA_CMD);
	k_usleep(5000);
	SEC_WR(0, ASPEED_ECDSA_CMD);

	do {
		k_usleep(10);
		sts = SEC_RD(ASPEED_SEC_STS);
	} while (!(sts & BIT(20)));

	ret = SEC_RD(ASPEED_SEC_STS);
	if (ret & BIT(21))
		return 0;

	return -1;
}

int aspeed_ecdsa_verify(struct ecdsa_ctx *ctx, struct ecdsa_pkt *pkt)
{
	struct ecdsa_key *key = &drv_state.data.key;

	if (pkt->r_len != 48 || pkt->s_len != 48 ||
		pkt->m_len != 48 || key->curve_id != ECC_CURVE_NIST_P384) {
		LOG_ERR("not support");
		return -EINVAL;

	}

	return aspeed_ecdsa_verify_trigger(ctx->device, pkt->m,
									   pkt->r, pkt->s,
									   key->qx, key->qy);
}

static int aspeed_ecdsa_session_setup(const struct device *dev,
									  struct ecdsa_ctx *ctx,
									  struct ecdsa_key *key)
{
	struct aspeed_ecdsa_ctx *data;

	ARG_UNUSED(dev);

	if (drv_state.in_use) {
		LOG_ERR("Peripheral in use");
		return -EBUSY;
	}

	data = &drv_state.data;
	drv_state.in_use = true;

	memcpy(&data->key, key, sizeof(struct ecdsa_key));

	ctx->ops.verify = aspeed_ecdsa_verify;
	ctx->device = dev;

	return 0;
}

static int aspeed_ecdsa_session_free(const struct device *dev,
									 struct ecdsa_ctx *ctx)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ctx);

	drv_state.in_use = false;

	return 0;
}

int ecdsa_init(const struct device *dev)
{
	drv_state.in_use = false;
	ecdsa_base = DEV_CFG(dev)->base;
	sram_base = DEV_CFG(dev)->sbase;

	return 0;
}

static struct ecdsa_driver_api ecdsa_funcs = {
	.begin_session = aspeed_ecdsa_session_setup,
	.free_session = aspeed_ecdsa_session_free,
	.query_hw_caps = NULL,
};

static const struct ecdsa_config ecdsa_aspeed_config = {
	.base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(ecdsa), 0),
	.sbase = DT_REG_ADDR_BY_IDX(DT_NODELABEL(ecdsa), 1),
};

DEVICE_DT_INST_DEFINE(0, ecdsa_init, NULL,
					  NULL, &ecdsa_aspeed_config,
					  POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
					  (void *)&ecdsa_funcs);
