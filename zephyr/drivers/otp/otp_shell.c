/*
 * Copyright (c) 2021 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/util.h>
#include "otp_info_10xx.h"
#include "sha256.h"

#define OTP_VER					"1.2.1"

#define OTP_PASSWD				0x349fe38a
#define RETRY					20
#define OTP_REGION_STRAP		BIT(0)
#define OTP_REGION_CONF			BIT(1)
#define OTP_REGION_DATA			BIT(2)

#define OTP_USAGE				-1
#define OTP_FAILURE				-2
#define OTP_SUCCESS				0

#define OTP_PROG_SKIP			1

#define OTP_KEY_TYPE_RSA_PUB	1
#define OTP_KEY_TYPE_RSA_PRIV	2
#define OTP_KEY_TYPE_AES		3
#define OTP_KEY_TYPE_VAULT		4
#define OTP_KEY_TYPE_HMAC		5
#define OTP_KEY_ECDSA384		6
#define OTP_KEY_ECDSA384P		7

#define OTP_BASE				0x7e6f2000
#define OTP_PROTECT_KEY			OTP_BASE
#define OTP_COMMAND				OTP_BASE + 0x4
#define OTP_TIMING				OTP_BASE + 0x8
#define OTP_ADDR				OTP_BASE + 0x10
#define OTP_STATUS				OTP_BASE + 0x14
#define OTP_COMPARE_1			OTP_BASE + 0x20
#define OTP_COMPARE_2			OTP_BASE + 0x24
#define OTP_COMPARE_3			OTP_BASE + 0x28
#define OTP_COMPARE_4			OTP_BASE + 0x2c
#define SW_REV_ID0				OTP_BASE + 0x68
#define SW_REV_ID1				OTP_BASE + 0x6c
#define SEC_KEY_NUM				OTP_BASE + 0x78
#define ASPEED_REVISION_ID0		0x7e6e2004
#define ASPEED_REVISION_ID1		0x7e6e2014

#define OTP_MAGIC				"SOCOTP"
#define CHECKSUM_LEN			32
#define OTP_INC_DATA			BIT(31)
#define OTP_INC_CONFIG			BIT(30)
#define OTP_INC_STRAP			BIT(29)
#define OTP_ECC_EN				BIT(28)
#define OTP_INC_SCU_PRO			BIT(25)
#define OTP_REGION_SIZE(info)	(((info) >> 16) & 0xffff)
#define OTP_REGION_OFFSET(info)	((info) & 0xffff)
#define OTP_IMAGE_SIZE(info)	((info) & 0xffff)

#define ID0_AST1030A0	0x80000000
#define ID1_AST1030A0	0x80000000
#define ID0_AST1030A1	0x80010000
#define ID1_AST1030A1	0x80010000
#define ID0_AST1060A1	0xA0010000
#define ID1_AST1060A1	0xA0010000

#define OTP_AST1030A0	1
#define OTP_AST1030A1	2
#define OTP_AST1060A1	3

#define SOC_AST1030A0	4
#define SOC_AST1030A1	5
#define SOC_AST1060A1	6

#define OTPTOOL_VERSION(a, b, c) (((a) << 24) + ((b) << 12) + (c))

#define shell_printf(_sh, _ft, ...) \
	shell_fprintf(_sh, SHELL_NORMAL, _ft, ##__VA_ARGS__)

struct otp_header {
	uint8_t		otp_magic[8];
	uint32_t	soc_ver;
	uint32_t	otptool_ver;
	uint32_t	image_info;
	uint32_t	data_info;
	uint32_t	config_info;
	uint32_t	strap_info;
	uint32_t	scu_protect_info;
	uint32_t	checksum_offset;
} __packed;

struct otpstrap_status {
	int value;
	int option_array[7];
	int remain_times;
	int writeable_option;
	int protected;
};

struct otpkey_type {
	int value;
	int key_type;
	int need_id;
	char information[110];
};

struct otp_pro_sts {
	char mem_lock;
	char pro_key_ret;
	char pro_strap;
	char pro_conf;
	char pro_data;
	char pro_sec;
	uint32_t sec_size;
};

struct otp_info_cb {
	int version;
	char ver_name[10];
	const struct otpstrap_info *strap_info;
	int strap_info_len;
	const struct otpconf_info *conf_info;
	int conf_info_len;
	const struct otpkey_type *key_info;
	int key_info_len;
	const struct scu_info *scu_info;
	int scu_info_len;
	struct otp_pro_sts pro_sts;
};

struct otp_image_layout {
	int data_length;
	int conf_length;
	int strap_length;
	int scu_pro_length;
	uint8_t *data;
	uint8_t *data_ignore;
	uint8_t *conf;
	uint8_t *conf_ignore;
	uint8_t *strap;
	uint8_t *strap_pro;
	uint8_t *strap_ignore;
	uint8_t *scu_pro;
	uint8_t *scu_pro_ignore;
};

static struct otp_info_cb info_cb;
struct otpstrap_status strap_status[64];

static const struct otpkey_type ast10xxa0_key_type[] = {
	{
		1, OTP_KEY_TYPE_VAULT, 0,
		"AES-256 as secret vault key"
	},
	{
		2, OTP_KEY_TYPE_AES,   1,
		"AES-256 as OEM platform key for image encryption/decryption in Mode 2 or AES-256 as OEM DSS keys for Mode GCM"
	},
	{
		8, OTP_KEY_TYPE_RSA_PUB,   1,
		"RSA-public as OEM DSS public keys in Mode 2"
	},
	{
		10, OTP_KEY_TYPE_RSA_PUB,  0,
		"RSA-public as AES key decryption key"
	},
	{
		14, OTP_KEY_TYPE_RSA_PRIV,  0,
		"RSA-private as AES key decryption key"
	},
};

static const struct otpkey_type ast10xxa1_key_type[] = {
	{
		1, OTP_KEY_TYPE_VAULT, 0,
		"AES-256 as secret vault key"
	},
	{
		2, OTP_KEY_TYPE_AES,   1,
		"AES-256 as OEM platform key for image encryption/decryption in Mode 2 or AES-256 as OEM DSS keys for Mode GCM"
	},
	{
		8, OTP_KEY_TYPE_RSA_PUB,   1,
		"RSA-public as OEM DSS public keys in Mode 2"
	},
	{
		9, OTP_KEY_TYPE_RSA_PUB,   1,
		"RSA-public as OEM DSS public keys in Mode 2(big endian)"
	},
	{
		10, OTP_KEY_TYPE_RSA_PUB,  0,
		"RSA-public as AES key decryption key"
	},
	{
		11, OTP_KEY_TYPE_RSA_PUB,  0,
		"RSA-public as AES key decryption key(big endian)"
	},
	{
		12, OTP_KEY_TYPE_RSA_PRIV,  0,
		"RSA-private as AES key decryption key"
	},
	{
		13, OTP_KEY_TYPE_RSA_PRIV,  0,
		"RSA-private as AES key decryption key(big endian)"
	},
	{
		5, OTP_KEY_ECDSA384P,  0,
		"ECDSA384 cure parameter"
	},
	{
		7, OTP_KEY_ECDSA384,  0,
		"ECDSA-public as OEM DSS public keys"
	}
};

static int ast_otp_init(const struct shell *shell);
static void ast_otp_finish(void);

int confirm_yesno(void)
{
	return 1;
}

static void buf_print(const struct shell *shell, uint8_t *buf, int len)
{
	int i;

	shell_printf(shell, "      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			shell_printf(shell, "%04X: ", i);
		shell_printf(shell, "%02X ", buf[i]);
		if ((i + 1) % 16 == 0)
			shell_printf(shell, "\n");
	}
	shell_printf(shell, "\n");
}

static int get_dw_bit(uint32_t *rid, int offset)
{
	int bit_offset;
	int i;

	if (offset < 32) {
		i = 0;
		bit_offset = offset;
	} else {
		i = 1;
		bit_offset = offset - 32;
	}
	if ((rid[i] >> bit_offset) & 0x1)
		return 1;
	else
		return 0;
}

static int get_rid_num(uint32_t *rid)
{
	int i;
	int fz = 0;
	int rid_num = 0;
	int ret = 0;

	for (i = 0; i < 64; i++) {
		if (get_dw_bit(rid, i) == 0) {
			if (!fz)
				fz = 1;

		} else {
			rid_num++;
			if (fz)
				ret = OTP_FAILURE;
		}
	}
	if (ret)
		return ret;

	return rid_num;
}

static uint32_t chip_version(void)
{
	uint32_t revid0, revid1;

	revid0 = sys_read32(ASPEED_REVISION_ID0);
	revid1 = sys_read32(ASPEED_REVISION_ID1);

	if (revid0 == ID0_AST1030A0 && revid1 == ID1_AST1030A0) {
		/* AST1030-A0 */
		return OTP_AST1030A0;
	} else if (revid0 == ID0_AST1030A1 && revid1 == ID1_AST1030A1) {
		/* AST1030-A1 */
		return OTP_AST1030A1;
	} else if (revid0 == ID0_AST1060A1 && revid1 == ID1_AST1060A1) {
		/* AST1060-A1 */
		return OTP_AST1060A1;
	}
	return OTP_FAILURE;
}

static void wait_complete(void)
{
	int reg;

	k_usleep(100);
	do {
		reg = sys_read32(OTP_STATUS);
	} while ((reg & 0x6) != 0x6);
}

static void otp_write(uint32_t otp_addr, uint32_t data)
{
	sys_write32(otp_addr, OTP_ADDR); /* write address */
	sys_write32(data, OTP_COMPARE_1); /* write data */
	sys_write32(0x23b1e362, OTP_COMMAND); /* write command */
	wait_complete();
}

static void otp_soak(int soak)
{
	switch (soak) {
	case 0: /* default */
		otp_write(0x3000, 0x0); /* Write MRA */
		otp_write(0x5000, 0x0); /* Write MRB */
		otp_write(0x1000, 0x0); /* Write MR */
		break;
	case 1: /* normal program */
		otp_write(0x3000, 0x1320); /* Write MRA */
		otp_write(0x5000, 0x1008); /* Write MRB */
		otp_write(0x1000, 0x0024); /* Write MR */
		sys_write32(0x04191388, OTP_TIMING); /* 200us */
		break;
	case 2: /* soak program */
		otp_write(0x3000, 0x1320); /* Write MRA */
		otp_write(0x5000, 0x0007); /* Write MRB */
		otp_write(0x1000, 0x0100); /* Write MR */
		sys_write32(0x04193a98, OTP_TIMING); /* 600us */
		break;
	}
	wait_complete();
}

static void otp_read_data(uint32_t offset, uint32_t *data)
{
	sys_write32(offset, OTP_ADDR); /* Read address */
	sys_write32(0x23b1e361, OTP_COMMAND); /* trigger read */
	wait_complete();
	data[0] = sys_read32(OTP_COMPARE_1);
	data[1] = sys_read32(OTP_COMPARE_2);
}

static void otp_read_conf(uint32_t offset, uint32_t *data)
{
	int config_offset;

	config_offset = 0x800;
	config_offset |= (offset / 8) * 0x200;
	config_offset |= (offset % 8) * 0x2;

	sys_write32(config_offset, OTP_ADDR);  /* Read address */
	sys_write32(0x23b1e361, OTP_COMMAND); /* trigger read */
	wait_complete();
	data[0] = sys_read32(OTP_COMPARE_1);
}

static int verify_bit(uint32_t otp_addr, int bit_offset, int value)
{
	uint32_t ret[2];

	if (otp_addr % 2 == 0)
		sys_write32(otp_addr, OTP_ADDR); /* Read address */
	else
		sys_write32(otp_addr - 1, OTP_ADDR); /* Read address */

	sys_write32(0x23b1e361, OTP_COMMAND); /* trigger read */
	wait_complete();
	ret[0] = sys_read32(OTP_COMPARE_1);
	ret[1] = sys_read32(OTP_COMPARE_2);

	if (otp_addr % 2 == 0) {
		if (((ret[0] >> bit_offset) & 1) == value)
			return OTP_SUCCESS;
		else
			return OTP_FAILURE;
	} else {
		if (((ret[1] >> bit_offset) & 1) == value)
			return OTP_SUCCESS;
		else
			return OTP_FAILURE;
	}
}

