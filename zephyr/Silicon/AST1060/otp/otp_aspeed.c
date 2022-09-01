/*
 * Copyright (c) 2021 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "otp_aspeed.h"
#include <shell/shell.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/util.h>

#define OTP_PASSWD                              0x349fe38a

#define OTP_BASE                                0x7e6f2000
#define OTP_PROTECT_KEY                 OTP_BASE
#define OTP_COMMAND                             OTP_BASE + 0x4
#define OTP_TIMING                              OTP_BASE + 0x8
#define OTP_ADDR                                OTP_BASE + 0x10
#define OTP_STATUS                              OTP_BASE + 0x14
#define OTP_COMPARE_1                   OTP_BASE + 0x20
#define OTP_COMPARE_2                   OTP_BASE + 0x24
#define OTP_COMPARE_3                   OTP_BASE + 0x28
#define OTP_COMPARE_4                   OTP_BASE + 0x2c
#define SW_REV_ID0                              OTP_BASE + 0x68
#define SW_REV_ID1                              OTP_BASE + 0x6c
#define SEC_KEY_NUM                             OTP_BASE + 0x78

#define ASPEED_REVISION_ID0             0x7e6e2004
#define ASPEED_REVISION_ID1             0x7e6e2014

#define ID0_AST1030A0   0x80000000
#define ID1_AST1030A0   0x80000000
#define ID0_AST1030A1   0x80010000
#define ID1_AST1030A1   0x80010000
#define ID0_AST1060A1   0xA0010000
#define ID1_AST1060A1   0xA0010000

#define OTP_AST1030A0   1
#define OTP_AST1030A1   2
#define OTP_AST1060A1   3

#define OTP_USAGE                               -1
#define OTP_FAILURE                             -2
#define OTP_SUCCESS                             0

#define OTP_KEY_TYPE_RSA_PUB    1
#define OTP_KEY_TYPE_RSA_PRIV   2
#define OTP_KEY_TYPE_AES                3
#define OTP_KEY_TYPE_VAULT              4
#define OTP_KEY_TYPE_HMAC               5
#define OTP_KEY_ECDSA384                6
#define OTP_KEY_ECDSA384P               7

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

struct otpkey_type {
	int value;
	int key_type;
	int need_id;
	char information[110];
};

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

static struct otp_info_cb info_cb;
// static struct otpstrap_status strap_status[64];

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

static void wait_complete(void)
{
	int reg;

	k_usleep(100);
	do {
		reg = sys_read32(OTP_STATUS);
	} while ((reg & 0x6) != 0x6);
}

static void otp_read_conf(uint32_t offset, uint32_t *data)
{
	int config_offset;

	config_offset = 0x800;
	config_offset |= (offset / 8) * 0x200;
	config_offset |= (offset % 8) * 0x2;

	sys_write32(config_offset, OTP_ADDR);   /* Read address */
	sys_write32(0x23b1e361, OTP_COMMAND);   /* trigger read */
	wait_complete();
	data[0] = sys_read32(OTP_COMPARE_1);
}

static int ast_otp_init(void)
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
		// printk("\r\n OTP init: the SOC version is AST1030A0 \r\n");
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
		// printk("\r\n OTP init: the SOC version is AST1030A1 \r\n");
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
		// printk("\r\n OTP init: the SOC version is AST1060A1 \r\n");
		break;
	default:
		printk("SOC is not supported\n");
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

static void otp_write(uint32_t otp_addr, uint32_t data)
{
	sys_write32(otp_addr, OTP_ADDR);        /* write address */
	sys_write32(data, OTP_COMPARE_1);       /* write data */
	sys_write32(0x23b1e362, OTP_COMMAND);   /* write command */
	wait_complete();
}

