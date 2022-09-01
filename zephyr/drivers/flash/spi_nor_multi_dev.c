/*
 * Copyright (c) 2018 Savoir-Faire Linux.
 * Copyright (c) 2020 Peter Bigot Consulting, LLC
 * Copyright (c) 2021 ASPEED Inc.
 *
 * This dirver can support multiple chip selects and
 * multiple SPI flash modes.
 * This driver is heavily inspired from the spi_flash_w25qxxdv.c
 * and spi_nor.c SPI NOR driver.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT jedec_spi_nor

#include <errno.h>
#include <drivers/flash.h>
#include <drivers/spi.h>
#include <init.h>
#include <string.h>
#include <logging/log.h>

#include <drivers/spi.h>
#include <drivers/spi_nor.h>
#include <drivers/jesd216.h>
#include "flash_priv.h"

LOG_MODULE_REGISTER(spi_nor_multi_dev, CONFIG_FLASH_LOG_LEVEL);


#ifndef NSEC_PER_MSEC
#define NSEC_PER_MSEC (NSEC_PER_USEC * USEC_PER_MSEC)
#endif

/* Build-time data associated with the device. */
struct spi_nor_config {
	/* Size of device in bytes, from size property */
	uint32_t flash_size;

	bool broken_sfdp;
	/* Optional bits in SR to be cleared on startup.
	 *
	 * This information cannot be derived from SFDP.
	 */
	uint8_t has_lock;

	uint32_t spi_max_buswidth;
	uint32_t spi_ctrl_caps_mask;
	uint32_t spi_nor_caps_mask;
};

/**
 * struct spi_nor_data - Structure for defining the SPI NOR access
 * @spi: The SPI device
 * @spi_cfg: The SPI configuration
 * @sem: The semaphore to access to the flash
 */
struct spi_nor_data {
	char *dev_name;
	uint8_t jedec_id[SPI_NOR_MAX_ID_LEN];
	struct k_sem sem;
	const struct device *spi;
	struct spi_config spi_cfg;

	/* Miscellaneous flags */

	/* If set addressed operations should use 32-bit rather than
	 * 24-bit addresses.
	 *
	 * This is ignored if the access parameter to a command
	 * explicitly specifies 24-bit or 32-bit addressing.
	 */
	bool flag_access_32bit: 1;

	struct spi_nor_cmd_info cmd_info;

	/* Minimal SFDP stores no dynamic configuration.  Runtime and
	 * devicetree store page size and erase_types; runtime also
	 * stores flash size and layout.
	 */
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];

	/* Number of bytes per page */
	uint16_t page_size;
	uint32_t sector_size;
	enum spi_nor_cap cap_mask;

	/* Size of flash, in bytes */
	uint32_t flash_size;
	bool jedec_4bai_support: 1;

	struct flash_pages_layout layout;

	struct flash_parameters flash_nor_parameter;

	bool init_4b_mode_once;
};

#define SPI_NOR_PROT_NUM 8

/* following order should be kept in order to get better performance */
struct spi_nor_mode_cap mode_cap_map[SPI_NOR_PROT_NUM] = {
	{JESD216_MODE_444, SPI_NOR_MODE_4_4_4_CAP},
	{JESD216_MODE_144, SPI_NOR_MODE_1_4_4_CAP},
	{JESD216_MODE_114, SPI_NOR_MODE_1_1_4_CAP},
	{JESD216_MODE_222, SPI_NOR_MODE_2_2_2_CAP},
	{JESD216_MODE_122, SPI_NOR_MODE_1_2_2_CAP},
	{JESD216_MODE_112, SPI_NOR_MODE_1_1_2_CAP},
	{JESD216_MODE_111_FAST, SPI_NOR_MODE_1_1_1_FAST_CAP},
	{JESD216_MODE_111, SPI_NOR_MODE_1_1_1_CAP},
};

static int spi_nor_write_protection_set(const struct device *dev,
					bool write_protect);

/* Get pointer to array of supported erase types.  Static const for
 * minimal, data for runtime and devicetree.
 */
static inline const struct jesd216_erase_type *
dev_erase_types(const struct device *dev)
{
	const struct spi_nor_data *data = dev->data;

	return data->erase_types;
}

/* Get the size of the flash device.  Data for runtime, constant for
 * minimal and devicetree.
 */
static inline uint32_t dev_flash_size(const struct device *dev)
{
	const struct spi_nor_data *data = dev->data;
	const struct spi_nor_config *cfg = dev->config;

	/* flash size set in device tree has higher priority */
	if (cfg->flash_size != 0)
		return cfg->flash_size;

	return data->flash_size;
}

/* Get the flash device page size.  Constant for minimal, data for
 * runtime and devicetree.
 */
static inline uint16_t dev_page_size(const struct device *dev)
{
	const struct spi_nor_data *data = dev->data;

	return data->page_size;
}

/* Indicates that an access command includes bytes for the address.
 * If not provided the opcode is not followed by address bytes.
 */
#define NOR_ACCESS_ADDRESSED BIT(0)

/* Indicates that addressed access uses a 24-bit address regardless of
 * spi_nor_data::flag_32bit_addr.
 */
#define NOR_ACCESS_24BIT_ADDR BIT(1)

/* Indicates that addressed access uses a 32-bit address regardless of
 * spi_nor_data::flag_32bit_addr.
 */
#define NOR_ACCESS_32BIT_ADDR BIT(2)

/* Indicates that an access command is performing a write.  If not
 * provided access is a read.
 */
#define NOR_ACCESS_WRITE BIT(7)

