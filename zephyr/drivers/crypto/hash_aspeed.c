/*
 * Copyright (c) 2021 ASPEED Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <sys/__assert.h>
#include <crypto/hash.h>
#include <logging/log.h>
#include <sys/byteorder.h>

#include "hace_aspeed.h"
#include "hash_aspeed_priv.h"

LOG_MODULE_REGISTER(hash_aspeed, CONFIG_CRYPTO_LOG_LEVEL);

static const uint32_t sha256_iv[8] = {
	0x67e6096aUL, 0x85ae67bbUL, 0x72f36e3cUL, 0x3af54fa5UL,
	0x7f520e51UL, 0x8c68059bUL, 0xabd9831fUL, 0x19cde05bUL
};

static const uint32_t sha384_iv[16] = {
	0x5d9dbbcbUL, 0xd89e05c1UL, 0x2a299a62UL, 0x07d57c36UL,
	0x5a015991UL, 0x17dd7030UL, 0xd8ec2f15UL, 0x39590ef7UL,
	0x67263367UL, 0x310bc0ffUL, 0x874ab48eUL, 0x11155868UL,
	0x0d2e0cdbUL, 0xa78ff964UL, 0x1d48b547UL, 0xa44ffabeUL
};

static const uint32_t sha512_iv[16] = {
	0x67e6096aUL, 0x08c9bcf3UL, 0x85ae67bbUL, 0x3ba7ca84UL,
	0x72f36e3cUL, 0x2bf894feUL, 0x3af54fa5UL, 0xf1361d5fUL,
	0x7f520e51UL, 0xd182e6adUL, 0x8c68059bUL, 0x1f6c3e2bUL,
	0xabd9831fUL, 0x6bbd41fbUL, 0x19cde05bUL, 0x79217e13UL
};

static struct aspeed_hash_drv_state drv_state NON_CACHED_BSS_ALIGN16;

static int aspeed_hash_wait_completion(int timeout_ms)
{
	struct hace_register_s *hace_register = hace_eng.base;
	union hace_sts_s hace_sts;
	int ret;

	ret = reg_read_poll_timeout(hace_register, hace_sts, hace_sts,
								hace_sts.fields.hash_int, 1, timeout_ms);
	if (ret)
		LOG_ERR("HACE poll timeout\n");
	return ret;
}

static void aspeed_ahash_fill_padding(struct aspeed_hash_ctx *ctx, unsigned int remainder)
{
	unsigned int index, padlen;
	uint64_t bits[2];

	if (ctx->block_size == 64) {
		bits[0] = sys_cpu_to_be64(ctx->digcnt[0] << 3);
		index = (ctx->bufcnt + remainder) & 0x3f;
		padlen = (index < 56) ? (56 - index) : ((64 + 56) - index);
		*(ctx->buffer + ctx->bufcnt) = 0x80;
		memset(ctx->buffer + ctx->bufcnt + 1, 0, padlen - 1);
		memcpy(ctx->buffer + ctx->bufcnt + padlen, bits, 8);
		ctx->bufcnt += padlen + 8;
	} else {
		bits[1] = sys_cpu_to_be64(ctx->digcnt[0] << 3);
		bits[0] = sys_cpu_to_be64(ctx->digcnt[1] << 3 | ctx->digcnt[0] >> 61);
		index = (ctx->bufcnt + remainder) & 0x7f;
		padlen = (index < 112) ? (112 - index) : ((128 + 112) - index);
		*(ctx->buffer + ctx->bufcnt) = 0x80;
		memset(ctx->buffer + ctx->bufcnt + 1, 0, padlen - 1);
		memcpy(ctx->buffer + ctx->bufcnt + padlen, bits, 16);
		ctx->bufcnt += padlen + 16;
	}
}

static int hash_trigger(struct aspeed_hash_ctx *data, int hash_len)
{
	struct hace_register_s *hace_register = hace_eng.base;

	if (hace_register->hace_sts.fields.hash_engine_sts) {
		LOG_ERR("HACE error: engine busy\n");
		return -EBUSY;
	}

	/* Clear pending completion status */
	hace_register->hace_sts.value = HACE_HASH_ISR;

	hace_register->hash_data_src.value = (uint32_t)data->sg;
	hace_register->hash_dgst_dst.value = (uint32_t)data->digest;
	hace_register->hash_key_buf.value = (uint32_t)data->digest;
	hace_register->hash_data_len.value = hash_len;
	hace_register->hash_cmd_reg.value = data->method;

	return aspeed_hash_wait_completion(3000);
}

