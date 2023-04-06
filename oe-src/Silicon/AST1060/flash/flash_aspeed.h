/*
 * Copyright (c) 2021 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SPI_API_MIDLEYER_H_
#define ZEPHYR_INCLUDE_SPI_API_MIDLEYER_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

enum {
	SPI_APP_CMD_NOOP                        = 0x00,         /**< No-op */
	SPI_APP_CMD_READ                        = 0x01,
	SPI_APP_CMD_WRITE                       = 0x02,
	SPI_APP_CMD_ERASE_SECTOR                = 0x03,
	SPI_APP_CMD_ERASE_BLOCK                 = 0x04,
	SPI_APP_CMD_ERASE_CHIP                  = 0x05,
	SPI_APP_CMD_GET_FLASH_SECTOR_SIZE       = 0x06,
	SPI_APP_CMD_GET_FLASH_BLOCK_SIZE        = 0x07,
	SPI_APP_CMD_GET_FLASH_PAGE_SIZE         = 0x08,
	SPI_APP_CMD_GET_FLASH_SIZE              = 0x09,
};

enum {
	BMC_SPI                 = 0,
	PCH_SPI,
	ROT_INTERNAL_ACTIVE     = 4,
	ROT_INTERNAL_RECOVERY,
	ROT_INTERNAL_STATE,
	ROT_INTERNAL_INTEL_STATE,
	ROT_INTERNAL_KEY,
	ROT_INTERNAL_LOG,
};

#define ROT_SPI 4

enum {
	MIDLEY_FLASH_CMD_NOOP   = 0x00,                         /**< No-op */
	// MIDLEY_FLASH_CMD_WRSR = 0x01,				/**< Write status register */
	MIDLEY_FLASH_CMD_PP     = 0x02,                         /**< Page program */
	MIDLEY_FLASH_CMD_READ   = 0x03,                         /**< Normal read */
	// MIDLEY_FLASH_CMD_WRDI = 0x04,				/**< Write disable */
	MIDLEY_FLASH_CMD_RDSR   = 0x05,                         /**< Read status register */
	MIDLEY_FLASH_CMD_WREN   = 0x06,                         /**< Write enable */
	// MIDLEY_FLASH_CMD_FAST_READ = 0x0b,		/**< Fast read */
	// MIDLEY_FLASH_CMD_4BYTE_FAST_READ = 0x0c,	/**< Fast read with 4 byte address */
	// MIDLEY_FLASH_CMD_4BYTE_PP = 0x12,			/**< Page program with 4 byte address */
	// MIDLEY_FLASH_CMD_4BYTE_READ = 0x13,		/**< Normal read with 4 byte address */
	// MIDLEY_FLASH_CMD_RDSR3 = 0x15,			/**< Read status register 3 (configuration register) */
	MIDLEY_FLASH_CMD_4K_ERASE = 0x20,                       /**< Sector erase 4kB */
	// MIDLEY_FLASH_CMD_4BYTE_4K_ERASE = 0x21,	/**< Sector erase 4kB with 4 byte address */
	// MIDLEY_FLASH_CMD_WRSR2 = 0x31,			/**< Write status register 2 */
	// MIDLEY_FLASH_CMD_RDSR2 = 0x35,			/**< Read status register 2 */
	// MIDLEY_FLASH_CMD_DUAL_READ = 0x3b,		/**< Dual output read */
	// MIDLEY_FLASH_CMD_4BYTE_DUAL_READ = 0x3c,	/**< Dual output read with 4 byte address */
	// MIDLEY_FLASH_CMD_ALT_WRSR2 = 0x3e,		/**< Alternate Write status register 2 */
	// MIDLEY_FLASH_CMD_ALT_RDSR2 = 0x3f,		/**< Alternate Read status register 2 */
	// MIDLEY_FLASH_CMD_VOLATILE_WREN = 0x50,	/**< Volatile write enabl efor status register 1 */
	// MIDLEY_FLASH_CMD_SFDP = 0x5a,				/**< Read SFDP registers */
	// MIDLEY_FLASH_CMD_RSTEN = 0x66,			/**< Reset enable */
	// MIDLEY_FLASH_CMD_QUAD_READ = 0x6b,		/**< Quad output read */
	// MIDLEY_FLASH_CMD_4BYTE_QUAD_READ = 0x6c,	/**< Quad output read with 4 byte address */
	// MIDLEY_FLASH_CMD_RDSR_FLAG = 0x70,		/**< Read flag status register */
	// MIDLEY_FLASH_CMD_RST = 0x99,				/**< Reset device */
	// MIDLEY_FLASH_CMD_RDID = 0x9f,				/**< Read identification */
	// MIDLEY_FLASH_CMD_RDP = 0xab,				/**< Release from deep power down */
	// MIDLEY_FLASH_CMD_WR_NV_CFG = 0xb1,		/**< Write non-volatile configuration register */
	// MIDLEY_FLASH_CMD_RD_NV_CFG = 0xb5,		/**< Read non-volatile configuration register */
	// MIDLEY_FLASH_CMD_EN4B = 0xb7,				/**< Enter 4-byte mode */
	// MIDLEY_FLASH_CMD_DP = 0xb9,				/**< Deep power down the device */
	// MIDLEY_FLASH_CMD_DIO_READ = 0xbb,			/**< Dual I/O read */
	// MIDLEY_FLASH_CMD_4BYTE_DIO_READ = 0xbc,	/**< Dual I/O read with 4 byte address */
	MIDLEY_FLASH_CMD_CE             = 0xc7,                 /**< Chip erase */
	MIDLEY_FLASH_CMD_64K_ERASE      = 0xd8,                 /**< Block erase 64kB */
	// MIDLEY_FLASH_CMD_4BYTE_64K_ERASE = 0xdc,	/**< Block erase 64kB with 4 byte address */
	// MIDLEY_FLASH_CMD_EX4B = 0xe9,				/**< Exit 4-byte mode */
	// MIDLEY_FLASH_CMD_QIO_READ = 0xeb,			/**< Quad I/O read */
	// MIDLEY_FLASH_CMD_4BYTE_QIO_READ = 0xec,	/**< Quad I/O read with 4 byte address */
	// MIDLEY_FLASH_CMD_ALT_RST = 0xf0,			/**< Alternate reset command supported by some devices. */
};