/*
 * @brief Send an SPI command
 *
 * @param dev Device struct
 * @param opcode The command to send
 * @param access flags that determine how the command is constructed.
 *        See NOR_ACCESS_*.
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_access(const struct device *const dev,
			  uint8_t opcode, unsigned int access,
			  off_t addr, void *data, size_t length)
{
	struct spi_nor_data *const driver_data = dev->data;
	bool is_addressed = (access & NOR_ACCESS_ADDRESSED) != 0U;
	bool is_write = (access & NOR_ACCESS_WRITE) != 0U;
	uint8_t buf[5] = { 0 };
	struct spi_buf spi_buf[2] = {
		{
			.buf = buf,
			.len = 1,
		},
		{
			.buf = data,
			.len = length
		}
	};

	buf[0] = opcode;
	if (is_addressed) {
		bool access_24bit = (access & NOR_ACCESS_24BIT_ADDR) != 0;
		bool access_32bit = (access & NOR_ACCESS_32BIT_ADDR) != 0;
		bool use_32bit = (access_32bit
				  || (!access_24bit
				      && driver_data->flag_access_32bit));
		union {
			uint32_t u32;
			uint8_t u8[4];
		} addr32 = {
			.u32 = sys_cpu_to_be32(addr),
		};

		if (use_32bit) {
			memcpy(&buf[1], &addr32.u8[0], 4);
			spi_buf[0].len += 4;
		} else {
			memcpy(&buf[1], &addr32.u8[1], 3);
			spi_buf[0].len += 3;
		}
	};

	const struct spi_buf_set tx_set = {
		.buffers = spi_buf,
		.count = (length != 0 && is_write) ? 2 : 1,
	};

	const struct spi_buf_set rx_set = {
		.buffers = spi_buf + 1,
		.count = is_write ? 0 : 1,
	};

	if (is_write) {
		return spi_write(driver_data->spi,
			&driver_data->spi_cfg, &tx_set);
	}

	return spi_transceive(driver_data->spi,
		&driver_data->spi_cfg, &tx_set, &rx_set);
}

#define spi_nor_cmd_read(dev, opcode, dest, length) \
	spi_nor_access(dev, opcode, 0, 0, dest, length)
#define spi_nor_cmd_addr_read(dev, opcode, addr, dest, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_ADDRESSED, addr, dest, length)
#define spi_nor_cmd_write(dev, opcode) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE, 0, NULL, 0)
#define spi_nor_cmd_addr_write(dev, opcode, addr, src, length) \
	spi_nor_access(dev, opcode, NOR_ACCESS_WRITE | NOR_ACCESS_ADDRESSED, \
		       addr, (void *)src, length)

static inline void spi_nor_assign_read_cmd(
	struct spi_nor_data *data,
	enum jesd216_mode_type mode,
	uint8_t opcode, uint8_t dummy_cycle)
{
	data->cmd_info.read_mode = mode;
	data->cmd_info.read_opcode = opcode;
	data->cmd_info.read_dummy = dummy_cycle;
}

static inline void spi_nor_assign_pp_cmd(
	struct spi_nor_data *data,
	enum jesd216_mode_type mode, uint8_t opcode)
{
	data->cmd_info.pp_mode = mode;
	data->cmd_info.pp_opcode = opcode;
}

static inline uint8_t covert_se_size_to_exp(uint32_t size)
{
	uint8_t ret = 0;

	size--;

	while (size != 0) {
		size >>= 1;
		ret++;
	}

	return ret;
}

static inline void spi_nor_assign_se_cmd(
	struct spi_nor_data *data, enum jesd216_mode_type mode,
	uint8_t type, uint8_t opcode, uint32_t size)
{
	if (type < 1)
		return;

	if (data->flash_nor_parameter.write_block_size != 0 &&
		data->flash_nor_parameter.write_block_size != size)
		return;

	data->erase_types[type - 1].cmd = opcode;
	data->erase_types[type - 1].exp = covert_se_size_to_exp(size);
	data->cmd_info.se_opcode = opcode;
	data->sector_size = size;
	data->cmd_info.se_mode = mode;
	data->flash_nor_parameter.write_block_size = data->sector_size;
}

static int spi_nor_op_exec(const struct device *dev,
	struct spi_nor_op_info op_info)
{
	int ret = 0;
	struct spi_nor_data *const driver_data = dev->data;
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)driver_data->spi->api;

	if (api->spi_nor_op && api->spi_nor_op->transceive) {
		LOG_DBG("Using spi_nor op framework");
		ret = api->spi_nor_op->transceive(driver_data->spi,
				&driver_data->spi_cfg, op_info);
		goto end;
	}

	if (op_info.addr_len == 0 && op_info.data_len == 0) {
		ret = spi_nor_cmd_write(dev, op_info.opcode);
	} else if (op_info.addr_len == 0) {
		ret = spi_nor_cmd_read(dev, op_info.opcode, op_info.buf,
			op_info.data_len);
	} else if (op_info.addr_len != 0 &&
		op_info.data_direct == SPI_NOR_DATA_DIRECT_IN) {
		ret = spi_nor_cmd_addr_read(dev, op_info.opcode,
			op_info.addr, op_info.buf, op_info.data_len);
	} else if (op_info.addr_len != 0 &&
		op_info.data_direct == SPI_NOR_DATA_DIRECT_OUT) {
		ret = spi_nor_cmd_addr_write(dev, op_info.opcode,
			op_info.addr, op_info.buf, op_info.data_len);
	} else {
		LOG_ERR("Undefine operation, opcode: %02xh", op_info.opcode);
	}

end:
	if (ret)
		LOG_ERR("%s, op: %02xh, ret = %d", __func__, op_info.opcode, ret);

	return ret;
}

/**
 * @brief Wait until the flash is ready
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * This function should be invoked after every ERASE, PROGRAM, or
 * WRITE_STATUS operation before continuing.  This allows us to assume
 * that the device is ready to accept new commands at any other point
 * in the code.
 *
 * @param dev The device structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_wait_until_ready(const struct device *dev)
{
	int ret;
	uint8_t reg;
	struct spi_nor_op_info op_info =
		SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_RDSR,
			0, 0, 0, &reg, sizeof(reg), SPI_NOR_DATA_DIRECT_IN);
	do {
		ret = spi_nor_op_exec(dev, op_info);
	} while (!ret && (reg & SPI_NOR_WIP_BIT));

	return ret;
}

/*
 * @brief Read content from the SFDP hierarchy
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param addr The address to send
 * @param data The buffer to store or read the value
 * @param length The size of the buffer
 * @return 0 on success, negative errno code otherwise
 */
