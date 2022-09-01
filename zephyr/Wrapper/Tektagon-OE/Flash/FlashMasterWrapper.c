#include "flash/flash_master.h"
#include <flash/flash_aspeed.h>
#include "FlashWrapper.h"

/**
 * Get a set of capabilities supported by the SPI master.
 *
 * @param spi The SPI master to query.
 *
 * @return A capabilities bitmask for the SPI master.
 */
uint32_t Wrapper_flash_master_capabilities (struct flash_master *spi)
{
	return FLASH_CAP_DUAL_1_2_2 | FLASH_CAP_DUAL_1_1_2 | FLASH_CAP_QUAD_4_4_4 |
			FLASH_CAP_QUAD_1_4_4 | FLASH_CAP_QUAD_1_1_4 | FLASH_CAP_3BYTE_ADDR | FLASH_CAP_4BYTE_ADDR;
}

uint32_t Wrapper_SPI_Command_Xfer (struct spi_flash *flash, const struct flash_xfer *Xfer)
{
	return 0;
}