#if 1

/**
 * Specifies a transaction to be executed by the SPI master.
 */
struct pflash_xfer {
	uint32_t address;       /**< The address for the command. */
	uint8_t *data;          /**< The buffer for the command data. */
	uint32_t length;        /**< The length of the command data. */
	uint8_t cmd;            /**< The flash command code. */
	uint8_t dummy_bytes;    /**< The number of dummy bytes in the transaction. */
	uint8_t mode_bytes;     /**< The number of mode bytes in the transaction. */
	uint16_t flags;         /**< Transaction flags. */
};


/**
 * Defines the interface to the SPI master connected to a flash device.
 */
struct pflash_master {
	/**
	 * Submit a transfer to be executed by the SPI master.
	 *
	 * @param spi The SPI master to use to execute the transfer.
	 * @param xfer The transfer to execute.
	 *
	 * @return 0 if the transfer was executed successfully or an error code.
	 */
	int (*xfer) (struct flash_master *spi, const struct flash_xfer *xfer);

	/**
	 * Get a set of capabilities supported by the SPI master.
	 *
	 * @param spi The SPI master to query.
	 *
	 * @return A capabilities bitmask for the SPI master.
	 */
	uint32_t (*capabilities) (struct flash_master *spi);
};


/**
 * API for interfacing with a flash device.
 */
struct pflash {
	/**
	 * Get the size of the flash device.
	 *
	 * @param flash The flash to query.
	 * @param bytes Output for the number of bytes in the device.
	 *
	 * @return 0 if the device size was successfully read or an error code.
	 */
	int (*get_device_size) (struct flash *flash, uint32_t *bytes);