static uint32_t verify_dw(uint32_t otp_addr,
						  uint32_t *value, uint32_t *ignore, uint32_t *compare, int size)
{
	uint32_t ret[2];

	otp_addr &= ~(1 << 15);

	if (otp_addr % 2 == 0)
		sys_write32(otp_addr, OTP_ADDR); /* Read address */
	else
		sys_write32(otp_addr - 1, OTP_ADDR); /* Read address */
	sys_write32(0x23b1e361, OTP_COMMAND); /* trigger read */
	wait_complete();
	ret[0] = sys_read32(OTP_COMPARE_1);
	ret[1] = sys_read32(OTP_COMPARE_2);
	if (size == 1) {
		if (otp_addr % 2 == 0) {
			if ((value[0] & ~ignore[0]) == (ret[0] & ~ignore[0])) {
				compare[0] = 0;
				return OTP_SUCCESS;
			}
			compare[0] = value[0] ^ ret[0];
			return OTP_FAILURE;

		} else {
			if ((value[0] & ~ignore[0]) == (ret[1] & ~ignore[0])) {
				compare[0] = ~0;
				return OTP_SUCCESS;
			}
			compare[0] = ~(value[0] ^ ret[1]);
			return OTP_FAILURE;
		}
	} else if (size == 2) {
		/* otp_addr should be even */
		if ((value[0] & ~ignore[0]) == (ret[0] & ~ignore[0]) &&
			(value[1] & ~ignore[1]) == (ret[1] & ~ignore[1])) {
			compare[0] = 0;
			compare[1] = ~0;
			return OTP_SUCCESS;
		}
		compare[0] = value[0] ^ ret[0];
		compare[1] = ~(value[1] ^ ret[1]);
		return OTP_FAILURE;
	} else {
		return OTP_FAILURE;
	}
}

static void otp_prog(uint32_t otp_addr, uint32_t prog_bit)
{
	otp_write(0x0, prog_bit);
	sys_write32(otp_addr, OTP_ADDR); /* write address */
	sys_write32(prog_bit, OTP_COMPARE_1); /* write data */
	sys_write32(0x23b1e364, OTP_COMMAND); /* write command */
	wait_complete();
}

static void _otp_prog_bit(uint32_t value, uint32_t prog_address, uint32_t bit_offset)
{
	int prog_bit;

	if (prog_address % 2 == 0) {
		if (value)
			prog_bit = ~(0x1 << bit_offset);
		else
			return;
	} else {
		if (!value)
			prog_bit = 0x1 << bit_offset;
		else
			return;
	}
	otp_prog(prog_address, prog_bit);
}

static int otp_prog_dc_b(uint32_t value, uint32_t prog_address, uint32_t bit_offset)
{
	int pass;
	int i;

	otp_soak(1);
	_otp_prog_bit(value, prog_address, bit_offset);
	pass = 0;

	for (i = 0; i < RETRY; i++) {
		if (verify_bit(prog_address, bit_offset, value) != 0) {
			otp_soak(2);
			_otp_prog_bit(value, prog_address, bit_offset);
			if (verify_bit(prog_address, bit_offset, value) != 0) {
				otp_soak(1);
			} else {
				pass = 1;
				break;
			}
		} else {
			pass = 1;
			break;
		}
	}
	if (pass)
		return OTP_SUCCESS;

	return OTP_FAILURE;
}

static void otp_prog_dw(uint32_t value, uint32_t ignore, uint32_t prog_address)
{
	int j, bit_value, prog_bit;

	for (j = 0; j < 32; j++) {
		if ((ignore >> j) & 0x1)
			continue;
		bit_value = (value >> j) & 0x1;
		if (prog_address % 2 == 0) {
			if (bit_value)
				prog_bit = ~(0x1 << j);
			else
				continue;
		} else {
			if (bit_value)
				continue;
			else
				prog_bit = 0x1 << j;
		}
		otp_prog(prog_address, prog_bit);
	}
}

static int otp_prog_verify_2dw(uint32_t *data,
							   uint32_t *buf, uint32_t *ignore_mask, uint32_t prog_address)
{
	int pass;
	int i;
	uint32_t data0_masked;
	uint32_t data1_masked;
	uint32_t buf0_masked;
	uint32_t buf1_masked;
	uint32_t compare[2];

	data0_masked = data[0]  & ~ignore_mask[0];
	buf0_masked  = buf[0] & ~ignore_mask[0];
	data1_masked = data[1]  & ~ignore_mask[1];
	buf1_masked  = buf[1] & ~ignore_mask[1];
	if (data0_masked == buf0_masked && data1_masked == buf1_masked)
		return OTP_SUCCESS;

	for (i = 0; i < 32; i++) {
		if (((data0_masked >> i) & 0x1) == 1 && ((buf0_masked >> i) & 0x1) == 0)
			return OTP_FAILURE;
		if (((data1_masked >> i) & 0x1) == 0 && ((buf1_masked >> i) & 0x1) == 1)
			return OTP_FAILURE;
	}

	otp_soak(1);
	if (data0_masked != buf0_masked)
		otp_prog_dw(buf[0], ignore_mask[0], prog_address);
	if (data1_masked != buf1_masked)
		otp_prog_dw(buf[1], ignore_mask[1], prog_address + 1);

	pass = 0;
	for (i = 0; i < RETRY; i++) {
		if (verify_dw(prog_address, buf, ignore_mask, compare, 2) != 0) {
			otp_soak(2);
			if (compare[0] != 0)
				otp_prog_dw(compare[0], ignore_mask[0], prog_address);
			if (compare[1] != ~0)
				otp_prog_dw(compare[1], ignore_mask[1], prog_address + 1);
			if (verify_dw(prog_address, buf, ignore_mask, compare, 2) != 0) {
				otp_soak(1);
			} else {
				pass = 1;
				break;
			}
		} else {
			pass = 1;
			break;
		}
	}

	if (!pass) {
		otp_soak(0);
		return OTP_FAILURE;
	}
	return OTP_SUCCESS;
}

static void otp_strap_status(struct otpstrap_status *os)
{
	uint32_t OTPSTRAP_RAW[2];
	int strap_end;
	int i, j;

	for (j = 0; j < 64; j++) {
		os[j].value = 0;
		os[j].remain_times = 6;
		os[j].writeable_option = -1;
		os[j].protected = 0;
	}
	strap_end = 28;

	otp_soak(0);
	for (i = 16; i < strap_end; i += 2) {
		int option = (i - 16) / 2;

		otp_read_conf(i, &OTPSTRAP_RAW[0]);
		otp_read_conf(i + 1, &OTPSTRAP_RAW[1]);
		for (j = 0; j < 32; j++) {
			char bit_value = ((OTPSTRAP_RAW[0] >> j) & 0x1);

			if (bit_value == 0 && os[j].writeable_option == -1)
				os[j].writeable_option = option;
			if (bit_value == 1)
				os[j].remain_times--;
			os[j].value ^= bit_value;
			os[j].option_array[option] = bit_value;
		}
		for (j = 32; j < 64; j++) {
			char bit_value = ((OTPSTRAP_RAW[1] >> (j - 32)) & 0x1);

			if (bit_value == 0 && os[j].writeable_option == -1)
				os[j].writeable_option = option;
			if (bit_value == 1)
				os[j].remain_times--;
			os[j].value ^= bit_value;
			os[j].option_array[option] = bit_value;
		}
	}

	otp_read_conf(30, &OTPSTRAP_RAW[0]);
	otp_read_conf(31, &OTPSTRAP_RAW[1]);
	for (j = 0; j < 32; j++) {
		if (((OTPSTRAP_RAW[0] >> j) & 0x1) == 1)
			os[j].protected = 1;
	}
	for (j = 32; j < 64; j++) {
		if (((OTPSTRAP_RAW[1] >> (j - 32)) & 0x1) == 1)
			os[j].protected = 1;
	}
}

static int otp_strap_bit_confirm(const struct shell *shell,
								 struct otpstrap_status *os, int offset,
								 int ibit, int bit, int pbit)
{
	int prog_flag = 0;

	/* ignore this bit */
	if (ibit == 1)
		return OTP_SUCCESS;
	shell_printf(shell, "OTPSTRAP[0x%X]:\n", offset);

	if (bit == os->value) {
		if (!pbit) {
			shell_printf(shell, "    The value is same as before, skip it.\n");
			return OTP_PROG_SKIP;
		}
		shell_printf(shell, "    The value is same as before.\n");
	} else {
		prog_flag = 1;
	}
	if (os->protected == 1 && prog_flag) {
		shell_printf(shell, "    This bit is protected and is not writable\n");
		return OTP_FAILURE;
	}
	if (os->remain_times == 0 && prog_flag) {
		shell_printf(shell, "    This bit has no remaining chance to write.\n");
		return OTP_FAILURE;
	}
	if (pbit == 1)
		shell_printf(shell, "    This bit will be protected and become non-writable.\n");
	if (prog_flag)
		shell_printf(shell,
					 "    Write 1 to OTPSTRAP[0x%X] OPTION[0x%X], that value becomes from 0x%X to 0x%X.\n",
					 offset, os->writeable_option + 1, os->value, os->value ^ 1);

	return OTP_SUCCESS;
}

static int otp_prog_strap_b(const struct shell *shell, int bit_offset, int value)
{
	uint32_t prog_address;
	int offset;
	int ret;

	otp_strap_status(strap_status);

	ret = otp_strap_bit_confirm(shell, &strap_status[bit_offset], bit_offset, 0, value, 0);

	if (ret != OTP_SUCCESS)
		return ret;

	prog_address = 0x800;
	if (bit_offset < 32) {
		offset = bit_offset;
		prog_address |= ((strap_status[bit_offset].writeable_option * 2 + 16) / 8) * 0x200;
		prog_address |= ((strap_status[bit_offset].writeable_option * 2 + 16) % 8) * 0x2;

	} else {
		offset = (bit_offset - 32);
		prog_address |= ((strap_status[bit_offset].writeable_option * 2 + 17) / 8) * 0x200;
		prog_address |= ((strap_status[bit_offset].writeable_option * 2 + 17) % 8) * 0x2;
	}

	return otp_prog_dc_b(1, prog_address, offset);
}

static int otp_print_conf(const struct shell *shell, uint32_t offset, int dw_count)
{
	int i;
	uint32_t ret[1];

	if (offset + dw_count > 32)
		return OTP_USAGE;
	otp_soak(0);
	for (i = offset; i < offset + dw_count; i++) {
		otp_read_conf(i, ret);
		shell_printf(shell, "OTPCFG0x%X: 0x%08X\n", i, ret[0]);
	}
	shell_printf(shell, "\n");
	return OTP_SUCCESS;
}

static int otp_print_data(const struct shell *shell, uint32_t offset, int dw_count)
{
	int i;
	uint32_t ret[2];

	if (offset + dw_count > 2048 || offset % 4 != 0)
		return OTP_USAGE;
	otp_soak(0);
	for (i = offset; i < offset + dw_count; i += 2) {
		otp_read_data(i, ret);
		if (i % 4 == 0)
			shell_printf(shell, "%03X: %08X %08X ", i * 4, ret[0], ret[1]);
		else
			shell_printf(shell, "%08X %08X\n", ret[0], ret[1]);
	}
	shell_printf(shell, "\n");
	return OTP_SUCCESS;
}

static int otp_print_strap(const struct shell *shell, int start, int count)
{
	int i, j;
	int remains = 6;

	if (start < 0 || start > 64)
		return OTP_USAGE;

	if ((start + count) < 0 || (start + count) > 64)
		return OTP_USAGE;

	otp_strap_status(strap_status);

	shell_printf(shell, "BIT(hex)  Value  Option           Status\n");
	shell_printf(shell,
				 "______________________________________________________________________________\n");

	for (i = start; i < start + count; i++) {
		shell_printf(shell, "0x%-8X", i);
		shell_printf(shell, "%-7d", strap_status[i].value);
		for (j = 0; j < remains; j++)
			shell_printf(shell, "%d ", strap_status[i].option_array[j]);
		shell_printf(shell, "   ");
		if (strap_status[i].protected == 1) {
			shell_printf(shell, "protected and not writable");
		} else {
			shell_printf(shell, "not protected ");
			if (strap_status[i].remain_times == 0)
				shell_printf(shell, "and no remaining times to write.");
			else
				shell_printf(shell, "and still can write %d times", strap_status[i].remain_times);
		}
		shell_printf(shell, "\n");
	}

	return OTP_SUCCESS;
}