static int read_sfdp(const struct device *const dev,
		     off_t addr, void *data, size_t length)
{
	/* READ_SFDP requires a 24-bit address followed by a single
	 * byte for a wait state.  This is effected by using 32-bit
	 * address by shifting the 24-bit address up 8 bits.
	 */
	int ret;
	struct spi_nor_data *const driver_data = dev->data;
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)driver_data->spi->api;

	if (api->spi_nor_op && api->spi_nor_op->transceive) {
		LOG_DBG("Using spi_nor op framework");
		struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, JESD216_CMD_READ_SFDP,
				addr, 3, 8, data, length, SPI_NOR_DATA_DIRECT_IN);

		ret = api->spi_nor_op->transceive(driver_data->spi,
				&driver_data->spi_cfg, op_info);
		return ret;
	}

	return spi_nor_access(dev, JESD216_CMD_READ_SFDP,
			      NOR_ACCESS_32BIT_ADDR | NOR_ACCESS_ADDRESSED,
			      addr << 8, data, length);
}

/* Everything necessary to acquire owning access to the device.
 *
 * This means taking the lock and, if necessary, waking the device
 * from deep power-down mode.
 */
static void acquire_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_take(&driver_data->sem, K_FOREVER);
	}

}

/* Everything necessary to release access to the device.
 *
 * This means (optionally) putting the device into deep power-down
 * mode, and releasing the lock.
 */
static void release_device(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_give(&driver_data->sem);
	}
}

static int spi_nor_wren(const struct device *dev)
{
	int ret;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_WREN,
				0, 0, 0, NULL, 0, SPI_NOR_DATA_DIRECT_OUT);

	ret = spi_nor_op_exec(dev, op_info);

	return ret;
}

static int spi_nor_wrdi(const struct device *dev)
{
	int ret;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_WRDI,
				0, 0, 0, NULL, 0, SPI_NOR_DATA_DIRECT_OUT);

	ret = spi_nor_op_exec(dev, op_info);

	return ret;
}


/**
 * @brief Read the status register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 *
 * @return the non-negative value of the status register, or an error code.
 */
static int spi_nor_rdsr(const struct device *dev)
{
	uint8_t reg;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_RDSR,
				0, 0, 0, &reg, sizeof(reg), SPI_NOR_DATA_DIRECT_IN);

	int ret = spi_nor_op_exec(dev, op_info);

	if (ret == 0) {
		ret = reg;
	}

	return ret;
}

static int spi_nor_rdsr2(const struct device *dev)
{
	uint8_t reg;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_RDSR2,
				0, 0, 0, &reg, sizeof(reg), SPI_NOR_DATA_DIRECT_IN);

	int ret = spi_nor_op_exec(dev, op_info);

	if (ret == 0) {
		ret = reg;
	}

	return ret;
}

/**
 * @brief Write the status register.
 *
 * @note The device must be externally acquired before invoking this
 * function.
 *
 * @param dev Device struct
 * @param sr The new value of the status register
 *
 * @return 0 on success or a negative error code.
 */
static int spi_nor_wrsr(const struct device *dev,
			uint8_t sr)
{
	int ret;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_WRSR,
				0, 0, 0, &sr, sizeof(sr), SPI_NOR_DATA_DIRECT_OUT);

	ret = spi_nor_wren(dev);

	if (ret == 0) {
		ret = spi_nor_op_exec(dev, op_info);
		spi_nor_wait_until_ready(dev);
	}

	return ret;
}

static int spi_nor_wrsr2(const struct device *dev,
			uint8_t sr)
{
	int ret;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_WRSR2,
				0, 0, 0, &sr, sizeof(sr), SPI_NOR_DATA_DIRECT_OUT);

	ret = spi_nor_wren(dev);

	if (ret == 0) {
		ret = spi_nor_op_exec(dev, op_info);
		spi_nor_wait_until_ready(dev);
	}

	return ret;
}

/* MXIC, ISSI */
static int spi_nor_sr1_bit6_config(const struct device *dev)
{
	int sr;

	sr = spi_nor_rdsr(dev);
	if (sr < 0)
		return sr;

	if ((sr & BIT(6)) != 0)
		return 0;

	sr |= BIT(6);

	sr = spi_nor_wrsr(dev, (uint8_t)sr);
	if (sr != 0)
		return sr;

	sr = spi_nor_rdsr(dev);
	if ((sr & BIT(6)) == 0) {
		LOG_ERR("Fail to set SR1[6]");
		return -ENOTSUP;
	}

	return 0;
}

/* Winbond */
static int spi_nor_sr2_bit1_config(const struct device *dev)
{
	int sr;

	sr = spi_nor_rdsr2(dev);
	if (sr < 0)
		return sr;

	if ((sr & BIT(1)) != 0)
		return 0;

	sr |= BIT(1);

	sr = spi_nor_wrsr2(dev, (uint8_t)sr);
	if (sr != 0)
		return sr;

	sr = spi_nor_rdsr2(dev);
	if ((sr & BIT(1)) == 0) {
		LOG_ERR("Fail to set SR2[1]");
		return -ENOTSUP;
	}

	return 0;
}

/* Cypress */
static int spi_nor_cf1_bit1_config(const struct device *dev)
{
	int ret;
	uint8_t sr[2];
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_WRSR,
				0, 0, 0, sr, sizeof(sr), SPI_NOR_DATA_DIRECT_OUT);

	ret = spi_nor_rdsr(dev);
	if (ret < 0)
		return ret;
	sr[0] = (uint8_t)ret;

	ret = spi_nor_rdsr2(dev);
	if (ret < 0)
		return ret;
	sr[1] = (uint8_t)(ret | BIT(1));

	ret = spi_nor_wren(dev);
	if (ret != 0)
		return ret;

	ret = spi_nor_op_exec(dev, op_info);
	spi_nor_wait_until_ready(dev);
	if (ret != 0)
		return ret;

	ret = spi_nor_rdsr2(dev);
	if ((ret & BIT(1)) == 0) {
		LOG_ERR("Fail to set SR2[1]");
		return -ENOTSUP;
	}

	return 0;
}

#define SPI_NOR_QE_NO_NEED        0x0 /* Micron, Gigadevice */
#define SPI_NOR_QE_NOT_SUPPORTED  0x7
#define SPI_NOR_QE_SR1_BIT6       0x2 /* MXIC, ISSI */
#define SPI_NOR_QE_SR2_BIT1       0x4 /* Winbond */
#define SPI_NOR_QE_CF1_BIT1       0x5 /* Cypress */