	/**
	 * Read data from flash.
	 *
	 * @param flash The flash to read from.
	 * @param address The address to start reading from.
	 * @param data The buffer to hold the data that has been read.
	 * @param length The number of bytes to read.
	 *
	 * @return 0 if the bytes were read from flash or an error code.
	 */
	int (*read) (struct flash *flash, uint32_t address, uint8_t *data, size_t length);

	/**
	 * Get the size of a flash page for write operations.
	 *
	 * @param flash The flash to query.
	 * @param bytes Output for the number of bytes in a flash page.
	 *
	 * @return 0 if the page size was successfully read or an error code.
	 */
	int (*get_page_size) (struct flash *flash, uint32_t *bytes);

	/**
	 * Get the minimum number of bytes that must be written to a single flash page.  Writing fewer
	 * bytes than the minimum to any page will still result in a minimum sized write to flash.  The
	 * extra bytes that were written must be erased before they can be written again.
	 *
	 * @param flash The flash to query.
	 * @param bytes Output for the minimum number of bytes for a page write.
	 *
	 * @return 0 if the minimum write size was successfully read or an error code.
	 */
	int (*minimum_write_per_page) (struct flash *flash, uint32_t *bytes);

	/**
	 * Write data to flash.  The flash region being written to needs to be erased prior to writing.
	 *
	 * Writes are achieved most efficiently if they align to page boundaries.
	 *
	 * @param flash The flash to write to.
	 * @param address The address to start writing to.
	 * @param data The data to write.
	 * @param length The number of bytes to write.
	 *
	 * @return The number of bytes written to the flash or an error code.  Use ROT_IS_ERROR to check
	 * the return value.
	 */
	int (*write) (struct flash *flash, uint32_t address, const uint8_t *data, size_t length);

	/**
	 * Get the size of a flash sector for erase operations.
	 *
	 * @param flash The flash to query.
	 * @param bytes Output for the number of bytes in a flash sector.
	 *
	 * @return 0 if the sector size was successfully read or an error code.
	 */
	int (*get_sector_size) (struct flash *flash, uint32_t *bytes);

	/**
	 * Erase a single sector of flash.
	 *
	 * @param flash The flash to erase.
	 * @param sector_addr An address within the sector to erase.  The erase operation will erase the
	 * entire sector that contains the specified address.
	 *
	 * @return 0 if the sector was erased or an error code.
	 */
	int (*sector_erase) (struct flash *flash, uint32_t sector_addr);

	/**
	 * Get the size of a flash block for erase operations.
	 *
	 * @param flash The flash to query.
	 * @param bytes Output for the number of bytes in a flash block.
	 * @return 0 if the block size was successfully read or an error code.
	 */
	int (*get_block_size) (struct flash *flash, uint32_t *bytes);

	/**
	 * Erase a block of flash.
	 *
	 * @param flash The flash to erase.
	 * @param block_addr An address within the block to erase.  The erase operation will erase the
	 * entire block that contains the specified address.
	 *
	 * @return 0 if the block was erased or an error code.
	 */
	int (*block_erase) (struct flash *flash, uint32_t block_addr);
	/**
	 * Erase the entire flash device.
	 *
	 * @param flash The flash to erase.
	 *
	 * @return 0 if the all flash memory was erased or an error code.
	 */
	int (*chip_erase) (struct flash *flash);
};

/**
 *  * Flash command codes to use for different operations.
 *   */
