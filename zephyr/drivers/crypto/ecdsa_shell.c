/*
 * Copyright (c) 2021 ASPEED
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#ifdef CONFIG_ECDSA_ASPEED
	#define ECDSA_DRV_NAME DT_LABEL(DT_INST(0, aspeed_ecdsa))
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shell/shell.h>
#include <crypto/ecdsa_structs.h>
#include <crypto/ecdsa.h>
#include "mbedtls/ecdsa.h"

struct ecdsa_testvec {
	const unsigned char *Qx;
	const unsigned char *Qy;
	const unsigned char *d;
	const unsigned char *r;
	const unsigned char *s;
	const unsigned char *m;
	unsigned int m_size;
};

static const struct ecdsa_testvec secp384r1[] = {
	{
		.Qx =
		"\x3B\xF7\x01\xBC\x9E\x9D\x36\xB4\xD5\xF1\x45\x53\x43\xF0\x91\x26"
		"\xF2\x56\x43\x90\xF2\xB4\x87\x36\x50\x71\x24\x3C\x61\xE6\x47\x1F"
		"\xB9\xD2\xAB\x74\x65\x7B\x82\xF9\x08\x64\x89\xD9\xEF\x0F\x5C\xB5",
		.Qy =
		"\xD1\xA3\x58\xEA\xFB\xF9\x52\xE6\x8D\x53\x38\x55\xCC\xBD\xAA\x6F"
		"\xF7\x5B\x13\x7A\x51\x01\x44\x31\x99\x32\x55\x83\x55\x2A\x62\x95"
		"\xFF\xE5\x38\x2D\x00\xCF\xCD\xA3\x03\x44\xA9\xB5\xB6\x8D\xB8\x55",
		.r =
		"\x30\xEA\x51\x4F\xC0\xD3\x8D\x82\x08\x75\x6F\x06\x81\x13\xC7\xCA"
		"\xDA\x9F\x66\xA3\xB4\x0E\xA3\xB3\x13\xD0\x40\xD9\xB5\x7D\xD4\x1A"
		"\x33\x27\x95\xD0\x2C\xC7\xD5\x07\xFC\xEF\x9F\xAF\x01\xA2\x70\x88",
		.s =
		"\x69\x1B\x9D\x49\x69\x45\x1A\x98\x03\x6D\x53\xAA\x72\x54\x58\x60"
		"\x21\x25\xDE\x74\x88\x1B\xBC\x33\x30\x12\xCA\x4F\xA5\x5B\xDE\x39"
		"\xD1\xBF\x16\xA6\xAA\xE3\xFE\x49\x92\xC5\x67\xC6\xE7\x89\x23\x37",
		.m =
		"\xF4\x92\xB9\xEB\x18\xA0\x6F\x7A\xA4\x79\x95\x3B\x31\xC3\x4F\xBF"
		"\xFC\xF4\x2A\x74\x27\xB5\xD2\xEF\xF0\x45\xDD\x61\x62\xB2\x4B\xCC"
		"\x37\xDA\x1A\xA7\x72\x5E\xD7\x1A\x65\x0E\xAB\x7D\xE7\x58\xFE\xFF",
		.m_size = 48
	}
};

static void dump_buf(const struct shell *shell, const char *title,
					 const unsigned char *buf, size_t len)
{
	size_t i;

	shell_fprintf(shell, SHELL_NORMAL, "%s", title);
	for (i = 0; i < len; i++)
		shell_fprintf(shell, SHELL_NORMAL, "%c%c", "0123456789ABCDEF"[buf[i] / 16],
					  "0123456789ABCDEF"[buf[i] % 16]);
	shell_fprintf(shell, SHELL_NORMAL, "\n");
}

static void dump_pubkey(const struct shell *shell, const char *title, mbedtls_ecdsa_context *key)
{
	unsigned char buf[300];
	size_t len;

	if (mbedtls_ecp_point_write_binary(&key->grp, &key->Q,
									   MBEDTLS_ECP_PF_UNCOMPRESSED, &len, buf, sizeof(buf)) != 0) {
		shell_fprintf(shell, SHELL_NORMAL, "internal error\n");
		return;
	}
	dump_buf(shell, title, buf, len);

	mbedtls_mpi_write_binary(&key->Q.X, buf, 48);
	dump_buf(shell, "  + Qx: ", buf, 48);
	mbedtls_mpi_write_binary(&key->Q.Y, buf, 48);
	dump_buf(shell, "  + Qy: ", buf, 48);
	mbedtls_mpi_write_binary(&key->Q.Z, buf, 48);
	dump_buf(shell, "  + Qz: ", buf, 48);

}

static int mbedtls_ecdsa_test(const struct shell *shell, int argc, char *argv[])
{
	mbedtls_ecdsa_context ctx_verify;
	int ret = 0;
	int i;
	mbedtls_mpi r, s;
	unsigned char hash[32];
	char z = 1;

	for (i = 0; i < ARRAY_SIZE(secp384r1); i++) {
		mbedtls_ecdsa_init(&ctx_verify);
		mbedtls_mpi_init(&r);
		mbedtls_mpi_init(&s);
		mbedtls_mpi_read_binary(&ctx_verify.Q.X, secp384r1[i].Qx, 48);
		mbedtls_mpi_read_binary(&ctx_verify.Q.Y, secp384r1[i].Qy, 48);
		mbedtls_mpi_read_binary(&ctx_verify.Q.Z, &z, 1);
		mbedtls_mpi_read_binary(&r, secp384r1[i].r, 48);
		mbedtls_mpi_read_binary(&s, secp384r1[i].s, 48);

		shell_fprintf(shell, SHELL_NORMAL, " start load curve\n");
		mbedtls_ecp_group_load(&ctx_verify.grp, MBEDTLS_ECP_DP_SECP384R1);
		shell_fprintf(shell, SHELL_NORMAL, " start verify\n");
		dump_pubkey(shell, "  + Public key: ", &ctx_verify);
		shell_fprintf(shell, SHELL_NORMAL, " signature\n");
		dump_buf(shell, "  + r: ", secp384r1[i].r, 48);
		dump_buf(shell, "  + s: ", secp384r1[i].s, 48);
		memcpy(hash, secp384r1[i].m, secp384r1[i].m_size);

		ret = mbedtls_ecdsa_verify(&ctx_verify.grp, hash, secp384r1[i].m_size,
								   &ctx_verify.Q, &r, &s);
		if (ret != 0) {
			shell_fprintf(shell, SHELL_NORMAL, " failed! mbedtls_ecdsa_verify returned %d\n", ret);
		} else {
			shell_fprintf(shell, SHELL_NORMAL, " Pass\n");
		}
		mbedtls_ecdsa_free(&ctx_verify);
		mbedtls_mpi_free(&r);
		mbedtls_mpi_free(&s);
	}

	return ret;
}

static int hw_ecdsa_test(const struct shell *shell, int argc, char *argv[])
{
	const struct device *dev = device_get_binding(ECDSA_DRV_NAME);
	struct ecdsa_ctx ini;
	struct ecdsa_pkt pkt;
	struct ecdsa_key ek;
	int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(secp384r1); i++) {
		ek.curve_id = ECC_CURVE_NIST_P384;
		ek.qx = (char *)secp384r1[i].Qx;
		ek.qy = (char *)secp384r1[i].Qy;
		pkt.m = (char *)secp384r1[i].m;
		pkt.r = (char *)secp384r1[i].r;
		pkt.s = (char *)secp384r1[i].s;
		pkt.m_len = 48;
		pkt.r_len = 48;
		pkt.s_len = 48;

		shell_fprintf(shell, SHELL_NORMAL, " start verify\n");
		ret = ecdsa_begin_session(dev, &ini, &ek);
		if (ret)
			shell_print(shell, "ecdsa_begin_session fail: %d", ret);
		ret = ecdsa_verify(&ini, &pkt);
		if (ret)
			shell_fprintf(shell, SHELL_NORMAL, " Failed\n");
		else
			shell_fprintf(shell, SHELL_NORMAL, " Pass\n");
		ecdsa_free_session(dev, &ini);
	}
	return ret;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ecdsa_cmds,
							   SHELL_CMD_ARG(test, NULL, "", mbedtls_ecdsa_test, 1, 0),
							   SHELL_CMD_ARG(hw_test, NULL, "", hw_ecdsa_test, 1, 0),
							   SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(ecdsa, &ecdsa_cmds, "ECDSA shell commands", NULL);