static int spi_nor_qe_config(const struct device *dev, uint32_t qer)
{
	int ret = 0;
	struct spi_nor_data *data = dev->data;

	acquire_device(dev);

	switch (qer) {
	case SPI_NOR_QE_NO_NEED:
		break;
	case SPI_NOR_QE_SR1_BIT6:
		ret = spi_nor_sr1_bit6_config(dev);
		break;
	case SPI_NOR_QE_SR2_BIT1:
		ret = spi_nor_sr2_bit1_config(dev);
		break;
	case SPI_NOR_QE_CF1_BIT1:
		ret = spi_nor_cf1_bit1_config(dev);
		break;
	case SPI_NOR_QE_NOT_SUPPORTED:
	default:
		LOG_INF("Disable QSPI bit");
		data->cap_mask &= ~SPI_NOR_QUAD_CAP_MASK;
		break;
	}

	release_device(dev);

	return ret;
}

static int winbond_w25q80dv_fixup(const struct device *dev)
{
	int ret = 0;
	struct spi_nor_data *data = dev->data;

	if (data->cap_mask & SPI_NOR_MODE_1_1_4_CAP) {
		spi_nor_assign_read_cmd(data, JESD216_MODE_114, SPI_NOR_CMD_READ_1_1_4, 8);
		/* SFDP is broken on some w25q80dv flash parts.
		 * Thus, force to set QE bit here.
		 */
		ret = spi_nor_sr2_bit1_config(dev);
	}

	return ret;
}

static int spi_nor_read(const struct device *dev, off_t addr, void *dest,
			size_t size)
{
	struct spi_nor_data *data = dev->data;
	struct spi_nor_cmd_info cmd_info = data->cmd_info;
	const size_t flash_size = dev_flash_size(dev);
	int ret;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(cmd_info.read_mode, cmd_info.read_opcode,
				addr, data->flag_access_32bit ? 4 : 3, cmd_info.read_dummy,
				dest, size, SPI_NOR_DATA_DIRECT_IN);

	/* should be between 0 and flash size */
	if ((addr < 0) || ((addr + size) > flash_size)) {
		return -EINVAL;
	}

#if CONFIG_SPI_NOR_ADDR_MODE_FALLBACK_DISABLED
	if (data->init_4b_mode_once && !data->flag_access_32bit) {
		ret = spi_nor_config_4byte_mode(dev, true);
		if (ret != 0)
			return ret;

		op_info.addr_len = 4;
	}
#endif

	acquire_device(dev);

	ret = spi_nor_op_exec(dev, op_info);

	release_device(dev);

	return ret;
}

static int spi_nor_write(const struct device *dev, off_t addr,
			 const void *src,
			 size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	const uint16_t page_size = dev_page_size(dev);
	struct spi_nor_data *data = dev->data;
	struct spi_nor_cmd_info cmd_info = data->cmd_info;
	int ret = 0;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(cmd_info.pp_mode, cmd_info.pp_opcode,
				addr, data->flag_access_32bit ? 4 : 3, 0,
				(void *)src, size, SPI_NOR_DATA_DIRECT_OUT);

	/* should be between 0 and flash size */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -EINVAL;
	}

#if CONFIG_SPI_NOR_ADDR_MODE_FALLBACK_DISABLED
	if (data->init_4b_mode_once && !data->flag_access_32bit) {
		ret = spi_nor_config_4byte_mode(dev, true);
		if (ret != 0)
			return ret;

		op_info.addr_len = 4;
	}
#endif

	acquire_device(dev);
	ret = spi_nor_write_protection_set(dev, false);
	if (ret == 0) {
		while (size > 0) {
			size_t to_write = size;

			/* Don't write more than a page. */
			if (to_write >= page_size) {
				to_write = page_size;
			}

			/* Don't write across a page boundary */
			if (((addr + to_write - 1U) / page_size)
			!= (addr / page_size)) {
				to_write = page_size - (addr % page_size);
			}

			ret = spi_nor_wren(dev);
			if (ret != 0)
				break;

			op_info.addr = addr;
			op_info.buf = (void *)src;
			op_info.data_len = to_write;

			ret = spi_nor_op_exec(dev, op_info);
			if (ret != 0) {
				break;
			}

			size -= to_write;
			src = (const uint8_t *)src + to_write;
			addr += to_write;

			spi_nor_wait_until_ready(dev);
		}
	}

	int ret2 = spi_nor_write_protection_set(dev, false);

	if (!ret) {
		ret = ret2;
	}

	release_device(dev);

	return ret;
}

static int spi_nor_erase(const struct device *dev, off_t addr, size_t size)
{
	const size_t flash_size = dev_flash_size(dev);
	int ret = 0;
	struct spi_nor_data *data = dev->data;
	struct spi_nor_cmd_info cmd_info = data->cmd_info;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(cmd_info.se_mode, cmd_info.se_opcode,
				addr, data->flag_access_32bit ? 4 : 3, 0,
				NULL, 0, SPI_NOR_DATA_DIRECT_OUT);

	/* erase area must be subregion of device */
	if ((addr < 0) || ((size + addr) > flash_size)) {
		return -ENODEV;
	}

	/* address must be sector-aligned */
	if (!SPI_NOR_IS_SECTOR_ALIGNED(addr)) {
		return -EINVAL;
	}

	/* size must be a multiple of sectors */
	if ((size % SPI_NOR_SECTOR_SIZE) != 0) {
		return -EINVAL;
	}

#if CONFIG_SPI_NOR_ADDR_MODE_FALLBACK_DISABLED
	if (data->init_4b_mode_once && !data->flag_access_32bit) {
		ret = spi_nor_config_4byte_mode(dev, true);
		if (ret != 0)
			return ret;

		op_info.addr_len = 4;
	}
#endif

	acquire_device(dev);
	ret = spi_nor_write_protection_set(dev, false);

	while ((size > 0) && (ret == 0)) {
		ret = spi_nor_wren(dev);
		if (ret != 0)
			break;

		if (size == flash_size) {
			struct spi_nor_op_info op_ce_info =
			SPI_NOR_OP_INFO(cmd_info.se_mode, SPI_NOR_CMD_CE,
				0, 0, 0, NULL, 0, SPI_NOR_DATA_DIRECT_OUT);
			/* chip erase */
			spi_nor_op_exec(dev, op_ce_info);
			size -= flash_size;
		} else {
			if (size % data->sector_size != 0) {
				LOG_ERR("Erase sz is not multiple of se sz");
				LOG_ERR("sz: %d, se sz: %d", size, data->sector_size);
				ret = -EINVAL;
				break;
			}

			op_info.opcode = cmd_info.se_opcode;
			op_info.addr = addr;

			spi_nor_op_exec(dev, op_info);
			addr += data->sector_size;
			size -= data->sector_size;
		}
		spi_nor_wait_until_ready(dev);
	}

	int ret2 = spi_nor_write_protection_set(dev, true);

	if (!ret) {
		ret = ret2;
	}

	release_device(dev);

	return ret;
}