static void otp_soak(int soak)
{
	switch (soak) {
	case 0:                                         /* default */
		otp_write(0x3000, 0x0);                 /* Write MRA */
		otp_write(0x5000, 0x0);                 /* Write MRB */
		otp_write(0x1000, 0x0);                 /* Write MR */
		break;
	case 1:                                         /* normal program */
		otp_write(0x3000, 0x1320);              /* Write MRA */
		otp_write(0x5000, 0x1008);              /* Write MRB */
		otp_write(0x1000, 0x0024);              /* Write MR */
		sys_write32(0x04191388, OTP_TIMING);    /* 200us */
		break;
	case 2:                                         /* soak program */
		otp_write(0x3000, 0x1320);              /* Write MRA */
		otp_write(0x5000, 0x0007);              /* Write MRB */
		otp_write(0x1000, 0x0100);              /* Write MR */
		sys_write32(0x04193a98, OTP_TIMING);    /* 600us */
		break;
	}
	wait_complete();
}

static void otp_read_data(uint32_t offset, uint32_t *data)
{
	sys_write32(offset, OTP_ADDR);          /* Read address */
	sys_write32(0x23b1e361, OTP_COMMAND);   /* trigger read */
	wait_complete();
	data[0] = sys_read32(OTP_COMPARE_1);
	data[1] = sys_read32(OTP_COMPARE_2);
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

			if (bit_value == 0 && os[j].writeable_option == -1) {
				os[j].writeable_option = option;
			}
			if (bit_value == 1)
				os[j].remain_times--;
			os[j].value ^= bit_value;
			os[j].option_array[option] = bit_value;
		}
		for (j = 32; j < 64; j++) {
			char bit_value = ((OTPSTRAP_RAW[1] >> (j - 32)) & 0x1);

			if (bit_value == 0 && os[j].writeable_option == -1) {
				os[j].writeable_option = option;
			}
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

static int otp_print_strap(int start, int count, struct otpstrap_status *strap_status)
{
	int i, j;
	int remains = 6;

	if (start < 0 || start > 64)
		return OTP_USAGE;

	if ((start + count) < 0 || (start + count) > 64)
		return OTP_USAGE;

	otp_strap_status(strap_status);
/*
		printk("BIT(hex)  Value  Option           Status\n");
		printk("______________________________________________________________________________\n");

		for (i = start; i < start + count; i++) {
			printk("0x%-8X", i);
			printk("%-7d", strap_status[i].value);
			for (j = 0; j < remains; j++)
				printk("%d ", strap_status[i].option_array[j]);
			printk("   ");
			if (strap_status[i].protected == 1) {
				printk("protected and not writable");
			} else {
				printk("not protected ");
				if (strap_status[i].remain_times == 0)
					printk("and no remaining times to write.");
				else
					printk("and still can write %d times", strap_status[i].remain_times);
			}
			printk("\n");
		}
 */
	return OTP_SUCCESS;
}

static int otp_print_conf(uint32_t offset, uint32_t dw_count, uint32_t *buf)
{
	int i;
	uint32_t ret[1];
	if (offset + dw_count > 32)
		return OTP_USAGE;
	otp_soak(0);
	for (i = offset; i < offset + dw_count; i++) {
		otp_read_conf(i, ret);
		// printk("OTPCFG0x%X: 0x%08X\n", i, ret[0]);
		buf[i - offset] = ret[0];
	}
	return OTP_SUCCESS;
}

int do_otpread_conf(uint32_t offset, uint32_t count, uint32_t *buf)
{
	int ret;
	ret = ast_otp_init();
	if (ret)
		goto end;

	ret = otp_print_conf(offset, count, buf);
end:
	ast_otp_finish();
	return ret;
}

static int otp_print_data(uint32_t offset, uint32_t dw_count, uint32_t *buf)
{
	int i;
	uint32_t ret[2];

	if (offset + dw_count > 2048 || offset % 4 != 0)
		return OTP_USAGE;
	otp_soak(0);
	for (i = offset; i < offset + dw_count; i += 2) {
		// printk(i);
		otp_read_data(i, ret);
		/*
			if (i % 4 == 0)
				printk("%03X: %08X %08X ", i * 4, ret[0], ret[1]);
			else
				printk("%08X %08X\n", ret[0], ret[1]);
		*/
		buf[i - offset] = ret[0];
		buf[i - offset + 1] = ret[1];
	}
	return OTP_SUCCESS;
}

int do_otpread_data(uint32_t offset, uint32_t count, uint32_t *buf)
{
	int ret;
	ret = ast_otp_init();
	if (ret)
		goto end;

	ret = otp_print_data(offset, count, buf);
end:
	ast_otp_finish();
	return ret;
}

int do_otpread_strap(uint32_t offset, uint32_t count, struct otpstrap_status *strap_status)
{
	int ret;
	ret = ast_otp_init();
	if (ret)
		goto end;

	ret = otp_print_strap(offset, count, strap_status);
end:
	ast_otp_finish();
	return ret;
}

static int otp_print_conf_info(int input_offset)
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

	printk("DW    BIT        Value       Description\n");
	printk("__________________________________________________________________________\n");
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
		    conf_info[i].value != OTP_REG_VALID_BIT) {
			continue;
		}
		printk("0x%-4X", dw_offset);

		if (conf_info[i].length == 1) {
			printk("0x%-9X", conf_info[i].bit_offset);
		} else {
			printk("0x%-2X:0x%-4X",
			       conf_info[i].bit_offset + conf_info[i].length - 1,
			       conf_info[i].bit_offset);
		}
		printk("0x%-10x", otp_value);

		if (conf_info[i].value == OTP_REG_RESERVED) {
			printk("Reserved\n");
		} else if (conf_info[i].value == OTP_REG_VALUE) {
			printk(conf_info[i].information, otp_value);
			printk("\n");
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
			printk(conf_info[i].information, valid_bit);
			printk("\n");
		} else {
			printk("%s\n", conf_info[i].information);
		}
	}
	return OTP_SUCCESS;
}