struct pspi_flash_commands {
	uint8_t read;                                   /**< The command code to read data from flash. */
	uint8_t read_dummy;                             /**< The number of read dummy bytes. */
	uint8_t read_mode;                              /**< The number of read mode bytes. */
	uint16_t read_flags;                            /**< Transfer flags for read requests. */
	uint8_t write;                                  /**< The command code to write data to flash. */
	uint16_t write_flags;                           /**< Transfer flags for write requests. */
	uint8_t erase_sector;                           /**< The command to erase a 4kB sector. */
	uint16_t sector_flags;                          /**< Transfer flags for sector erase requests. */
	uint8_t erase_block;                            /**< The command to erase a 64kB block. */
	uint16_t block_flags;                           /**< Transfer flags for block erase requests. */
	uint8_t reset;                                  /**< The command to soft reset the device. */
	uint8_t enter_pwrdown;                          /**< The command to enter deep powerdown. */
	uint8_t release_pwrdown;                        /**< The command to release deep powerdown. */
};

/* Zephyr mutex. */
typedef struct k_sem platform_mutex;

/**
 *  * Supported methods for entering and exiting 4-byte addressing mode.
 *   */
enum pspi_flash_sfdp_4byte_addressing {
	pSPI_FLASH_SFDP_4BYTE_MODE_UNSUPPORTED,                 /**< 4-byte addressing is not supported. */
	pSPI_FLASH_SFDP_4BYTE_MODE_COMMAND,                     /**< Use a command to switch the mode. */
	pSPI_FLASH_SFDP_4BYTE_MODE_COMMAND_WRITE_ENABLE,        /**< Issue write enable before mode switch. */
	pSPI_FLASH_SFDP_4BYTE_MODE_FIXED,                       /**< Device is permanently in 4-byte mode. */
};

/**
 *  * Mechanisms defined for enabling QSPI.
 *   */
enum pspi_flash_sfdp_quad_enable {
	pSPI_FLASH_SFDP_QUAD_NO_QE_BIT = 0,                     /**< No quad enable bit is necessary. */
	pSPI_FLASH_SFDP_QUAD_QE_BIT1_SR2,                       /**< Quad enable is bit 1 in status register 2. */
	pSPI_FLASH_SFDP_QUAD_QE_BIT6_SR1,                       /**< Quad enable is bit 6 in status register 1. */
	pSPI_FLASH_SFDP_QUAD_QE_BIT7_SR2,                       /**< Quad enable is bit 7 in status register 2. */
	pSPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_NO_CLR,                /**< Quad enable is bit 1 in status register 2, without inadvertant clearing. */
	pSPI_FLASH_SFDP_QUAD_QE_BIT1_SR2_35,                    /**< Quad enable is bit 1 in status register 2, using 35 to read. */
	pSPI_FLASH_SFDP_QUAD_NO_QE_HOLD_DISABLE = 8,            /**< No quad enable bit, but HOLD/RESET can be disabled. */
};

/**
 *  * Interface to a single SPI flash.
 *   */
struct pspi_flash {
	struct pflash base;                                     /**< Base flash instance. */
	struct flash_master *spi;                               /**< The SPI master connected to the flash device. */
	platform_mutex lock;                                    /**< Synchronization lock for accessing the flash. */
	uint16_t addr_mode;                                     /**< The current address mode of the SPI flash device. */
	uint8_t device_id[3];                                   /**< Device identification data. */
	uint32_t device_size;                                   /**< The total capacity of the flash device. */
	struct pspi_flash_commands command;                     /**< Commands to use with the flash device. */
	uint32_t capabilities;                                  /**< Capabilities of the flash device. */
	bool use_fast_read;                                     /**< Flag to use fast read for SPI reads. */
	bool use_busy_flag;                                     /**< Flag to use the busy status instead of WIP. */
	enum pspi_flash_sfdp_4byte_addressing switch_4byte;     /**< Method for switching address mode. */
	bool reset_3byte;                                       /**< Flag to switch to 3-byte mode on reset. */
	enum pspi_flash_sfdp_quad_enable quad_enable;           /**< Method to enable QSPI. */
	bool sr1_volatile;                                      /**< Flag to use volatile write enable for status register 1. */
};

#endif

int SPI_Command_Xfer(struct pspi_flash *flash, struct pflash_xfer *xfer);

#endif