/* @note The device must be externally acquired before invoking this
 * function.
 */
static int spi_nor_write_protection_set(const struct device *dev,
					bool write_protect)
{
	int ret;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, 0,
				0, 0, 0, NULL, 0, SPI_NOR_DATA_DIRECT_OUT);

	op_info.opcode = (write_protect) ? SPI_NOR_CMD_WRDI : SPI_NOR_CMD_WREN;
	ret = spi_nor_op_exec(dev, op_info);

	if (IS_ENABLED(DT_INST_PROP(0, requires_ulbpr))
	    && (ret == 0)
	    && !write_protect) {
		op_info.opcode = SPI_NOR_CMD_ULBPR;
		ret = spi_nor_op_exec(dev, op_info);
	}

	return ret;
}

static int spi_nor_sfdp_read(const struct device *dev, off_t addr,
			     void *dest, size_t size)
{
	acquire_device(dev);

	int ret = read_sfdp(dev, addr, dest, size);

	release_device(dev);

	return ret;
}

static int spi_nor_read_jedec_id(const struct device *dev,
				 uint8_t *id)
{
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_RDID,
				0, 0, 0, id, SPI_NOR_MAX_ID_LEN, SPI_NOR_DATA_DIRECT_IN);

	if (id == NULL) {
		return -EINVAL;
	}

	acquire_device(dev);

	int ret = spi_nor_op_exec(dev, op_info);

	release_device(dev);

	return ret;
}

/* Put the device into the appropriate address mode, if supported.
 *
 * On successful return spi_nor_data::flag_access_32bit has been set
 * (cleared) if the device is configured for 4-byte (3-byte) addresses
 * for read, write, and erase commands.
 *
 * @param dev the device
 *
 * @param enter_4byte_addr the Enter 4-Byte Addressing bit set from
 * DW16 of SFDP BFP.  A value of all zeros or all ones is interpreted
 * as "not supported".
 *
 * @retval -ENOTSUP if 4-byte addressing is supported but not in a way
 * that the driver can handle.
 * @retval negative codes if the attempt was made and failed
 * @retval 0 if the device is successfully left in 24-bit mode or
 *         reconfigured to 32-bit mode.
 */
static int spi_nor_set_address_mode(const struct device *dev,
				    uint8_t enter_4byte_addr)
{
	int ret = 0;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_4BA,
				0, 0, 0, NULL, 0, SPI_NOR_DATA_DIRECT_OUT);

	/* Do nothing if not provided (either no bits or all bits
	 * set).
	 */
	if ((enter_4byte_addr == 0)
	    || (enter_4byte_addr == 0xff)) {
		return 0;
	}

	LOG_DBG("Checking enter-4byte-addr %02x", enter_4byte_addr);

	/* This currently only supports command 0xB7 (Enter 4-Byte
	 * Address Mode), with or without preceding WREN.
	 */
	if ((enter_4byte_addr & 0x03) == 0) {
		return -ENOTSUP;
	}

	acquire_device(dev);

	if ((enter_4byte_addr & 0x02) != 0) {
		/* Enter after WREN. */
		ret = spi_nor_wren(dev);
	}
	if (ret == 0) {
		ret = spi_nor_op_exec(dev, op_info);
	}

	if (ret == 0) {
		struct spi_nor_data *data = dev->data;

		data->flag_access_32bit = true;
#if CONFIG_SPI_NOR_ADDR_MODE_FALLBACK_DISABLED
		data->init_4b_mode_once = true;
#endif
	}

	release_device(dev);

	return ret;
}

int spi_nor_config_4byte_mode(const struct device *dev, bool en4b)
{
	int ret = 0;
	struct spi_nor_data *data = dev->data;
	struct spi_nor_op_info op_info =
			SPI_NOR_OP_INFO(JESD216_MODE_111, SPI_NOR_CMD_4BA,
				0, 0, 0, NULL, 0, SPI_NOR_DATA_DIRECT_OUT);

	if (!en4b)
		op_info.opcode = SPI_NOR_CMD_EXIT_4BA;

	acquire_device(dev);

	ret = spi_nor_wren(dev);
	if (ret == 0) {
		ret = spi_nor_op_exec(dev, op_info);
	}

	if (ret != 0)
		goto end;

	ret = spi_nor_wrdi(dev);

	if (en4b) {
		data->flag_access_32bit = true;
#if CONFIG_SPI_NOR_ADDR_MODE_FALLBACK_DISABLED
		data->init_4b_mode_once = true;
#endif
	} else {
		data->flag_access_32bit = false;
	}

end:
	release_device(dev);

	return ret;
}