static void otp_print_revid(const struct shell *shell, uint32_t *rid)
{
	int bit_offset;
	int i, j;

	shell_printf(shell, "     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	shell_printf(shell, "___________________________________________________\n");
	for (i = 0; i < 64; i++) {
		if (i < 32) {
			j = 0;
			bit_offset = i;
		} else {
			j = 1;
			bit_offset = i - 32;
		}
		if (i % 16 == 0)
			shell_printf(shell, "%2x | ", i);
		shell_printf(shell, "%d  ", (rid[j] >> bit_offset) & 0x1);
		if ((i + 1) % 16 == 0)
			shell_printf(shell, "\n");
	}
}

static int otp_print_scu_image(const struct shell *shell, struct otp_image_layout *image_layout)
{
	const struct scu_info *scu_info = info_cb.scu_info;
	uint32_t *OTPSCU = (uint32_t *)image_layout->scu_pro;
	uint32_t *OTPSCU_IGNORE = (uint32_t *)image_layout->scu_pro_ignore;
	int i;
	uint32_t scu_offset;
	uint32_t dw_offset;
	uint32_t bit_offset;
	uint32_t mask;
	uint32_t otp_value;
	uint32_t otp_ignore;

	shell_printf(shell, "SCU     BIT          reg_protect     Description\n");
	shell_printf(shell, "____________________________________________________________________\n");
	for (i = 0; i < info_cb.scu_info_len; i++) {
		mask = BIT(scu_info[i].length) - 1;

		if (scu_info[i].bit_offset > 31) {
			scu_offset = 0x510;
			dw_offset = 1;
			bit_offset = scu_info[i].bit_offset - 32;
		} else {
			scu_offset = 0x500;
			dw_offset = 0;
			bit_offset = scu_info[i].bit_offset;
		}

		otp_value = (OTPSCU[dw_offset] >> bit_offset) & mask;
		otp_ignore = (OTPSCU_IGNORE[dw_offset] >> bit_offset) & mask;

		if (otp_ignore == mask)
			continue;
		else if (otp_ignore != 0)
			return OTP_FAILURE;

		if (otp_value != 0 && otp_value != mask)
			return OTP_FAILURE;

		shell_printf(shell, "0x%-6X", scu_offset);
		if (scu_info[i].length == 1)
			shell_printf(shell, "0x%-11X", bit_offset);
		else
			shell_printf(shell, "0x%-2X:0x%-6x", bit_offset, bit_offset + scu_info[i].length - 1);
		shell_printf(shell, "0x%-14X", otp_value);
		shell_printf(shell, "%s\n", scu_info[i].information);
	}
	return OTP_SUCCESS;
}

static void otp_print_scu_info(const struct shell *shell)
{
	const struct scu_info *scu_info = info_cb.scu_info;
	uint32_t OTPCFG[2];
	uint32_t scu_offset;
	uint32_t bit_offset;
	uint32_t reg_p;
	uint32_t length;
	int i, j;

	otp_soak(0);
	otp_read_conf(28, &OTPCFG[0]);
	otp_read_conf(29, &OTPCFG[1]);
	shell_printf(shell, "SCU     BIT   reg_protect     Description\n");
	shell_printf(shell, "____________________________________________________________________\n");
	for (i = 0; i < info_cb.scu_info_len; i++) {
		length = scu_info[i].length;
		for (j = 0; j < length; j++) {
			if (scu_info[i].bit_offset + j < 32) {
				scu_offset = 0x500;
				bit_offset = scu_info[i].bit_offset + j;
				reg_p = (OTPCFG[0] >> bit_offset) & 0x1;
			} else {
				scu_offset = 0x510;
				bit_offset = scu_info[i].bit_offset + j - 32;
				reg_p = (OTPCFG[1] >> bit_offset) & 0x1;
			}
			shell_printf(shell, "0x%-6X", scu_offset);
			shell_printf(shell, "0x%-4X", bit_offset);
			shell_printf(shell, "0x%-13X", reg_p);
			if (length == 1) {
				shell_printf(shell, " %s\n", scu_info[i].information);
				continue;
			}

			if (j == 0)
				shell_printf(shell, "/%s\n", scu_info[i].information);
			else if (j == length - 1)
				shell_printf(shell, "\\ \"\n");
			else
				shell_printf(shell, "| \"\n");
		}
	}
}

static int otp_print_conf_image(const struct shell *shell, struct otp_image_layout *image_layout)
{
	const struct otpconf_info *conf_info = info_cb.conf_info;
	uint32_t *OTPCFG = (uint32_t *)image_layout->conf;
	uint32_t *OTPCFG_IGNORE = (uint32_t *)image_layout->conf_ignore;
	uint32_t mask;
	uint32_t dw_offset;
	uint32_t bit_offset;
	uint32_t otp_value;
	uint32_t otp_ignore;
	int fail = 0;
	int mask_err;
	int rid_num = 0;
	char valid_bit[20];
	int fz;
	int i;
	int j;

	shell_printf(shell, "DW    BIT        Value       Description\n");
	shell_printf(shell,
				 "__________________________________________________________________________\n");
	for (i = 0; i < info_cb.conf_info_len; i++) {
		mask_err = 0;
		dw_offset = conf_info[i].dw_offset;
		bit_offset = conf_info[i].bit_offset;
		mask = BIT(conf_info[i].length) - 1;
		otp_value = (OTPCFG[dw_offset] >> bit_offset) & mask;
		otp_ignore = (OTPCFG_IGNORE[dw_offset] >> bit_offset) & mask;

		if (conf_info[i].value == OTP_REG_VALID_BIT) {
			if (((otp_value + otp_ignore) & mask) != mask) {
				fail = 1;
				mask_err = 1;
			}
		} else {
			if (otp_ignore == mask) {
				continue;
			} else if (otp_ignore != 0) {
				fail = 1;
				mask_err = 1;
			}
		}

		if (otp_value != conf_info[i].value &&
			conf_info[i].value != OTP_REG_RESERVED &&
			conf_info[i].value != OTP_REG_VALUE &&
			conf_info[i].value != OTP_REG_VALID_BIT)
			continue;
		shell_printf(shell, "0x%-4X", dw_offset);

		if (conf_info[i].length == 1) {
			shell_printf(shell, "0x%-9X", conf_info[i].bit_offset);
		} else {
			shell_printf(shell, "0x%-2X:0x%-4X",
						 conf_info[i].bit_offset + conf_info[i].length - 1,
						 conf_info[i].bit_offset);
		}
		shell_printf(shell, "0x%-10x", otp_value);

		if (mask_err) {
			shell_printf(shell, "Ignore, mask error\n");
			continue;
		}
		if (conf_info[i].value == OTP_REG_RESERVED) {
			shell_printf(shell, "Reserved\n");
		} else if (conf_info[i].value == OTP_REG_VALUE) {
			shell_printf(shell, conf_info[i].information, otp_value);
			shell_printf(shell, "\n");
		} else if (conf_info[i].value == OTP_REG_VALID_BIT) {
			if (otp_value != 0) {
				for (j = 0; j < 7; j++) {
					if (otp_value & (1 << j))
						valid_bit[j * 2] = '1';
					else
						valid_bit[j * 2] = '0';
					valid_bit[j * 2 + 1] = ' ';
				}
				valid_bit[15] = 0;
			} else {
				strcpy(valid_bit, "0 0 0 0 0 0 0 0\0");
			}
			shell_printf(shell, conf_info[i].information, valid_bit);
			shell_printf(shell, "\n");
		} else {
			shell_printf(shell, "%s\n", conf_info[i].information);
		}
	}

	if (OTPCFG[0xa] != 0 || OTPCFG[0xb] != 0) {
		if (OTPCFG_IGNORE[0xa] != 0 && OTPCFG_IGNORE[0xb] != 0) {
			shell_printf(shell, "OTP revision ID is invalid.\n");
			fail = 1;
		} else {
			fz = 0;
			for (i = 0; i < 64; i++) {
				if (get_dw_bit(&OTPCFG[0xa], i) == 0) {
					if (!fz)
						fz = 1;
				} else {
					rid_num++;
					if (fz) {
						shell_printf(shell, "OTP revision ID is invalid.\n");
						fail = 1;
						break;
					}
				}
			}
		}
		if (fail)
			shell_printf(shell, "OTP revision ID\n");
		else
			shell_printf(shell, "OTP revision ID: 0x%x\n", rid_num);
		otp_print_revid(shell, &OTPCFG[0xa]);
	}

	if (fail)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_print_conf_info(const struct shell *shell, int input_offset)
{
	const struct otpconf_info *conf_info = info_cb.conf_info;
	uint32_t OTPCFG[16];
	uint32_t mask;
	uint32_t dw_offset;
	uint32_t bit_offset;
	uint32_t otp_value;
	char valid_bit[20];
	int i;
	int j;

	otp_soak(0);
	for (i = 0; i < 16; i++)
		otp_read_conf(i, &OTPCFG[i]);

	shell_printf(shell, "DW    BIT        Value       Description\n");
	shell_printf(shell,
				 "__________________________________________________________________________\n");
	for (i = 0; i < info_cb.conf_info_len; i++) {
		if (input_offset != -1 && input_offset != conf_info[i].dw_offset)
			continue;
		dw_offset = conf_info[i].dw_offset;
		bit_offset = conf_info[i].bit_offset;
		mask = BIT(conf_info[i].length) - 1;
		otp_value = (OTPCFG[dw_offset] >> bit_offset) & mask;

		if (otp_value != conf_info[i].value &&
			conf_info[i].value != OTP_REG_RESERVED &&
			conf_info[i].value != OTP_REG_VALUE &&
			conf_info[i].value != OTP_REG_VALID_BIT)
			continue;
		shell_printf(shell, "0x%-4X", dw_offset);

		if (conf_info[i].length == 1) {
			shell_printf(shell, "0x%-9X", conf_info[i].bit_offset);
		} else {
			shell_printf(shell, "0x%-2X:0x%-4X",
						 conf_info[i].bit_offset + conf_info[i].length - 1,
						 conf_info[i].bit_offset);
		}
		shell_printf(shell, "0x%-10x", otp_value);

		if (conf_info[i].value == OTP_REG_RESERVED) {
			shell_printf(shell, "Reserved\n");
		} else if (conf_info[i].value == OTP_REG_VALUE) {
			shell_printf(shell, conf_info[i].information, otp_value);
			shell_printf(shell, "\n");
		} else if (conf_info[i].value == OTP_REG_VALID_BIT) {
			if (otp_value != 0) {
				for (j = 0; j < 7; j++) {
					if (otp_value & (1 << j))
						valid_bit[j * 2] = '1';
					else
						valid_bit[j * 2] = '0';
					valid_bit[j * 2 + 1] = ' ';
				}
				valid_bit[15] = 0;
			} else {
				strcpy(valid_bit, "0 0 0 0 0 0 0 0\0");
			}
			shell_printf(shell, conf_info[i].information, valid_bit);
			shell_printf(shell, "\n");
		} else {
			shell_printf(shell, "%s\n", conf_info[i].information);
		}
	}
	return OTP_SUCCESS;
}

static int otp_print_strap_image(const struct shell *shell, struct otp_image_layout *image_layout)
{
	const struct otpstrap_info *strap_info = info_cb.strap_info;
	uint32_t *OTPSTRAP;
	uint32_t *OTPSTRAP_PRO;
	uint32_t *OTPSTRAP_IGNORE;
	int i;
	int fail = 0;
	uint32_t bit_offset;
	uint32_t dw_offset;
	uint32_t mask;
	uint32_t otp_value;
	uint32_t otp_protect;
	uint32_t otp_ignore;

	OTPSTRAP = (uint32_t *)image_layout->strap;
	OTPSTRAP_PRO = (uint32_t *)image_layout->strap_pro;
	OTPSTRAP_IGNORE = (uint32_t *)image_layout->strap_ignore;

	shell_printf(shell, "BIT(hex)   Value       Protect     Description\n");
	shell_printf(shell,
				 "__________________________________________________________________________________________\n");

	for (i = 0; i < info_cb.strap_info_len; i++) {
		fail = 0;
		if (strap_info[i].bit_offset > 31) {
			dw_offset = 1;
			bit_offset = strap_info[i].bit_offset - 32;
		} else {
			dw_offset = 0;
			bit_offset = strap_info[i].bit_offset;
		}

		mask = BIT(strap_info[i].length) - 1;
		otp_value = (OTPSTRAP[dw_offset] >> bit_offset) & mask;
		otp_protect = (OTPSTRAP_PRO[dw_offset] >> bit_offset) & mask;
		otp_ignore = (OTPSTRAP_IGNORE[dw_offset] >> bit_offset) & mask;

		if (otp_ignore == mask)
			continue;
		else if (otp_ignore != 0)
			fail = 1;

		if (otp_value != strap_info[i].value &&
			strap_info[i].value != OTP_REG_RESERVED)
			continue;

		if (strap_info[i].length == 1) {
			shell_printf(shell, "0x%-9X", strap_info[i].bit_offset);
		} else {
			shell_printf(shell, "0x%-2X:0x%-4X",
						 strap_info[i].bit_offset + strap_info[i].length - 1,
						 strap_info[i].bit_offset);
		}
		shell_printf(shell, "0x%-10x", otp_value);
		shell_printf(shell, "0x%-10x", otp_protect);

		if (fail) {
			shell_printf(shell, "Ignore mask error\n");
		} else {
			if (strap_info[i].value != OTP_REG_RESERVED)
				shell_printf(shell, "%s\n", strap_info[i].information);
			else
				shell_printf(shell, "Reserved\n");
		}
	}

	if (fail)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_print_strap_info(const struct shell *shell, int view)
{
	const struct otpstrap_info *strap_info = info_cb.strap_info;
	int i, j;
	int fail = 0;
	uint32_t bit_offset;
	uint32_t length;
	uint32_t otp_value;
	uint32_t otp_protect;

	otp_strap_status(strap_status);

	if (view) {
		shell_printf(shell, "BIT(hex) Value  Remains  Protect   Description\n");
		shell_printf(shell,
					 "___________________________________________________________________________________________________\n");
	} else {
		shell_printf(shell, "BIT(hex)   Value       Description\n");
		shell_printf(shell,
					 "________________________________________________________________________________\n");
	}
	for (i = 0; i < info_cb.strap_info_len; i++) {
		otp_value = 0;
		bit_offset = strap_info[i].bit_offset;
		length = strap_info[i].length;
		for (j = 0; j < length; j++) {
			otp_value |= strap_status[bit_offset + j].value << j;
			otp_protect |= strap_status[bit_offset + j].protected << j;
		}
		if (otp_value != strap_info[i].value &&
			strap_info[i].value != OTP_REG_RESERVED)
			continue;
		if (view) {
			for (j = 0; j < length; j++) {
				shell_printf(shell, "0x%-7X", strap_info[i].bit_offset + j);
				shell_printf(shell, "0x%-5X", strap_status[bit_offset + j].value);
				shell_printf(shell, "%-9d", strap_status[bit_offset + j].remain_times);
				shell_printf(shell, "0x%-7X", strap_status[bit_offset + j].protected);
				if (strap_info[i].value == OTP_REG_RESERVED) {
					shell_printf(shell, " Reserved\n");
					continue;
				}
				if (length == 1) {
					shell_printf(shell, " %s\n", strap_info[i].information);
					continue;
				}

				if (j == 0)
					shell_printf(shell, "/%s\n", strap_info[i].information);
				else if (j == length - 1)
					shell_printf(shell, "\\ \"\n");
				else
					shell_printf(shell, "| \"\n");
			}
		} else {
			if (length == 1) {
				shell_printf(shell, "0x%-9X", strap_info[i].bit_offset);
			} else {
				shell_printf(shell, "0x%-2X:0x%-4X",
							 bit_offset + length - 1, bit_offset);
			}

			shell_printf(shell, "0x%-10X", otp_value);

			if (strap_info[i].value != OTP_REG_RESERVED)
				shell_printf(shell, "%s\n", strap_info[i].information);
			else
				shell_printf(shell, "Reserved\n");
		}
	}

	if (fail)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static void _otp_print_key(const struct shell *shell, uint32_t *data)
{
	int i, j;
	int key_id, key_offset, last, key_type, key_length, exp_length;
	struct otpkey_type key_info;
	const struct otpkey_type *key_info_array = info_cb.key_info;
	int len = 0;
	uint8_t *byte_buf;
	int empty;

	byte_buf = (uint8_t *)data;

	empty = 1;
	for (i = 0; i < 16; i++) {
		if (i % 2) {
			if (data[i] != 0xffffffff)
				empty = 0;
		} else {
			if (data[i] != 0)
				empty = 0;
		}
	}
	if (empty) {
		shell_printf(shell, "OTP data header is empty\n");
		return;
	}

	for (i = 0; i < 16; i++) {
		key_id = data[i] & 0x7;
		key_offset = data[i] & 0x1ff8;
		last = (data[i] >> 13) & 1;
		key_type = (data[i] >> 14) & 0xf;
		key_length = (data[i] >> 18) & 0x3;
		exp_length = (data[i] >> 20) & 0xfff;

		key_info.value = -1;
		for (j = 0; j < info_cb.key_info_len; j++) {
			if (key_type == key_info_array[j].value) {
				key_info = key_info_array[j];
				break;
			}
		}
		if (key_info.value == -1)
			break;

		shell_printf(shell, "\nKey[%d]:\n", i);
		shell_printf(shell, "Key Type: ");
		shell_printf(shell, "%s\n", key_info.information);

		if (key_info.key_type == OTP_KEY_TYPE_HMAC) {
			shell_printf(shell, "HMAC SHA Type: ");
			switch (key_length) {
			case 0:
				shell_printf(shell, "HMAC(SHA224)\n");
				break;
			case 1:
				shell_printf(shell, "HMAC(SHA256)\n");
				break;
			case 2:
				shell_printf(shell, "HMAC(SHA384)\n");
				break;
			case 3:
				shell_printf(shell, "HMAC(SHA512)\n");
				break;
			}
		} else if (key_info.key_type == OTP_KEY_TYPE_RSA_PRIV ||
				   key_info.key_type == OTP_KEY_TYPE_RSA_PUB) {
			shell_printf(shell, "RSA SHA Type: ");
			switch (key_length) {
			case 0:
				shell_printf(shell, "RSA1024\n");
				len = 0x100;
				break;
			case 1:
				shell_printf(shell, "RSA2048\n");
				len = 0x200;
				break;
			case 2:
				shell_printf(shell, "RSA3072\n");
				len = 0x300;
				break;
			case 3:
				shell_printf(shell, "RSA4096\n");
				len = 0x400;
				break;
			}
			shell_printf(shell, "RSA exponent bit length: %d\n", exp_length);
		}
		if (key_info.need_id)
			shell_printf(shell, "Key Number ID: %d\n", key_id);
		shell_printf(shell, "Key Value:\n");
		if (key_info.key_type == OTP_KEY_TYPE_HMAC) {
			buf_print(shell, &byte_buf[key_offset], 0x40);
		} else if (key_info.key_type == OTP_KEY_TYPE_AES) {
			shell_printf(shell, "AES Key:\n");
			buf_print(shell, &byte_buf[key_offset], 0x20);
		} else if (key_info.key_type == OTP_KEY_TYPE_VAULT) {
			shell_printf(shell, "AES Key 1:\n");
			buf_print(shell, &byte_buf[key_offset], 0x20);
			shell_printf(shell, "AES Key 2:\n");
			buf_print(shell, &byte_buf[key_offset + 0x20], 0x20);
		} else if (key_info.key_type == OTP_KEY_TYPE_RSA_PRIV) {
			shell_printf(shell, "RSA mod:\n");
			buf_print(shell, &byte_buf[key_offset], len / 2);
			shell_printf(shell, "RSA exp:\n");
			buf_print(shell, &byte_buf[key_offset + (len / 2)], len / 2);
		} else if (key_info.key_type == OTP_KEY_TYPE_RSA_PUB) {
			shell_printf(shell, "RSA mod:\n");
			buf_print(shell, &byte_buf[key_offset], len / 2);
			shell_printf(shell, "RSA exp:\n");
			buf_print(shell, (uint8_t *)"\x01\x00\x01", 3);
		}
		if (last)
			break;
	}
}

static int otp_print_data_image(const struct shell *shell, struct otp_image_layout *image_layout)
{
	uint32_t *buf;

	buf = (uint32_t *)image_layout->data;
	_otp_print_key(shell, buf);

	return OTP_SUCCESS;
}

static void otp_print_key_info(const struct shell *shell)
{
	uint32_t *data;
	int i;

	data = malloc(8192);
	for (i = 0; i < 2048 ; i += 2)
		otp_read_data(i, &data[i]);

	_otp_print_key(shell, data);
	free(data);
}

static int otp_prog_data(const struct shell *shell,
						 struct otp_image_layout *image_layout, uint32_t *data)
{
	int i;
	int ret;
	uint32_t *buf;
	uint32_t *buf_ignore;

	buf = (uint32_t *)image_layout->data;
	buf_ignore = (uint32_t *)image_layout->data_ignore;
	shell_printf(shell, "Start Programing...\n");

	/* programing ecc region first */
	for (i = 1792; i < 2046; i += 2) {
		ret = otp_prog_verify_2dw(&data[i], &buf[i], &buf_ignore[i], i);
		if (ret != OTP_SUCCESS) {
			shell_printf(shell,
						 "address: %08x, data: %08x %08x, buffer: %08x %08x, mask: %08x %08x\n",
						 i, data[i], data[i + 1], buf[i], buf[i + 1],
						 buf_ignore[i], buf_ignore[i + 1]);
			return ret;
		}
	}

	for (i = 0; i < 1792; i += 2) {
		ret = otp_prog_verify_2dw(&data[i], &buf[i], &buf_ignore[i], i);
		if (ret != OTP_SUCCESS) {
			shell_printf(shell,
						 "address: %08x, data: %08x %08x, buffer: %08x %08x, mask: %08x %08x\n",
						 i, data[i], data[i + 1], buf[i], buf[i + 1],
						 buf_ignore[i], buf_ignore[i + 1]);
			return ret;
		}
	}
	otp_soak(0);
	return OTP_SUCCESS;
}

static int otp_prog_strap(struct otp_image_layout *image_layout, struct otpstrap_status *os)
{
	uint32_t *strap;
	uint32_t *strap_ignore;
	uint32_t *strap_pro;
	uint32_t prog_address;
	int i;
	int bit, pbit, ibit, offset;
	int fail = 0;
	int ret;
	int prog_flag = 0;

	strap = (uint32_t *)image_layout->strap;
	strap_pro = (uint32_t *)image_layout->strap_pro;
	strap_ignore = (uint32_t *)image_layout->strap_ignore;

	for (i = 0; i < 64; i++) {
		prog_address = 0x800;
		if (i < 32) {
			offset = i;
			bit = (strap[0] >> offset) & 0x1;
			ibit = (strap_ignore[0] >> offset) & 0x1;
			pbit = (strap_pro[0] >> offset) & 0x1;
			prog_address |= ((os[i].writeable_option * 2 + 16) / 8) * 0x200;
			prog_address |= ((os[i].writeable_option * 2 + 16) % 8) * 0x2;

		} else {
			offset = (i - 32);
			bit = (strap[1] >> offset) & 0x1;
			ibit = (strap_ignore[1] >> offset) & 0x1;
			pbit = (strap_pro[1] >> offset) & 0x1;
			prog_address |= ((os[i].writeable_option * 2 + 17) / 8) * 0x200;
			prog_address |= ((os[i].writeable_option * 2 + 17) % 8) * 0x2;
		}

		if (ibit == 1)
			continue;
		if (bit == os[i].value)
			prog_flag = 0;
		else
			prog_flag = 1;

		if (os[i].protected == 1 && prog_flag) {
			fail = 1;
			continue;
		}
		if (os[i].remain_times == 0 && prog_flag) {
			fail = 1;
			continue;
		}

		if (prog_flag) {
			ret = otp_prog_dc_b(1, prog_address, offset);
			if (ret)
				return OTP_FAILURE;
		}

		if (pbit != 0) {
			prog_address = 0x800;
			if (i < 32)
				prog_address |= 0x60c;
			else
				prog_address |= 0x60e;

			ret = otp_prog_dc_b(1, prog_address, offset);
			if (ret)
				return OTP_FAILURE;
		}
	}
	otp_soak(0);
	if (fail == 1)
		return OTP_FAILURE;
	return OTP_SUCCESS;
}

static int otp_prog_conf(const struct shell *shell,
						 struct otp_image_layout *image_layout, uint32_t *otp_conf)
{
	int i, k;
	int pass = 0;
	uint32_t prog_address;
	uint32_t compare[2];
	uint32_t *conf = (uint32_t *)image_layout->conf;
	uint32_t *conf_ignore = (uint32_t *)image_layout->conf_ignore;
	uint32_t data_masked;
	uint32_t buf_masked;

	shell_printf(shell, "Start Programing...\n");
	otp_soak(0);
	for (i = 0; i < 16; i++) {
		data_masked = otp_conf[i]  & ~conf_ignore[i];
		buf_masked  = conf[i] & ~conf_ignore[i];
		prog_address = 0x800;
		prog_address |= (i / 8) * 0x200;
		prog_address |= (i % 8) * 0x2;
		if (data_masked == buf_masked) {
			pass = 1;
			continue;
		}

		otp_soak(1);
		otp_prog_dw(conf[i], conf_ignore[i], prog_address);

		pass = 0;
		for (k = 0; k < RETRY; k++) {
			if (verify_dw(prog_address, &conf[i], &conf_ignore[i], compare, 1) != 0) {
				otp_soak(2);
				otp_prog_dw(compare[0], conf_ignore[i], prog_address);
				if (verify_dw(prog_address, &conf[i], &conf_ignore[i], compare, 1) != 0) {
					otp_soak(1);
				} else {
					pass = 1;
					break;
				}
			} else {
				pass = 1;
				break;
			}
		}
		if (pass == 0) {
			shell_printf(shell, "address: %08x, otp_conf: %08x, input_conf: %08x, mask: %08x\n",
						 i, otp_conf[i], conf[i], conf_ignore[i]);
			break;
		}
	}

	otp_soak(0);
	if (!pass)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_prog_scu_protect(const struct shell *shell,
								struct otp_image_layout *image_layout, uint32_t *scu_pro)
{
	int i, k;
	int pass = 0;
	uint32_t prog_address;
	uint32_t compare[2];
	uint32_t *OTPSCU = (uint32_t *)image_layout->scu_pro;
	uint32_t *OTPSCU_IGNORE = (uint32_t *)image_layout->scu_pro_ignore;
	uint32_t data_masked;
	uint32_t buf_masked;

	shell_printf(shell, "Start Programing...\n");
	otp_soak(0);
	for (i = 0; i < 2; i++) {
		data_masked = scu_pro[i]  & ~OTPSCU_IGNORE[i];
		buf_masked  = OTPSCU[i] & ~OTPSCU_IGNORE[i];
		prog_address = 0xe08 + i * 2;
		if (data_masked == buf_masked) {
			pass = 1;
			continue;
		}

		otp_soak(1);
		otp_prog_dw(OTPSCU[i], OTPSCU_IGNORE[i], prog_address);

		pass = 0;
		for (k = 0; k < RETRY; k++) {
			if (verify_dw(prog_address, &OTPSCU[i], &OTPSCU_IGNORE[i], compare, 1) != 0) {
				otp_soak(2);
				otp_prog_dw(compare[0], OTPSCU_IGNORE[i], prog_address);
				if (verify_dw(prog_address, &OTPSCU[i], &OTPSCU_IGNORE[i], compare, 1) != 0) {
					otp_soak(1);
				} else {
					pass = 1;
					break;
				}
			} else {
				pass = 1;
				break;
			}
		}
		if (pass == 0) {
			shell_printf(shell, "OTPCFG0x%x: 0x%08x, input: 0x%08x, mask: 0x%08x\n",
						 i + 28, scu_pro[i], OTPSCU[i], OTPSCU_IGNORE[i]);
			break;
		}
	}

	otp_soak(0);
	if (!pass)
		return OTP_FAILURE;

	return OTP_SUCCESS;
}

static int otp_check_data_image(const struct shell *shell,
								struct otp_image_layout *image_layout, uint32_t *data)
{
	int data_dw;
	uint32_t data_masked;
	uint32_t buf_masked;
	uint32_t *buf = (uint32_t *)image_layout->data;
	uint32_t *buf_ignore = (uint32_t *)image_layout->data_ignore;
	int i;

	data_dw = image_layout->data_length / 4;
	/* ignore last two dw, the last two dw is used for slt otp write check. */
	for (i = 0; i < data_dw - 2; i++) {
		data_masked = data[i]  & ~buf_ignore[i];
		buf_masked  = buf[i] & ~buf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		if (i % 2 == 0) {
			if ((data_masked | buf_masked) == buf_masked) {
				continue;
			} else {
				shell_printf(shell, "Input image can't program into OTP, please check.\n");
				shell_printf(shell, "OTP_ADDR[0x%x] = 0x%x\n", i, data[i]);
				shell_printf(shell, "Input   [0x%x] = 0x%x\n", i, buf[i]);
				shell_printf(shell, "Mask    [0x%x] = 0x%x\n", i, ~buf_ignore[i]);
				return OTP_FAILURE;
			}
		} else {
			if ((data_masked & buf_masked) == buf_masked) {
				continue;
			} else {
				shell_printf(shell, "Input image can't program into OTP, please check.\n");
				shell_printf(shell, "OTP_ADDR[0x%x] = 0x%x\n", i, data[i]);
				shell_printf(shell, "Input   [0x%x] = 0x%x\n", i, buf[i]);
				shell_printf(shell, "Mask    [0x%x] = 0x%x\n", i, ~buf_ignore[i]);
				return OTP_FAILURE;
			}
		}
	}
	return OTP_SUCCESS;
}

static int otp_check_strap_image(const struct shell *shell,
								 struct otp_image_layout *image_layout, struct otpstrap_status *os)
{
	int i;
	uint32_t *strap;
	uint32_t *strap_ignore;
	uint32_t *strap_pro;
	int bit, pbit, ibit;
	int fail = 0;
	int ret;

	strap = (uint32_t *)image_layout->strap;
	strap_pro = (uint32_t *)image_layout->strap_pro;
	strap_ignore = (uint32_t *)image_layout->strap_ignore;

	for (i = 0; i < 64; i++) {
		if (i < 32) {
			bit = (strap[0] >> i) & 0x1;
			ibit = (strap_ignore[0] >> i) & 0x1;
			pbit = (strap_pro[0] >> i) & 0x1;
		} else {
			bit = (strap[1] >> (i - 32)) & 0x1;
			ibit = (strap_ignore[1] >> (i - 32)) & 0x1;
			pbit = (strap_pro[1] >> (i - 32)) & 0x1;
		}

		ret = otp_strap_bit_confirm(shell, &os[i], i, ibit, bit, pbit);

		if (ret == OTP_FAILURE)
			fail = 1;
	}
	if (fail == 1) {
		shell_printf(shell, "Input image can't program into OTP, please check.\n");
		return OTP_FAILURE;
	}
	return OTP_SUCCESS;
}

static int otp_check_conf_image(const struct shell *shell,
								struct otp_image_layout *image_layout, uint32_t *otp_conf)
{
	uint32_t *conf = (uint32_t *)image_layout->conf;
	uint32_t *conf_ignore = (uint32_t *)image_layout->conf_ignore;
	uint32_t data_masked;
	uint32_t buf_masked;
	int i;

	for (i = 0; i < 16; i++) {
		data_masked = otp_conf[i]  & ~conf_ignore[i];
		buf_masked  = conf[i] & ~conf_ignore[i];
		if (data_masked == buf_masked)
			continue;
		if ((data_masked | buf_masked) == buf_masked) {
			continue;
		} else {
			shell_printf(shell, "Input image can't program into OTP, please check.\n");
			shell_printf(shell, "OTPCFG[%X] = %x\n", i, otp_conf[i]);
			shell_printf(shell, "Input [%X] = %x\n", i, conf[i]);
			shell_printf(shell, "Mask  [%X] = %x\n", i, ~conf_ignore[i]);
			return OTP_FAILURE;
		}
	}
	return OTP_SUCCESS;
}

static int otp_check_scu_image(const struct shell *shell,
							   struct otp_image_layout *image_layout, uint32_t *scu_pro)
{
	uint32_t *OTPSCU = (uint32_t *)image_layout->scu_pro;
	uint32_t *OTPSCU_IGNORE = (uint32_t *)image_layout->scu_pro_ignore;
	uint32_t data_masked;
	uint32_t buf_masked;
	int i;

	for (i = 0; i < 2; i++) {
		data_masked = scu_pro[i]  & ~OTPSCU_IGNORE[i];
		buf_masked  = OTPSCU[i] & ~OTPSCU_IGNORE[i];
		if (data_masked == buf_masked)
			continue;
		if ((data_masked | buf_masked) == buf_masked) {
			continue;
		} else {
			shell_printf(shell, "Input image can't program into OTP, please check.\n");
			shell_printf(shell, "OTPCFG[0x%X] = 0x%X\n", 28 + i, scu_pro[i]);
			shell_printf(shell, "Input [0x%X] = 0x%X\n", 28 + i, OTPSCU[i]);
			shell_printf(shell, "Mask  [0x%X] = 0x%X\n", 28 + i, ~OTPSCU_IGNORE[i]);
			return OTP_FAILURE;
		}
	}
	return OTP_SUCCESS;
}

static int otp_verify_image(uint8_t *src_buf, uint32_t length, uint8_t *digest_buf)
{
	struct SHA256_CTX ctx;
	uint8_t digest_ret[CHECKSUM_LEN];

	sha256_init(&ctx);
	sha256_update(&ctx, src_buf, length);
	sha256_final(&ctx, digest_ret);

	if (!memcmp(digest_buf, digest_ret, CHECKSUM_LEN))
		return OTP_SUCCESS;
	return OTP_FAILURE;
}

static int otp_prog_image(const struct shell *shell, int addr, int nconfirm)
{
	int ret;
	int image_soc_ver = 0;
	struct otp_header *otp_header;
	struct otp_image_layout image_layout;
	int image_size;
	uint8_t *buf;
	uint8_t *checksum;
	int i;
	uint32_t data[2048];
	uint32_t conf[16];
	uint32_t scu_pro[2];

	/*
	 *	otp_header = map_physmem(addr, sizeof(struct otp_header), MAP_WRBACK);
	 *	if (!otp_header) {
	 *		shell_printf(shell, "Failed to map physical memory\n");
	 *		return OTP_FAILURE;
	 *	}
	 *	image_size = OTP_IMAGE_SIZE(otp_header->image_info);
	 *	unmap_physmem(otp_header, MAP_WRBACK);
	 *
	 *	buf = map_physmem(addr, image_size + CHECKSUM_LEN, MAP_WRBACK);
	 */

	if (!buf) {
		shell_printf(shell, "Failed to map physical memory\n");
		return OTP_FAILURE;
	}
	otp_header = (struct otp_header *)buf;
	checksum = buf + otp_header->checksum_offset;

	if (strcmp(OTP_MAGIC, (char *)otp_header->otp_magic) != 0) {
		shell_printf(shell, "Image is invalid\n");
		return OTP_FAILURE;
	}

	image_layout.data_length = (int)(OTP_REGION_SIZE(otp_header->data_info) / 2);
	image_layout.data = buf + OTP_REGION_OFFSET(otp_header->data_info);
	image_layout.data_ignore = image_layout.data + image_layout.data_length;

	image_layout.conf_length = (int)(OTP_REGION_SIZE(otp_header->config_info) / 2);
	image_layout.conf = buf + OTP_REGION_OFFSET(otp_header->config_info);
	image_layout.conf_ignore = image_layout.conf + image_layout.conf_length;

	image_layout.strap = buf + OTP_REGION_OFFSET(otp_header->strap_info);
	image_layout.strap_length = (int)(OTP_REGION_SIZE(otp_header->strap_info) / 3);
	image_layout.strap_pro = image_layout.strap + image_layout.strap_length;
	image_layout.strap_ignore = image_layout.strap + 2 * image_layout.strap_length;

	image_layout.scu_pro = buf + OTP_REGION_OFFSET(otp_header->scu_protect_info);
	image_layout.scu_pro_length = (int)(OTP_REGION_SIZE(otp_header->scu_protect_info) / 2);
	image_layout.scu_pro_ignore = image_layout.scu_pro + image_layout.scu_pro_length;

	if (otp_header->soc_ver == SOC_AST1030A0) {
		image_soc_ver = OTP_AST1030A0;
	} else if (otp_header->soc_ver == SOC_AST1030A1) {
		image_soc_ver = OTP_AST1030A1;
	} else if (otp_header->soc_ver == SOC_AST1060A1) {
		image_soc_ver = OTP_AST1060A1;
	} else {
		shell_printf(shell, "Image SOC Version is not supported\n");
		return OTP_FAILURE;
	}

	if (image_soc_ver != info_cb.version) {
		shell_printf(shell, "Version is not match\n");
		return OTP_FAILURE;
	}

	if (otp_header->otptool_ver != OTPTOOL_VERSION(1, 0, 0)) {
		shell_printf(shell, "OTP image is not generated by otptool v1.0.0\n");
		return OTP_FAILURE;
	}

	if (otp_verify_image(buf, image_size, checksum)) {
		shell_printf(shell, "checksum is invalid\n");
		return OTP_FAILURE;
	}

	if (info_cb.pro_sts.mem_lock) {
		shell_printf(shell, "OTP memory is locked\n");
		return OTP_FAILURE;
	}
	ret = 0;
	if (otp_header->image_info & OTP_INC_DATA) {
		if (info_cb.pro_sts.pro_data) {
			shell_printf(shell, "OTP data region is protected\n");
			ret = -1;
		}
		if (info_cb.pro_sts.pro_sec) {
			shell_printf(shell, "OTP secure region is protected\n");
			ret = -1;
		}
		shell_printf(shell, "Read OTP Data Region:\n");
		for (i = 0; i < 2048 ; i += 2)
			otp_read_data(i, &data[i]);

		shell_printf(shell, "Check writable...\n");
		if (otp_check_data_image(shell, &image_layout, data) == OTP_FAILURE)
			ret = -1;
	}
	if (otp_header->image_info & OTP_INC_CONFIG) {
		if (info_cb.pro_sts.pro_conf) {
			shell_printf(shell, "OTP config region is protected\n");
			ret = -1;
		}
		shell_printf(shell, "Read OTP Config Region:\n");
		for (i = 0; i < 16 ; i++)
			otp_read_conf(i, &conf[i]);

		shell_printf(shell, "Check writable...\n");
		if (otp_check_conf_image(shell, &image_layout, conf) == OTP_FAILURE)
			ret = -1;
	}
	if (otp_header->image_info & OTP_INC_STRAP) {
		if (info_cb.pro_sts.pro_strap) {
			shell_printf(shell, "OTP strap region is protected\n");
			ret = -1;
		}
		shell_printf(shell, "Read OTP Strap Region:\n");
		otp_strap_status(strap_status);

		shell_printf(shell, "Check writable...\n");
		if (otp_check_strap_image(shell, &image_layout, strap_status) == OTP_FAILURE)
			ret = -1;
	}
	if (otp_header->image_info & OTP_INC_SCU_PRO) {
		if (info_cb.pro_sts.pro_strap) {
			shell_printf(shell, "OTP strap region is protected\n");
			ret = -1;
		}
		shell_printf(shell, "Read SCU Protect Region:\n");
		otp_read_conf(28, &scu_pro[0]);
		otp_read_conf(29, &scu_pro[1]);

		shell_printf(shell, "Check writable...\n");
		if (otp_check_scu_image(shell, &image_layout, scu_pro) == OTP_FAILURE)
			ret = -1;
	}
	if (ret == -1)
		return OTP_FAILURE;

	if (!nconfirm) {
		if (otp_header->image_info & OTP_INC_DATA) {
			shell_printf(shell, "\nOTP data region :\n");
			if (otp_print_data_image(shell, &image_layout) < 0) {
				shell_printf(shell, "OTP data error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_CONFIG) {
			shell_printf(shell, "\nOTP configuration region :\n");
			if (otp_print_conf_image(shell, &image_layout) < 0) {
				shell_printf(shell, "OTP config error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_STRAP) {
			shell_printf(shell, "\nOTP strap region :\n");
			if (otp_print_strap_image(shell, &image_layout) < 0) {
				shell_printf(shell, "OTP strap error, please check.\n");
				return OTP_FAILURE;
			}
		}
		if (otp_header->image_info & OTP_INC_SCU_PRO) {
			shell_printf(shell, "\nOTP scu protect region :\n");
			if (otp_print_scu_image(shell, &image_layout) < 0) {
				shell_printf(shell, "OTP scu protect error, please check.\n");
				return OTP_FAILURE;
			}
		}

		shell_printf(shell, "type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			shell_printf(shell, " Aborting\n");
			return OTP_FAILURE;
		}
	}

	if (otp_header->image_info & OTP_INC_DATA) {
		shell_printf(shell, "programing data region ...\n");
		ret = otp_prog_data(shell, &image_layout, data);
		if (ret != 0) {
			shell_printf(shell, "Error\n");
			return ret;
		}
		shell_printf(shell, "Done\n");
	}
	if (otp_header->image_info & OTP_INC_STRAP) {
		shell_printf(shell, "programing strap region ...\n");
		ret = otp_prog_strap(&image_layout, strap_status);
		if (ret != 0) {
			shell_printf(shell, "Error\n");
			return ret;
		}
		shell_printf(shell, "Done\n");
	}
	if (otp_header->image_info & OTP_INC_SCU_PRO) {
		shell_printf(shell, "programing scu protect region ...\n");
		ret = otp_prog_scu_protect(shell, &image_layout, scu_pro);
		if (ret != 0) {
			shell_printf(shell, "Error\n");
			return ret;
		}
		shell_printf(shell, "Done\n");
	}
	if (otp_header->image_info & OTP_INC_CONFIG) {
		shell_printf(shell, "programing configuration region ...\n");
		ret = otp_prog_conf(shell, &image_layout, conf);
		if (ret != 0) {
			shell_printf(shell, "Error\n");
			return ret;
		}
		shell_printf(shell, "Done\n");
	}

	return OTP_SUCCESS;
}

static int otp_prog_bit(const struct shell *shell,
						int mode, int otp_dw_offset, int bit_offset, int value, int nconfirm)
{
	uint32_t read[2];
	uint32_t prog_address = 0;
	int otp_bit;
	int ret = 0;

	otp_soak(0);
	switch (mode) {
	case OTP_REGION_CONF:
		otp_read_conf(otp_dw_offset, read);
		prog_address = 0x800;
		prog_address |= (otp_dw_offset / 8) * 0x200;
		prog_address |= (otp_dw_offset % 8) * 0x2;
		otp_bit = (read[0] >> bit_offset) & 0x1;
		if (otp_bit == value) {
			shell_printf(shell, "OTPCFG0x%X[0x%X] = %d\n", otp_dw_offset, bit_offset, value);
			shell_printf(shell, "No need to program\n");
			return OTP_SUCCESS;
		}
		if (otp_bit == 1 && value == 0) {
			shell_printf(shell, "OTPCFG0x%X[0x%X] = 1\n", otp_dw_offset, bit_offset);
			shell_printf(shell, "OTP is programmed, which can't be clean\n");
			return OTP_FAILURE;
		}
		shell_printf(shell, "Program OTPCFG0x%X[0x%X] to 1\n", otp_dw_offset, bit_offset);
		break;
	case OTP_REGION_DATA:
		prog_address = otp_dw_offset;

		if (otp_dw_offset % 2 == 0) {
			otp_read_data(otp_dw_offset, read);
			otp_bit = (read[0] >> bit_offset) & 0x1;

			if (otp_bit == 1 && value == 0) {
				shell_printf(shell, "OTPDATA0x%X[0x%X] = 1\n", otp_dw_offset, bit_offset);
				shell_printf(shell, "OTP is programmed, which can't be cleared\n");
				return OTP_FAILURE;
			}
		} else {
			otp_read_data(otp_dw_offset - 1, read);
			otp_bit = (read[1] >> bit_offset) & 0x1;

			if (otp_bit == 0 && value == 1) {
				shell_printf(shell, "OTPDATA0x%X[0x%X] = 1\n", otp_dw_offset, bit_offset);
				shell_printf(shell, "OTP is programmed, which can't be written\n");
				return OTP_FAILURE;
			}
		}
		if (otp_bit == value) {
			shell_printf(shell, "OTPDATA0x%X[0x%X] = %d\n", otp_dw_offset, bit_offset, value);
			shell_printf(shell, "No need to program\n");
			return OTP_SUCCESS;
		}

		shell_printf(shell, "Program OTPDATA0x%X[0x%X] to 1\n", otp_dw_offset, bit_offset);
		break;
	case OTP_REGION_STRAP:
		otp_strap_status(strap_status);
		otp_print_strap(shell, bit_offset, 1);
		ret = otp_strap_bit_confirm(shell, &strap_status[bit_offset], bit_offset, 0, value, 0);
		if (ret == OTP_FAILURE)
			return OTP_FAILURE;
		else if (ret == OTP_PROG_SKIP)
			return OTP_SUCCESS;

		break;
	}

	if (!nconfirm) {
		shell_printf(shell, "type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			shell_printf(shell, " Aborting\n");
			return OTP_FAILURE;
		}
	}

	switch (mode) {
	case OTP_REGION_STRAP:
		ret =  otp_prog_strap_b(shell, bit_offset, value);
		break;
	case OTP_REGION_CONF:
	case OTP_REGION_DATA:
		ret = otp_prog_dc_b(value, prog_address, bit_offset);
		break;
	}
	otp_soak(0);
	if (ret) {
		shell_printf(shell, "OTP cannot be programmed\n");
		shell_printf(shell, "FAILURE\n");
		return OTP_FAILURE;
	}

	shell_printf(shell, "SUCCESS\n");
	return OTP_SUCCESS;
}

static int otp_update_rid(const struct shell *shell, uint32_t update_num, int force)
{
	uint32_t otp_rid[2];
	uint32_t sw_rid[2];
	int rid_num = 0;
	int sw_rid_num = 0;
	int bit_offset;
	int dw_offset;
	int i;
	int ret;

	otp_read_conf(10, &otp_rid[0]);
	otp_read_conf(11, &otp_rid[1]);

	sw_rid[0] = sys_read32(SW_REV_ID0);
	sw_rid[1] = sys_read32(SW_REV_ID1);

	rid_num = get_rid_num(otp_rid);
	sw_rid_num = get_rid_num(sw_rid);

	if (sw_rid_num < 0) {
		shell_printf(shell, "SW revision id is invalid, please check.\n");
		return OTP_FAILURE;
	}

	if (update_num > sw_rid_num) {
		shell_printf(shell, "current SW revision ID: 0x%x\n", sw_rid_num);
		shell_printf(shell, "update number could not bigger than current SW revision id\n");
		return OTP_FAILURE;
	}

	if (rid_num < 0) {
		shell_printf(shell, "Current OTP revision ID cannot handle by this command,\n"
					 "please use 'otp pb' command to update it manually\n");
		otp_print_revid(shell, otp_rid);
		return OTP_FAILURE;
	}

	shell_printf(shell, "current OTP revision ID: 0x%x\n", rid_num);
	otp_print_revid(shell, otp_rid);
	shell_printf(shell, "input update number: 0x%x\n", update_num);

	if (rid_num > update_num) {
		shell_printf(shell, "OTP rev_id is bigger than 0x%X\n", update_num);
		shell_printf(shell, "Skip\n");
		return OTP_FAILURE;
	} else if (rid_num == update_num) {
		shell_printf(shell, "OTP rev_id is same as input\n");
		shell_printf(shell, "Skip\n");
		return OTP_FAILURE;
	}

	for (i = rid_num; i < update_num; i++) {
		if (i < 32) {
			dw_offset = 0xa;
			bit_offset = i;
		} else {
			dw_offset = 0xb;
			bit_offset = i - 32;
		}
		shell_printf(shell, "OTPCFG0x%X[0x%X]", dw_offset, bit_offset);
		if (i + 1 != update_num)
			shell_printf(shell, ", ");
	}

	shell_printf(shell, " will be programmed\n");
	if (force == 0) {
		shell_printf(shell, "type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			shell_printf(shell, " Aborting\n");
			return OTP_FAILURE;
		}
	}

	ret = 0;
	for (i = rid_num; i < update_num; i++) {
		if (i < 32) {
			dw_offset = 0xa04;
			bit_offset = i;
		} else {
			dw_offset = 0xa06;
			bit_offset = i - 32;
		}
		if (otp_prog_dc_b(1, dw_offset, bit_offset)) {
			shell_printf(shell, "OTPCFG0x%X[0x%X] programming failed\n", dw_offset, bit_offset);
			ret = OTP_FAILURE;
			break;
		}
	}
	otp_soak(0);
	otp_read_conf(10, &otp_rid[0]);
	otp_read_conf(11, &otp_rid[1]);
	rid_num = get_rid_num(otp_rid);
	if (rid_num >= 0)
		shell_printf(shell, "OTP revision ID: 0x%x\n", rid_num);
	else
		shell_printf(shell, "OTP revision ID\n");
	otp_print_revid(shell, otp_rid);
	if (!ret)
		shell_printf(shell, "SUCCESS\n");
	else
		shell_printf(shell, "FAILED\n");
	return ret;
}

static int otp_retire_key(const struct shell *shell, uint32_t retire_id, int force)
{
	uint32_t otpcfg4;
	uint32_t krb;
	uint32_t krb_b;
	uint32_t krb_or;
	uint32_t current_id;

	otp_read_conf(4, &otpcfg4);
	current_id = sys_read32(SEC_KEY_NUM) & 7;
	krb = otpcfg4 & 0xff;
	krb_b = (otpcfg4 >> 16) & 0xff;
	krb_or = krb | krb_b;

	shell_printf(shell, "current Key ID: 0x%x\n", current_id);
	shell_printf(shell, "input retire ID: 0x%x\n", retire_id);
	shell_printf(shell, "OTPCFG0x4 = 0x%X\n", otpcfg4);

	if (info_cb.pro_sts.pro_key_ret) {
		shell_printf(shell, "OTPCFG4 is protected\n");
		return OTP_FAILURE;
	}

	if (retire_id >= current_id) {
		shell_printf(shell, "Retire key id is equal or bigger than current boot key\n");
		return OTP_FAILURE;
	}

	if (krb_or & (1 << retire_id)) {
		shell_printf(shell, "Key 0x%X already retired\n", retire_id);
		return OTP_SUCCESS;
	}

	shell_printf(shell, "OTPCFG0x4[0x%X] will be programmed\n", retire_id);
	if (force == 0) {
		shell_printf(shell, "type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			shell_printf(shell, " Aborting\n");
			return OTP_FAILURE;
		}
	}

	if (otp_prog_dc_b(1, 0x808, retire_id) == OTP_FAILURE) {
		shell_printf(shell, "OTPCFG0x4[0x%X] programming failed\n", retire_id);
		shell_printf(shell, "try to program backup OTPCFG0x4[0x%X]\n", retire_id + 16);
		if (otp_prog_dc_b(1, 0x808, retire_id + 16) == OTP_FAILURE)
			shell_printf(shell, "OTPCFG0x4[0x%X] programming failed", retire_id + 16);
	}

	otp_soak(0);
	otp_read_conf(4, &otpcfg4);
	krb = otpcfg4 & 0xff;
	krb_b = (otpcfg4 >> 16) & 0xff;
	krb_or = krb | krb_b;
	if (krb_or & (1 << retire_id)) {
		shell_printf(shell, "SUCCESS\n");
		return OTP_SUCCESS;
	}
	shell_printf(shell, "FAILED\n");
	return OTP_FAILURE;
}

static int do_otpread(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t offset, count;
	int ret;

	if (argc == 4) {
		offset = strtoul(argv[2], NULL, 16);
		count = strtoul(argv[3], NULL, 16);
	} else if (argc == 3) {
		offset = strtoul(argv[2], NULL, 16);
		count = 1;
	} else {
		return -EINVAL;
	}
	ret = ast_otp_init(shell);
	if (ret)
		goto end;

	if (!strcmp(argv[1], "conf"))
		ret = otp_print_conf(shell, offset, count);
	else if (!strcmp(argv[1], "data"))
		ret = otp_print_data(shell, offset, count);
	else if (!strcmp(argv[1], "strap"))
		ret = otp_print_strap(shell, offset, count);
	else
		ret = -EINVAL;

end:
	ast_otp_finish();
	return ret;
}

static int do_otppb(const struct shell *shell, size_t argc, char **argv)
{
	int mode = 0;
	int nconfirm = 0;
	int otp_addr = 0;
	int bit_offset;
	int value;
	int ret;
	uint32_t otp_strap_pro;

	if (argc != 4 && argc != 5 && argc != 6)
		return -EINVAL;

	/* Drop the pb cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "conf"))
		mode = OTP_REGION_CONF;
	else if (!strcmp(argv[0], "strap"))
		mode = OTP_REGION_STRAP;
	else if (!strcmp(argv[0], "data"))
		mode = OTP_REGION_DATA;
	else
		return -EINVAL;

	/* Drop the region cmd */
	argc--;
	argv++;

	if (!strcmp(argv[0], "o")) {
		nconfirm = 1;
		/* Drop the force option */
		argc--;
		argv++;
	}

	if (mode == OTP_REGION_STRAP) {
		bit_offset = strtoul(argv[0], NULL, 16);
		value = strtoul(argv[1], NULL, 16);
		if (bit_offset >= 64 || (value != 0 && value != 1))
			return -EINVAL;
	} else {
		otp_addr = strtoul(argv[0], NULL, 16);
		bit_offset = strtoul(argv[1], NULL, 16);
		value = strtoul(argv[2], NULL, 16);
		if (bit_offset >= 32 || (value != 0 && value != 1))
			return -EINVAL;
		if (mode == OTP_REGION_DATA) {
			if (otp_addr >= 0x800)
				return -EINVAL;
		} else {
			if (otp_addr >= 0x20)
				return -EINVAL;
		}
	}
	if (value != 0 && value != 1)
		return -EINVAL;

	ret = 0;
	if (info_cb.pro_sts.mem_lock) {
		shell_printf(shell, "OTP memory is locked\n");
		return -EINVAL;
	}

	ret = ast_otp_init(shell);
	if (ret)
		goto end;

	if (mode == OTP_REGION_DATA) {
		if (info_cb.pro_sts.sec_size == 0) {
			if (info_cb.pro_sts.pro_data) {
				shell_printf(shell, "OTP data region is protected\n");
				ret = -EINVAL;
			}
		} else if (otp_addr < info_cb.pro_sts.sec_size && otp_addr >= 16) {
			shell_printf(shell,
						 "OTP secure region is not readable, skip it to prevent unpredictable result\n");
			ret = -EINVAL;
		} else if (otp_addr < info_cb.pro_sts.sec_size) {
			/* header region(0x0~0x40) is still readable even secure region is set. */
			if (info_cb.pro_sts.pro_sec) {
				shell_printf(shell, "OTP secure region is protected\n");
				ret = -EINVAL;
			}
		} else if (info_cb.pro_sts.pro_data) {
			shell_printf(shell, "OTP data region is protected\n");
			ret = -EINVAL;
		}
	} else if (mode == OTP_REGION_CONF) {
		if (otp_addr != 4 && otp_addr != 10 && otp_addr != 11 && otp_addr < 16) {
			if (info_cb.pro_sts.pro_conf) {
				shell_printf(shell, "OTP config region is protected\n");
				ret = -EINVAL;
			}
		} else if (otp_addr == 10 || otp_addr == 11) {
			uint32_t otp_rid[2];
			uint32_t sw_rid[2];
			uint64_t *otp_rid64 = (uint64_t *)otp_rid;
			uint64_t *sw_rid64 = (uint64_t *)sw_rid;

			otp_read_conf(10, &otp_rid[0]);
			otp_read_conf(11, &otp_rid[1]);
			sw_rid[0] = sys_read32(SW_REV_ID0);
			sw_rid[1] = sys_read32(SW_REV_ID1);

			if (otp_addr == 10)
				otp_rid[0] |= 1 << bit_offset;
			else
				otp_rid[1] |= 1 << bit_offset;

			if (*otp_rid64 > *sw_rid64) {
				shell_printf(shell, "update number could not bigger than current SW revision id\n");
				ret = -EINVAL;
			}
		} else if (otp_addr == 4) {
			if (info_cb.pro_sts.pro_key_ret) {
				shell_printf(shell, "OTPCFG4 is protected\n");
				ret = -EINVAL;
			} else {
				if ((bit_offset >= 0 && bit_offset <= 7) ||
					(bit_offset >= 16 && bit_offset <= 23)) {
					uint32_t key_num;
					uint32_t retire;

					key_num = sys_read32(SEC_KEY_NUM) & 3;
					if (bit_offset >= 16)
						retire = bit_offset - 16;
					else
						retire = bit_offset;
					if (retire >= key_num) {
						shell_printf(shell,
									 "Retire key id is equal or bigger than current boot key\n");
						ret = -EINVAL;
					}
				}
			}
		} else if (otp_addr >= 16 && otp_addr <= 31) {
			if (info_cb.pro_sts.pro_strap) {
				shell_printf(shell, "OTP strap region is protected\n");
				ret = -EINVAL;
			} else if (otp_addr < 28) {
				if (otp_addr % 2 == 0)
					otp_read_conf(30, &otp_strap_pro);
				else
					otp_read_conf(31, &otp_strap_pro);
				if (otp_strap_pro >> bit_offset & 0x1) {
					shell_printf(shell, "OTPCFG0x%X[0x%X] is protected\n", otp_addr, bit_offset);
					ret = -EINVAL;
				}
			}
		}
	} else if (mode == OTP_REGION_STRAP) {
		/* per bit protection will check in otp_strap_bit_confirm */
		if (info_cb.pro_sts.pro_strap) {
			shell_printf(shell, "OTP strap region is protected\n");
			ret = -EINVAL;
		}
	}

	if (ret)
		goto end;

	ret = otp_prog_bit(shell, mode, otp_addr, bit_offset, value, nconfirm);

	if (ret == OTP_SUCCESS)
		ret = 0;
	else
		ret = -EINVAL;

end:
	ast_otp_finish();
	return ret;
}

static int do_otpinfo(const struct shell *shell, size_t argc, char **argv)
{
	int view = 0;
	int input;
	int ret;

	if (argc != 2 && argc != 3)
		return -EINVAL;

	ret = ast_otp_init(shell);
	if (ret)
		goto end;

	if (!strcmp(argv[1], "conf")) {
		if (argc == 3) {
			input = strtoul(argv[2], NULL, 16);
			otp_print_conf_info(shell, input);
		} else {
			otp_print_conf_info(shell, -1);
		}
	} else if (!strcmp(argv[1], "strap")) {
		if (argc == 3) {
			if (!strcmp(argv[2], "v")) {
				view = 1;
				/* Drop the view option */
				argc--;
				argv++;
			}
		}
		otp_print_strap_info(shell, view);
	} else if (!strcmp(argv[1], "scu")) {
		otp_print_scu_info(shell);
	} else if (!strcmp(argv[1], "key")) {
		otp_print_key_info(shell);
	} else {
		ret = -EINVAL;
	}

end:
	ast_otp_finish();
	return ret;
}

static int do_otpprotect(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t input;
	uint32_t bit_offset;
	uint32_t prog_address;
	char force;
	int ret;

	if (argc != 3 && argc != 2)
		return -EINVAL;

	if (!strcmp(argv[1], "o")) {
		input = strtoul(argv[2], NULL, 16);
		force = 1;
	} else {
		input = strtoul(argv[1], NULL, 16);
		force = 0;
	}

	if (input < 32) {
		bit_offset = input;
		prog_address = 0xe0c;
	} else if (input < 64) {
		bit_offset = input - 32;
		prog_address = 0xe0e;
	} else {
		return -EINVAL;
	}

	if (info_cb.pro_sts.pro_strap) {
		shell_printf(shell, "OTP strap region is protected\n");
		return -EINVAL;
	}

	if (!force) {
		shell_printf(shell, "OTPSTRAP[0x%X] will be protected\n", input);
		shell_printf(shell, "type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			shell_printf(shell, " Aborting\n");
			return -EINVAL;
		}
	}

	ret = ast_otp_init(shell);
	if (ret)
		goto end;

	if (verify_bit(prog_address, bit_offset, 1) == 0) {
		shell_printf(shell, "OTPSTRAP[0x%X] already protected\n", input);
		ret = 0;
		goto end;
	}

	ret = otp_prog_dc_b(1, prog_address, bit_offset);
	otp_soak(0);

	if (ret) {
		shell_printf(shell, "Protect OTPSTRAP[0x%X] fail\n", input);
		ret = -EINVAL;
		goto end;
	}

	shell_printf(shell, "OTPSTRAP[0x%X] is protected\n", input);
	ret = 0;

end:
	ast_otp_finish();
	return ret;
}

static int do_otp_scuprotect(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t scu_offset;
	uint32_t bit_offset;
	uint32_t conf_offset;
	uint32_t prog_address;
	char force;
	int ret;

	if (argc != 4 && argc != 3)
		return -EINVAL;

	if (!strcmp(argv[1], "o")) {
		scu_offset = strtoul(argv[2], NULL, 16);
		bit_offset = strtoul(argv[3], NULL, 16);
		force = 1;
	} else {
		scu_offset = strtoul(argv[1], NULL, 16);
		bit_offset = strtoul(argv[2], NULL, 16);
		force = 0;
	}
	if (scu_offset == 0x500) {
		prog_address = 0xe08;
		conf_offset = 28;
	} else if (scu_offset == 0x510) {
		prog_address = 0xe0a;
		conf_offset = 29;
	} else {
		return -EINVAL;
	}
	if (bit_offset < 0 || bit_offset > 31)
		return -EINVAL;
	if (info_cb.pro_sts.pro_strap) {
		shell_printf(shell, "OTP strap region is protected\n");
		return -EINVAL;
	}
	if (!force) {
		shell_printf(shell, "OTPCONF0x%X[0x%X] will be programmed\n", conf_offset, bit_offset);
		shell_printf(shell, "SCU0x%X[0x%X] will be protected\n", scu_offset, bit_offset);
		shell_printf(shell, "type \"YES\" (no quotes) to continue:\n");
		if (!confirm_yesno()) {
			shell_printf(shell, " Aborting\n");
			return -EINVAL;
		}
	}

	ret = ast_otp_init(shell);
	if (ret)
		goto end;

	if (verify_bit(prog_address, bit_offset, 1) == 0) {
		shell_printf(shell, "OTPCONF0x%X[0x%X] already programmed\n", conf_offset, bit_offset);
		ret = 0;
		goto end;
	}

	ret = otp_prog_dc_b(1, prog_address, bit_offset);
	otp_soak(0);

	if (ret) {
		shell_printf(shell, "Program OTPCONF0x%X[0x%X] fail\n", conf_offset, bit_offset);
		ret = -EINVAL;
		goto end;
	}

	shell_printf(shell, "OTPCONF0x%X[0x%X] programmed success\n", conf_offset, bit_offset);
	ret = 0;

end:
	ast_otp_finish();
	return ret;
}

static int do_otpver(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ret = ast_otp_init(shell);
	shell_printf(shell, "SOC OTP version: %s\n", info_cb.ver_name);
	shell_printf(shell, "OTP tool version: %s\n", OTP_VER);
	shell_printf(shell, "OTP info version: %s\n", OTP_INFO_VER);
	ast_otp_finish();

	return ret;
}

static int do_otpupdate(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t update_num;
	int force = 0;
	int ret;

	if (argc == 3) {
		if (strcmp(argv[1], "o"))
			return -EINVAL;
		force = 1;
		update_num = strtoul(argv[2], NULL, 16);
	} else if (argc == 2) {
		update_num = strtoul(argv[1], NULL, 16);
	} else {
		return -EINVAL;
	}

	if (update_num > 64)
		return -EINVAL;

	ret = ast_otp_init(shell);
	if (ret) {
		ast_otp_finish();
		return ret;
	}
	ret = otp_update_rid(shell, update_num, force);
	ast_otp_finish();

	if (ret)
		return -EINVAL;
	return 0;
}

static int do_otprid(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t otp_rid[2];
	uint32_t sw_rid[2];
	int rid_num = 0;
	int sw_rid_num = 0;
	int ret;

	if (argc != 1)
		return -EINVAL;

	ret = ast_otp_init(shell);
	if (ret)
		goto end;

	otp_read_conf(10, &otp_rid[0]);
	otp_read_conf(11, &otp_rid[1]);

	sw_rid[0] = sys_read32(SW_REV_ID0);
	sw_rid[1] = sys_read32(SW_REV_ID1);

	rid_num = get_rid_num(otp_rid);
	sw_rid_num = get_rid_num(sw_rid);

	if (sw_rid_num < 0) {
		shell_printf(shell, "SW revision id is invalid, please check.\n");
		shell_printf(shell, "SEC68:0x%x\n", sw_rid[0]);
		shell_printf(shell, "SEC6C:0x%x\n", sw_rid[1]);
	} else {
		shell_printf(shell, "current SW revision ID: 0x%x\n", sw_rid_num);
	}
	if (rid_num >= 0) {
		shell_printf(shell, "current OTP revision ID: 0x%x\n", rid_num);
		ret = 0;
	} else {
		shell_printf(shell, "Current OTP revision ID cannot handle by 'otp update',\n"
					 "please use 'otp pb' command to update it manually\n"
					 "current OTP revision ID\n");
		ret = -EINVAL;
	}
	otp_print_revid(shell, otp_rid);

end:
	ast_otp_finish();
	return ret;
}

static int do_otpretire(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t retire_id;
	int force = 0;
	int ret;

	if (argc == 3) {
		if (strcmp(argv[1], "o"))
			return -EINVAL;
		force = 1;
		retire_id = strtoul(argv[2], NULL, 16);
	} else if (argc == 2) {
		retire_id = strtoul(argv[1], NULL, 16);
	} else {
		return -EINVAL;
	}

	if (retire_id > 7)
		return -EINVAL;
	ret = ast_otp_init(shell);
	if (ret)
		goto end;
	ret = otp_retire_key(shell, retire_id, force);
	if (ret)
		ret = -EINVAL;
end:
	ast_otp_finish();
	return ret;
}

static int ast_otp_init(const struct shell *shell)
{
	struct otp_pro_sts *pro_sts;
	uint32_t ver;
	uint32_t otp_conf0;

	ver = chip_version();
	switch (ver) {
	case OTP_AST1030A0:
		info_cb.version = OTP_AST1030A0;
		info_cb.conf_info = ast1030a0_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(ast1030a0_conf_info);
		info_cb.strap_info = ast1030a0_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(ast1030a0_strap_info);
		info_cb.key_info = ast10xxa0_key_type;
		info_cb.key_info_len = ARRAY_SIZE(ast10xxa0_key_type);
		sprintf(info_cb.ver_name, "AST1030A0");
		break;
	case OTP_AST1030A1:
		info_cb.version = OTP_AST1030A1;
		info_cb.conf_info = ast1030a1_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(ast1030a1_conf_info);
		info_cb.strap_info = ast1030a0_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(ast1030a0_strap_info);
		info_cb.key_info = ast10xxa1_key_type;
		info_cb.key_info_len = ARRAY_SIZE(ast10xxa1_key_type);
		sprintf(info_cb.ver_name, "AST1030A1");
		break;
	case OTP_AST1060A1:
		info_cb.version = OTP_AST1060A1;
		info_cb.conf_info = ast1030a1_conf_info;
		info_cb.conf_info_len = ARRAY_SIZE(ast1030a1_conf_info);
		info_cb.strap_info = ast1030a0_strap_info;
		info_cb.strap_info_len = ARRAY_SIZE(ast1030a0_strap_info);
		info_cb.key_info = ast10xxa1_key_type;
		info_cb.key_info_len = ARRAY_SIZE(ast10xxa1_key_type);
		sprintf(info_cb.ver_name, "AST1060A1");
		break;
	default:
		shell_printf(shell, "SOC is not supported\n");
		return -EINVAL;
	}

	sys_write32(OTP_PASSWD, OTP_PROTECT_KEY); /* password */
	otp_read_conf(0, &otp_conf0);
	pro_sts = &info_cb.pro_sts;

	pro_sts->mem_lock = (otp_conf0 >> 31) & 0x1;
	pro_sts->pro_key_ret = (otp_conf0 >> 29) & 0x1;
	pro_sts->pro_strap = (otp_conf0 >> 25) & 0x1;
	pro_sts->pro_conf = (otp_conf0 >> 24) & 0x1;
	pro_sts->pro_data = (otp_conf0 >> 23) & 0x1;
	pro_sts->pro_sec = (otp_conf0 >> 22) & 0x1;
	pro_sts->sec_size = ((otp_conf0 >> 16) & 0x3f) << 5;

	return 0;
}

static void ast_otp_finish(void)
{
	sys_write32(1, OTP_PROTECT_KEY); /* protect otp controller */
}

SHELL_STATIC_SUBCMD_SET_CREATE(otp_cmds,
							   SHELL_CMD(version, NULL, "", do_otpver),
							   SHELL_CMD_ARG(read, NULL, "", do_otpread, 3, 1),
							   SHELL_CMD_ARG(info, NULL, "", do_otpinfo, 2, 1),
							   SHELL_CMD_ARG(pb, NULL, "", do_otppb, 4, 2),
							   SHELL_CMD_ARG(protect, NULL, "", do_otpprotect, 2, 1),
							   SHELL_CMD_ARG(scuprotect, NULL, "", do_otp_scuprotect, 3, 1),
							   SHELL_CMD_ARG(update, NULL, "", do_otpupdate, 2, 1),
							   SHELL_CMD_ARG(rid, NULL, "", do_otprid, 1, 0),
							   SHELL_CMD_ARG(retire, NULL, "", do_otpretire, 2, 1),
							   SHELL_SUBCMD_SET_END
							  );

SHELL_CMD_ARG_REGISTER(otp, &otp_cmds,
					   "ASPEED One-Time-Programmable sub-system\n"
					   "otp version\n"
					   "otp read conf|data <otp_dw_offset> <dw_count>\n"
					   "otp read strap <strap_bit_offset> <bit_count>\n"
					   "otp info strap [v]\n"
					   "otp info conf [otp_dw_offset]\n"
					   "otp info scu\n"
					   "otp info key\n"
					   /* "otp prog [o] <addr>\n" */
					   "otp pb conf|data [o] <otp_dw_offset> <bit_offset> <value>\n"
					   "otp pb strap [o] <bit_offset> <value>\n"
					   "otp protect [o] <bit_offset>\n"
					   "otp scuprotect [o] <scu_offset> <bit_offset>\n"
					   "otp update [o] <revision_id>\n"
					   "otp retire [o] <key_id>\n"
					   "otp rid\n",
					   NULL, 7, 0);