static int aspeed_hash_update(struct hash_ctx *ctx, struct hash_pkt *pkt)
{
	struct aspeed_hash_ctx *data = &drv_state.data;
	struct aspeed_sg *sg = data->sg;
	int rc;
	int remainder;
	int total_len;
	int i;

	data->digcnt[0] += pkt->in_len;
	if (data->digcnt[0] < pkt->in_len)
		data->digcnt[1]++;

	if (data->bufcnt + pkt->in_len < data->block_size) {
		memcpy(data->buffer + data->bufcnt, pkt->in_buf, pkt->in_len);
		data->bufcnt += pkt->in_len;
		return 0;
	}
	remainder = (pkt->in_len + data->bufcnt) % data->block_size;
	total_len = pkt->in_len + data->bufcnt - remainder;
	i = 0;
	if (data->bufcnt != 0) {
		sg[0].addr = (uint32_t)data->buffer;
		sg[0].len = data->bufcnt;
		if (total_len == data->bufcnt)
			sg[0].len |= HACE_SG_LAST;
		i++;
	}

	if (total_len != data->bufcnt) {
		sg[i].addr = (uint32_t)pkt->in_buf;
		sg[i].len = (total_len - data->bufcnt) | HACE_SG_LAST;
	}

	rc = hash_trigger(data, total_len);
	if (remainder != 0) {
		memcpy(data->buffer, pkt->in_buf + (total_len - data->bufcnt), remainder);
		data->bufcnt = remainder;
	}
	if (rc)
		return rc;

	return 0;
}

static int aspeed_hash_final(struct hash_ctx *ctx, struct hash_pkt *pkt)
{
	struct aspeed_hash_ctx *data = &drv_state.data;
	struct aspeed_sg *sg = data->sg;
	int rc;

	if (pkt->out_buf_max < ctx->digest_size) {
		LOG_ERR("HACE error: insufficient size on destination buffer\n");
		return -EINVAL;
	}
	aspeed_ahash_fill_padding(data, 0);

	sg[0].addr = (uint32_t)data->buffer;
	sg[0].len = data->bufcnt | HACE_SG_LAST;

	rc = hash_trigger(data, data->bufcnt);
	if (rc) {
		return rc;
	}
	memcpy(pkt->out_buf, data->digest, ctx->digest_size);

	return 0;
}

static int aspeed_hash_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	drv_state.in_use = false;

	return 0;
}

static int aspeed_hash_session_setup(const struct device *dev,
									 struct hash_ctx *ctx,
									 enum hash_algo algo)
{
	struct aspeed_hash_ctx *data;

	ARG_UNUSED(dev);

	if (drv_state.in_use) {
		LOG_ERR("Peripheral in use");
		return -EBUSY;
	}

	data = &drv_state.data;
	drv_state.in_use = true;

	data->method = HASH_CMD_ACC_MODE | HACE_SHA_BE_EN | HACE_SG_EN;
	switch (algo) {
	case HASH_SHA256:
		ctx->digest_size = 32;
		data->block_size = 64;
		data->method |= HACE_ALGO_SHA256;
		memcpy(data->digest, sha256_iv, 32);
		break;
	case HASH_SHA384:
		ctx->digest_size = 48;
		data->block_size = 128;
		data->method |= HACE_ALGO_SHA384;
		memcpy(data->digest, sha384_iv, 64);
		break;
	case HASH_SHA512:
		ctx->digest_size = 64;
		data->block_size = 128;
		data->method |= HACE_ALGO_SHA512;
		memcpy(data->digest, sha512_iv, 64);
		break;
	default:
		LOG_ERR("ASPEED HASH Unsupported mode");
		return -EINVAL;
	}
	ctx->ops.update_hndlr = aspeed_hash_update;
	ctx->ops.final_hndlr = aspeed_hash_final;

	data->bufcnt = 0;
	data->digcnt[0] = 0;
	data->digcnt[1] = 0;

	return 0;
}

static int aspeed_hash_session_free(const struct device *dev,
									struct hash_ctx *ctx)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(ctx);

	drv_state.in_use = false;

	return 0;
}

static struct hash_driver_api hash_funcs = {
	.begin_session = aspeed_hash_session_setup,
	.free_session = aspeed_hash_session_free,
	.query_hw_caps = NULL,
};

DEVICE_DEFINE(hash_aspeed, CONFIG_CRYPTO_ASPEED_HASH_DRV_NAME,
			  aspeed_hash_init, NULL, NULL, NULL,
			  POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
			  (void *)&hash_funcs);