static int spi_nor_process_bfp(const struct device *dev,
			       const struct jesd216_param_header *php,
			       const struct jesd216_bfp *bfp)
{
	struct spi_nor_data *data = dev->data;
	struct jesd216_erase_type *etp = data->erase_types;
	const size_t flash_size = jesd216_bfp_density(bfp) / 8U;
	struct jesd216_instr instr;
	int rc = 0;

	LOG_DBG("%s: %u MiBy flash", dev->name, (uint32_t)(flash_size >> 20));

	/* Copy over the erase types, preserving their order.  (The
	 * Sector Map Parameter table references them by index.)
	 */
	memset(data->erase_types, 0, sizeof(data->erase_types));
	for (uint8_t ti = 1; ti <= ARRAY_SIZE(data->erase_types); ++ti) {
		if (jesd216_bfp_erase(bfp, ti, etp) == 0) {
			LOG_DBG("Erase %u with %02x", (uint32_t)BIT(etp->exp), etp->cmd);
		}
		++etp;
	}

	data->page_size = jesd216_bfp_page_size(php, bfp);
	data->flash_size = flash_size;

	LOG_DBG("Page size %u bytes", data->page_size);

	/* If 4-byte addressing is supported, switch to it. */
	if (jesd216_bfp_addrbytes(bfp) != JESD216_SFDP_BFP_DW1_ADDRBYTES_VAL_3B) {
		struct jesd216_bfp_dw16 dw16;

		if (jesd216_bfp_decode_dw16(php, bfp, &dw16) == 0) {
			rc = spi_nor_set_address_mode(dev, dw16.enter_4ba);
		}

		if (rc != 0) {
			LOG_ERR("Unable to enter 4-byte mode: %d\n", rc);
			return rc;
		}
	}

	struct jesd216_bfp_dw15 dw15;

	if (jesd216_bfp_decode_dw15(php, bfp, &dw15) == 0) {
		rc = spi_nor_qe_config(dev, dw15.qer);
		if (rc != 0) {
			LOG_ERR("Fail to configure QE bit : %d\n", rc);
			return rc;
		}
	}

	/* get read cmd info from bfp */
	for (uint8_t mc = 0; mc < ARRAY_SIZE(mode_cap_map); mc++) {
		if ((data->cap_mask & mode_cap_map[mc].cap) != 0) {
			rc = jesd216_bfp_read_support(php, bfp, mode_cap_map[mc].mode, &instr);
			if (rc > 0) {
				spi_nor_assign_read_cmd(data, mode_cap_map[mc].mode,
					instr.instr, instr.mode_clocks + instr.wait_states);
				break;
			} else if (rc == 0) {
				/* do nothing for unsupport and pre-init mode */
				break;
			}
		}
	}

	LOG_DBG("[bfp][mode] read: %08x, write: %08x, erase: %08x",
		data->cmd_info.read_mode, data->cmd_info.pp_mode, data->cmd_info.se_mode);
	LOG_DBG("[bfp] read op: %02xh (%02x), write op: %02xh, erase op: %02xh",
		data->cmd_info.read_opcode, data->cmd_info.read_dummy,
		data->cmd_info.pp_opcode, data->cmd_info.se_opcode);

	return 0;
}

static int spi_nor_process_4bai(const struct device *dev,
			       uint32_t *jedec_4bai)
{
	struct spi_nor_data *data = dev->data;
	int rv;
	uint8_t cmd;
	uint8_t ti;

	LOG_DBG("4bai dw: [0x%08x, 0x%08x]", jedec_4bai[0], jedec_4bai[1]);

	data->jedec_4bai_support = false;

	rv = jesd216_4bai_read_support(jedec_4bai, data->cmd_info.read_mode, &cmd);
	if (rv < 0)
		goto end;

	data->cmd_info.read_opcode = cmd;

	rv = jesd216_4bai_pp_support(jedec_4bai, data->cmd_info.pp_mode, &cmd);
	if (rv < 0)
		goto end;

	data->cmd_info.pp_opcode = cmd;

	for (ti = 0; ti < JESD216_NUM_ERASE_TYPES; ti++) {
		if (data->erase_types[ti].exp != 0) {
			rv = jesd216_4bai_se_support(jedec_4bai, ti + 1, &cmd);
			if (rv < 0)
				goto end;
			data->erase_types[ti].cmd = cmd;
		}
	}

	if (ti > JESD216_NUM_ERASE_TYPES) {
		rv = -ENOTSUP;
		goto end;
	}

	data->jedec_4bai_support = true;
	data->flag_access_32bit = true;
	data->init_4b_mode_once = true;

end:
	LOG_DBG("[4bai][mode] read: %08x, write: %08x, erase: %08x",
		data->cmd_info.read_mode, data->cmd_info.pp_mode, data->cmd_info.se_mode);
	LOG_DBG("[4bai] read op: %02xh (%02x), write op: %02xh, erase op: %02xh(%dKB)",
		data->cmd_info.read_opcode, data->cmd_info.read_dummy,
		data->cmd_info.pp_opcode, data->cmd_info.se_opcode, data->sector_size / 1024);

	return 0;
}


static int spi_nor_process_sfdp(
	const struct device *dev)
{
	int rc;
	struct spi_nor_data *data = dev->data;
	uint8_t ti;

	/* For runtime we need to read the SFDP table, identify the
	 * BFP block, and process it.
	 */
	const uint8_t decl_nph = 4;
	union {
		/* We only process BFP so use one parameter block */
		uint8_t raw[JESD216_SFDP_SIZE(decl_nph)];
		struct jesd216_sfdp_header sfdp;
	} u;
	const struct jesd216_sfdp_header *hp = &u.sfdp;

	rc = read_sfdp(dev, 0, u.raw, sizeof(u.raw));
	if (rc != 0) {
		LOG_ERR("[%d]SFDP read failed: %d", __LINE__, rc);
		return rc;
	}

	uint32_t magic = jesd216_sfdp_magic(hp);

	if (magic != JESD216_SFDP_MAGIC) {
		LOG_ERR("[%d][%s]SFDP magic %08x invalid",
			__LINE__, dev->name, magic);
		return -EINVAL;
	}

	LOG_DBG("%s: SFDP v %u.%u AP %x with %u PH", dev->name,
		hp->rev_major, hp->rev_minor, hp->access, 1 + hp->nph);

	const struct jesd216_param_header *php = hp->phdr;
	const struct jesd216_param_header *phpe = php + MIN(decl_nph, 1 + hp->nph);

	while (php != phpe) {
		uint16_t id = jesd216_param_id(php);

		LOG_DBG("PH%u: %04x rev %u.%u: %u DW @ %x",
			(php - hp->phdr), id, php->rev_major, php->rev_minor,
			php->len_dw, jesd216_param_addr(php));

		if (id == JESD216_SFDP_PARAM_ID_BFP) {
			union {
				uint32_t dw[MIN(php->len_dw, JESD216_MAX_DWORD)];
				struct jesd216_bfp bfp;
			} u;
			const struct jesd216_bfp *bfp = &u.bfp;

			rc = read_sfdp(dev, jesd216_param_addr(php), u.dw, sizeof(u.dw));
			if (rc == 0) {
				rc = spi_nor_process_bfp(dev, php, bfp);
			}

			if (rc != 0) {
				LOG_INF("SFDP BFP failed: %d", rc);
				break;
			}
		}

		if (id == JESD216_SFDP_PARAM_ID_4B_ADDR_INSTR) {
			uint32_t jedec_4bai_dw[2];

			rc = read_sfdp(dev, jesd216_param_addr(php),
					jedec_4bai_dw, sizeof(jedec_4bai_dw));
			if (rc == 0) {
				rc = spi_nor_process_4bai(dev, jedec_4bai_dw);
			}

			if (rc != 0) {
				LOG_INF("SFDP 4BAI failed: %d", rc);
				break;
			}
		}

		++php;
	}

	for (ti = 0; ti < JESD216_NUM_ERASE_TYPES; ti++) {
		if (data->flash_nor_parameter.write_block_size == 0 &&
				data->erase_types[ti].exp != 0) {
			break;
		}

		if (data->flash_nor_parameter.write_block_size ==
			BIT(data->erase_types[ti].exp)) {
			break;
		}
	}

	if (ti >= JESD216_NUM_ERASE_TYPES || data->erase_types[ti].exp == 0)
		return -EINVAL;

	spi_nor_assign_se_cmd(data, JESD216_MODE_111, ti + 1,
		data->erase_types[ti].cmd, BIT(data->erase_types[ti].exp));

	return rc;
}