static int do_otpinfo_conf(void)
{
	int ret;

	ret = ast_otp_init();
	if (ret)
		goto end;

	otp_print_conf_info(-1);

end:
	ast_otp_finish();
	return ret;
}
/*
static int otp_print_strap_info(int view)
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
			printk("BIT(hex) Value  Remains  Protect   Description\n");
			printk("___________________________________________________________________________________________________\n");
	} else {
			printk("BIT(hex)   Value       Description\n");
			printk("________________________________________________________________________________\n");
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
							printk("0x%-7X", strap_info[i].bit_offset + j);
							printk("0x%-5X", strap_status[bit_offset + j].value);
							printk("%-9d", strap_status[bit_offset + j].remain_times);
							printk("0x%-7X", strap_status[bit_offset + j].protected);
							if (strap_info[i].value == OTP_REG_RESERVED) {
									printk("Reserved\n");
									continue;
							}
							if (length == 1) {
									printk("%s\n", strap_info[i].information);
									continue;
							}

							if (j == 0)
									printk("/%s\n", strap_info[i].information);
							else if (j == length - 1)
									printk("\\ \"\n");
							else
									printk("| \"\n");
					}
			} else {
					if (length == 1) {
							printk("0x%-9X", strap_info[i].bit_offset);
					} else {
							printk("0x%-2X:0x%-4X",
														bit_offset + length - 1, bit_offset);
					}

					printk("0x%-10X", otp_value);

					if (strap_info[i].value != OTP_REG_RESERVED)
							printk("%s\n", strap_info[i].information);
					else
							printk("Reserved\n");
			}
	}

	if (fail)
			return OTP_FAILURE;

	return OTP_SUCCESS;
}
 */
static int do_otpinfo_strap(void)
{
	int ret;

	ret = ast_otp_init();
	if (ret)
		goto end;

	otp_print_strap_info(1);

end:
	ast_otp_finish();
	return ret;
}
