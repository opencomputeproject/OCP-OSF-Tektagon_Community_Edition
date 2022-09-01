/*
 * Copyright (c) ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_rsa
#include <drivers/clock_control.h>
#include <drivers/reset_control.h>
#include <string.h>
#include <logging/log.h>
#include <crypto/rsa.h>
#include <stdint.h>
#include <soc.h>
#include "rsa_aspeed_priv.h"
LOG_MODULE_REGISTER(rsa_aspeed, CONFIG_LOG_DEFAULT_LEVEL);

#define ASPEED_SEC_STS					0x14
#define ASPEED_SEC_MCU_MEMORY_MODE		0x5c
#define ASPEED_SEC_RSA_KEY_LEN			0xb0
#define ASPEED_SEC_RSA_TRIG				0xbc
#define RSA_MAX_LEN						0x400

static uintptr_t rsa_base;
static uintptr_t sram_base;
#define SEC_RD(reg) sys_read32(rsa_base + (reg))
#define SEC_WR(val, reg) sys_write32(val, rsa_base + (reg))

/* Device config */
struct rsa_config {
	uintptr_t base; /* RSA engine base address */
	uintptr_t sbase; /* RSA engine sram base address */
	const struct device *clock_dev;
	const clock_control_subsys_t clk_id;
};

#define DEV_CFG(dev)				 \
	((const struct rsa_config *const) \
	 (dev)->config)

static struct aspeed_rsa_drv_state drv_state NON_CACHED_BSS_ALIGN16;

static int aspeed_rsa_trigger(const struct device *dev, char *data, uint32_t data_len, char *dst,
							  int *out_len, char *m, char *e,
							  uint32_t m_bits, uint32_t e_bits)
{
	uint32_t m_len = (m_bits + 7) / 8;
	uint32_t e_len = (e_bits + 7) / 8;
	uint32_t sts;
	char *sram = (char *)sram_base;
	char *s;
	int leading_zero;
	int i, j;
	int result_nbytes;

	if (data_len > 512) {
		LOG_ERR("the maximum rsa input data length is 4096\n");
		return -EINVAL;
	}

	memset(sram, 0, 0x1800);

	for (i = 0; i < e_len; i++) {
		sram[i] = e[e_len - i - 1];
	}

	s = sram + 0x400;
	for (i = 0; i < m_len; i++) {
		s[i] = m[m_len - i - 1];
	}

	s = sram + 0x800;
	for (i = 0; i < data_len; i++) {
		s[i] = data[data_len - i - 1];
	}

	SEC_WR(e_bits << 16 | m_bits, ASPEED_SEC_RSA_KEY_LEN);
	SEC_WR(1, ASPEED_SEC_RSA_TRIG);
	SEC_WR(0, ASPEED_SEC_RSA_TRIG);
	do {
		k_usleep(10);
		sts = SEC_RD(ASPEED_SEC_STS);
	} while (!(sts & BIT(4)));

	s = sram + 0x1400;
	i = 0;
	leading_zero = 1;
	result_nbytes = RSA_MAX_LEN;
	for (j = RSA_MAX_LEN - 1; j >= 0; j--) {
		if (s[j] == 0 && leading_zero) {
			result_nbytes--;
		} else {
			leading_zero = 0;
			dst[i] = s[j];
			i++;
		}
	}

	*out_len = result_nbytes;
	memset(sram, 0, 0x1800);

	return 0;
}

int aspeed_rsa_dec(struct rsa_ctx *ctx, struct rsa_pkt *pkt)
{
	struct rsa_key *key = &drv_state.data.key;

	return aspeed_rsa_trigger(ctx->device, pkt->in_buf, pkt->in_len,
							  pkt->out_buf, &pkt->out_len, key->m, key->d,
							  key->m_bits, key->d_bits);
}

int aspeed_rsa_enc(struct rsa_ctx *ctx, struct rsa_pkt *pkt)
{
	struct rsa_key *key = &drv_state.data.key;

	return aspeed_rsa_trigger(ctx->device, pkt->in_buf, pkt->in_len,
							  pkt->out_buf, &pkt->out_len, key->m, key->e,
							  key->m_bits, key->e_bits);
}

static int aspeed_rsa_session_setup(const struct device *dev,
									struct rsa_ctx *ctx,
									struct rsa_key *key)
{
	struct aspeed_rsa_ctx *data;

	ARG_UNUSED(dev);

	if (drv_state.in_use) {
		LOG_ERR("Peripheral in use");
		return -EBUSY;
	}

	data = &drv_state.data;
	drv_state.in_use = true;

	memcpy(&data->key, key, sizeof(struct rsa_key));

	ctx->ops.decrypt = aspeed_rsa_dec;
	ctx->ops.encrypt = aspeed_rsa_enc;
	ctx->ops.sign = aspeed_rsa_dec;
	ctx->ops.verify = aspeed_rsa_enc;
	ctx->device = dev;

	return 0;
}

static int aspeed_rsa_session_free(const struct device *dev,
								   struct rsa_ctx *ctx)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ctx);

	drv_state.in_use = false;

	return 0;
}

int rsa_init(const struct device *dev)
{
	const struct rsa_config *config = DEV_CFG(dev);

	clock_control_on(config->clock_dev, DEV_CFG(dev)->clk_id);
	drv_state.in_use = false;
	rsa_base = DEV_CFG(dev)->base;
	sram_base = DEV_CFG(dev)->sbase;

	return 0;
}

static struct rsa_driver_api rsa_funcs = {
	.begin_session = aspeed_rsa_session_setup,
	.free_session = aspeed_rsa_session_free,
	.query_hw_caps = NULL,
};

static const struct rsa_config rsa_aspeed_config = {
	.base = DT_REG_ADDR_BY_IDX(DT_NODELABEL(rsa), 0),
	.sbase = DT_REG_ADDR_BY_IDX(DT_NODELABEL(rsa), 1),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clk_id = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, clk_id),
};

DEVICE_DT_INST_DEFINE(0, rsa_init, NULL,
					  NULL, &rsa_aspeed_config,
					  POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
					  (void *)&rsa_funcs);