#define SPI_NOR_GET_JESDID(id) ((id[1] << 8) | id[2])

static int sfdp_post_fixup(const struct device *dev)
{
	int ret = 0;
	struct spi_nor_data *data = dev->data;
	const struct spi_nor_config *cfg = dev->config;

	switch (data->jedec_id[0]) {
	case SPI_NOR_MFR_ID_WINBOND:
		if (SPI_NOR_GET_JESDID(data->jedec_id) == 0x4014) {
			if (cfg->broken_sfdp)
				ret = winbond_w25q80dv_fixup(dev);
		}
		break;
	default:
		/* do nothing */
		break;
	}

	return ret;
}

static int setup_pages_layout(const struct device *dev)
{
	int rv = 0;

	struct spi_nor_data *data = dev->data;
	const size_t flash_size = dev_flash_size(dev);
	const uint32_t layout_page_size = CONFIG_SPI_NOR_FLASH_LAYOUT_PAGE_SIZE;
	uint8_t exp = 0;

	/* Find the smallest erase size. */
	for (size_t i = 0; i < ARRAY_SIZE(data->erase_types); ++i) {
		const struct jesd216_erase_type *etp = &data->erase_types[i];

		if ((etp->cmd != 0)
		    && ((exp == 0) || (etp->exp < exp))) {
			exp = etp->exp;
		}
	}

	if (exp == 0) {
		return -ENOTSUP;
	}

	uint32_t erase_size = BIT(exp);

	/* Error if layout page size is not a multiple of smallest
	 * erase size.
	 */
	if ((layout_page_size % erase_size) != 0) {
		LOG_ERR("layout page %u not compatible with erase size %u",
			layout_page_size, erase_size);
		return -EINVAL;
	}

	/* Warn but accept layout page sizes that leave inaccessible
	 * space.
	 */
	if ((flash_size % layout_page_size) != 0) {
		LOG_INF("layout page %u wastes space with device size %zu",
			layout_page_size, flash_size);
	}

	data->layout.pages_size = layout_page_size;
	data->layout.pages_count = flash_size / layout_page_size;
	LOG_DBG("layout %u x %u By pages", data->layout.pages_count, data->layout.pages_size);

	return rv;
}

static void spi_nor_info_init_params(
	const struct device *dev)
{
	struct spi_nor_data *data = dev->data;

	memset(&data->cmd_info, 0x0, sizeof(struct spi_nor_cmd_info));

	const struct spi_nor_config *cfg = dev->config;

	data->cap_mask = ~(cfg->spi_ctrl_caps_mask | cfg->spi_nor_caps_mask);

	/* 4-4-4 QPI format is not supported */
	data->cap_mask &= ~SPI_NOR_MODE_4_4_4_CAP;

	if (cfg->spi_max_buswidth < 2)
		data->cap_mask &= ~(SPI_NOR_DUAL_CAP_MASK | SPI_NOR_QUAD_CAP_MASK);
	else if (cfg->spi_max_buswidth < 4)
		data->cap_mask &= ~SPI_NOR_QUAD_CAP_MASK;

	/* initial basic command */
	if (data->cap_mask & SPI_NOR_MODE_1_1_1_FAST_CAP)
		spi_nor_assign_read_cmd(data, JESD216_MODE_111_FAST, SPI_NOR_CMD_READ_FAST, 8);
	else if (data->cap_mask & SPI_NOR_MODE_1_1_1_CAP)
		spi_nor_assign_read_cmd(data, JESD216_MODE_111, SPI_NOR_CMD_READ, 0);

	spi_nor_assign_pp_cmd(data, JESD216_MODE_111, SPI_NOR_CMD_PP);

	spi_nor_assign_se_cmd(data, JESD216_MODE_111, 1, SPI_NOR_CMD_SE, 0x1000);

	data->page_size = 256;
}

/**
 * @brief Configure the flash
 *
 * @param dev The flash device structure
 * @param info The flash info structure
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_configure(const struct device *dev)
{
	struct spi_nor_data *data = dev->data;
	const struct spi_nor_config *cfg = dev->config;
	int rc;

	data->spi = device_get_binding(data->dev_name);
	if (!data->spi) {
		return -EINVAL;
	}

	/* now the spi bus is configured, we can verify SPI
	 * connectivity by reading the JEDEC ID.
	 */

	rc = spi_nor_read_jedec_id(dev, data->jedec_id);
	if (rc != 0) {
		LOG_ERR("JEDEC ID read failed: %d", rc);
		return -ENODEV;
	}

	/* Check for block protect bits that need to be cleared.  This
	 * information cannot be determined from SFDP content, so the
	 * devicetree node property must be set correctly for any device
	 * that powers up with block protect enabled.
	 */
	if (cfg->has_lock != 0) {
		acquire_device(dev);

		rc = spi_nor_rdsr(dev);

		/* Only clear if RDSR worked and something's set. */
		if (rc > 0) {
			rc = spi_nor_wrsr(dev, rc & ~cfg->has_lock);
		}

		if (rc != 0) {
			LOG_ERR("BP clear failed: %d\n", rc);
			return -ENODEV;
		}

		release_device(dev);
	}

	spi_nor_info_init_params(dev);

	if (!cfg->broken_sfdp) {
		rc = spi_nor_process_sfdp(dev);
		if (rc != 0) {
			LOG_ERR("[%d]SFDP read failed: %d", __LINE__, rc);
			return -ENODEV;
		}
	}

	rc = sfdp_post_fixup(dev);
	if (rc != 0)
		return -ENODEV;

	rc = setup_pages_layout(dev);
	if (rc != 0) {
		LOG_ERR("layout setup failed: %d", rc);
		return -ENODEV;
	}

	data->flash_nor_parameter.flash_size = dev_flash_size(dev);

	if (data->flash_size > 0x1000000 && !data->flag_access_32bit) {
		rc = spi_nor_config_4byte_mode(dev, true);
		if (rc != 0)
			return -ENODEV;
	}

	const struct spi_driver_api *api =
			(const struct spi_driver_api *)data->spi->api;

	if (api->spi_nor_op && api->spi_nor_op->read_init) {
		struct spi_nor_op_info read_op_info =
			SPI_NOR_OP_INFO(data->cmd_info.read_mode, data->cmd_info.read_opcode,
				0, data->flag_access_32bit ? 4 : 3, data->cmd_info.read_dummy,
				NULL, dev_flash_size(dev), SPI_NOR_DATA_DIRECT_IN);

		rc = api->spi_nor_op->read_init(data->spi,
				&data->spi_cfg, read_op_info);
		if (rc != 0)
			return -ENODEV;
	}

	if (api->spi_nor_op && api->spi_nor_op->write_init) {
		struct spi_nor_op_info write_op_info =
			SPI_NOR_OP_INFO(data->cmd_info.pp_mode, data->cmd_info.pp_opcode,
				0, data->flag_access_32bit ? 4 : 3, 0,
				NULL, dev_flash_size(dev), SPI_NOR_DATA_DIRECT_OUT);

		rc = api->spi_nor_op->write_init(data->spi,
				&data->spi_cfg, write_op_info);
		if (rc != 0)
			return -ENODEV;
	}

	LOG_DBG("%s: %d MB flash", dev->name, dev_flash_size(dev) >> 20);
	LOG_DBG("bus_width: %d, cap: %08x", cfg->spi_max_buswidth, data->cap_mask);
	LOG_DBG("read: %08x, write: %08x, erase: %08x",
		data->cmd_info.read_mode, data->cmd_info.pp_mode, data->cmd_info.se_mode);
	LOG_DBG("read op: %02xh (%d), write op: %02xh, erase op: %02xh(%dKB)",
		data->cmd_info.read_opcode, data->cmd_info.read_dummy,
		data->cmd_info.pp_opcode, data->cmd_info.se_opcode, data->sector_size / 1024);

	return 0;
}

/**
 * @brief Initialize and configure the flash
 *
 * @param name The flash name
 * @return 0 on success, negative errno code otherwise
 */
static int spi_nor_init(const struct device *dev)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		struct spi_nor_data *const driver_data = dev->data;

		k_sem_init(&driver_data->sem, 1, K_SEM_MAX_LIMIT);
	}

	return spi_nor_configure(dev);
}

static void spi_nor_pages_layout(const struct device *dev,
				 const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	/* Data for runtime, const for devicetree and minimal. */
	const struct spi_nor_data *data = dev->data;

	*layout = &data->layout;
	*layout_size = 1;
}

static const struct flash_parameters *
flash_nor_get_parameters(const struct device *dev)
{
	const struct spi_nor_data *data = dev->data;

	return &data->flash_nor_parameter;
}

static const struct flash_driver_api spi_nor_api = {
	.read = spi_nor_read,
	.write = spi_nor_write,
	.erase = spi_nor_erase,
	.get_parameters = flash_nor_get_parameters,
	.page_layout = spi_nor_pages_layout,
	.sfdp_read = spi_nor_sfdp_read,
	.read_jedec_id = spi_nor_read_jedec_id,
};

#define SPI_NOR_MULTI_INIT(n)	\
	static const struct spi_nor_config spi_nor_config_##n = {	\
		.flash_size = DT_INST_PROP_OR(n, size, 0) / 8,	\
		.broken_sfdp = DT_PROP(DT_INST(n, DT_DRV_COMPAT), broken_sfdp),	\
		.spi_max_buswidth = DT_INST_PROP_OR(n, spi_max_buswidth, 1),	\
		.spi_ctrl_caps_mask =	\
			DT_PROP_OR(DT_PARENT(DT_INST(n, DT_DRV_COMPAT)),	\
				spi_ctrl_caps_mask, 0),	\
		.spi_nor_caps_mask = DT_INST_PROP_OR(n, spi_nor_caps_mask, 0),	\
	};	\
	static struct spi_nor_data spi_nor_data_##n = {	\
		.dev_name = DT_INST_BUS_LABEL(n),	\
		.spi_cfg = {	\
			.frequency = DT_INST_PROP(n, spi_max_frequency),	\
			.operation = SPI_WORD_SET(8),	\
			.slave = DT_INST_REG_ADDR(n),	\
		},	\
		.flash_nor_parameter = {	\
			.write_block_size = DT_INST_PROP_OR(n, write_block_size, 0),	\
			.erase_value = 0xff,	\
			.flash_size = 0,	\
		},	\
		.init_4b_mode_once = false,	\
	};	\
		\
	DEVICE_DT_INST_DEFINE(n, &spi_nor_init, NULL,	\
			 &spi_nor_data_##n, &spi_nor_config_##n,	\
			 POST_KERNEL, CONFIG_SPI_NOR_INIT_PRIORITY,	\
			 &spi_nor_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_NOR_MULTI_INIT)

