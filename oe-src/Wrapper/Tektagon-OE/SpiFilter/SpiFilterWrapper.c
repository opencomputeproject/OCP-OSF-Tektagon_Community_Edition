// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the GPIO Handling functions
 */

#include "SpiFilterWrapper.h"
#include "SpiFilter/SpiFilter.h"


static char *spim_devs[4] = {
	"spi_m1",
	"spi_m2",
	"spi_m3",
	"spi_m4"
};

/**
 * Get the port identifier of the SPI filter.
 *
 * @param filter The SPI filter to query.
 *
 * @return The port identifier or an error code.  Use ROT_IS_ERROR to check the return value.
 */
int SpiFilterGetPort(void)
{
	return 0;
}

/**
 * Get SPI filter manufacturer ID
 *
 * @param filter The SPI filter instance to use
 * @param mfg_id ID buffer to fill
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetMfgId(uint8_t *MfgId)
{
	return 0;
}

/**
 * Set SPI filter manufacturer ID
 *
 * @param filter The SPI filter instance to use
 * @param mfg_id ID to set
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterSetMfgId(uint8_t MfgId)
{
	return 0;
}

/**
 * Get the configured size of the flash device.
 *
 * @param filter The SPI filter to query.
 * @param bytes Output for the size of the flash.  If the filter is configured for the maximum
 * size, this value will be SPI_FILTER_MAX_FLASH_SIZE.
 *
 * @return 0 if the size was successfully queried or an error code.
 */
int SpiFilterGetFlashSize(uint32_t *Bytes)
{
	return 0;
}

/**
 * Set the size of the flash device.
 *
 * @param filter The SPI filter to update.
 * @param bytes The number of bytes available in the flash device.  Set this to
 * SPI_FILTER_MAX_FLASH_SIZE to support the maximum sized device.
 *
 * @return 0 if the size was configured successfully or an error code.
 */
int SpiFilterSetFlashSize(uint32_t Bytes)
{
	return 0;
}

/**
 * Get the flash management mode of the SPI filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode Output for the flash management mode.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiGetFilterEnabled(spi_filter_flash_mode *Mode)
{
	return 0;
}

/**
 * Set the flash management mode for the SPI  filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode The flash management mode
 */
int SpiSetFilterEnabled(spi_filter_flash_mode Mode)
{
	return 0;
}

/**
 * Get the flash management mode of the SPI filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode Output for the flash management mode.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiGetFilterMode(spi_filter_flash_mode *Mode)
{
	return 0;
}

/**
 * Set the flash management mode for the SPI  filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode The flash management mode
 */
int SpiSetFilterMode(spi_filter_flash_mode Mode)
{
	return 0;
}

/**
 * Enable or disable the SPI filter.  A disabled SPI filter will block all access from the host
 * to flash.
 *
 * @param filter The SPI filter to update.
 * @param enable Flag indicating if the SPI filter should be enabled or disabled.
 *
 * @return 0 if the SPI filter was configured successfully or an error code.
 */
int SpiEnableFilter(bool Enable)
{
	int ret = 0;

	for (uint32_t i = 0; i < 4; i++) {
		SPI_Monitor_Enable(spim_devs[i], Enable);
	}

	return ret;
}

/**
 * Get SPI filter read-only select for dual flash operation.
 *
 * @param filter The SPI filter to query.
 * @param act_sel Output for the current active RO CS.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetRoCs(spi_filter_cs *ActiveSelect)
{
	return 0;
}

/**
 * Set SPI filter read-only select for dual flash operation.
 *
 * @param filter The SPI filter to update.
 * @param act_sel The RO CS to set.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterSetRoCs(spi_filter_cs ActiveSelect)
{
	return 0;
}

/**
 * Get the current SPI filter byte address mode.
 *
 * @param filter The SPI filter to query.
 * @param mode Output for the current SPI filter address mode.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetAddressByteMode(spi_filter_address_mode *Mode)
{
	return 0;
}

/**
 * Indicate if the SPI filter is configured to prevent address mode switching.
 *
 * @param filter The SPI filter to query.
 * @param fixed Output boolean indicating if the address byte mode is fixed.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetFixedAddressByteMode(bool *Fixed)
{
	return 0;
}

/**
 * Set the current SPI filter byte address mode.  The flash can switch between 3-byte and 4-byte
 * address modes and the filter will track the current address mode.
 *
 * @param filter The SPI filter to update.
 * @param mode Address mode to set.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterSetAddressByteMode(spi_filter_address_mode Mode)
{
	return 0;
}

/**
 * Set the current SPI filter byte address mode.  The flash cannot switch between 3-byte and
 * 4-byte address modes and the filter byte adddress mode will be fixed.
 *
 * @param filter The SPI filter to update.
 * @param mode Address mode to set.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterSetFixedAddressByteMode(
	spi_filter_address_mode Mode)
{
	return 0;
}

/**
 * Get the SPI filter mode that indicates if the write enable command is required to switch
 * address modes
 *
 * @param filter The SPI filter to query.
 * @param required Output boolean indicating if the write enable command is required to switch
 * address modes.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetAddressByteModeWriteEnableRequired(bool *Required)
{
	return 0;
}

/**
 * Set the SPI filter mode that indicates if the write enable command is required to switch
 * address modes
 *
 * @param filter The SPI filter to update.
 * @param require A boolean indicating whether or not the write enable command is required to
 * switch address modes.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterRequireAddressByteModeWriteEnable(bool Require)
{
	return 0;
}

/**
 * Get the SPI filter mode that indicates the address byte mode after device reset
 *
 * @param filter The SPI filter to query.
 * @param mode Output for the address mode on reset.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetResetAddressByteMode(spi_filter_address_mode *Mode)
{
	return 0;
}

/**
 * Set the SPI filter mode that indicates the address mode after reset
 *
 * @param filter The SPI filter to update.
 * @param mode The reset address mode to set.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterSetResetAddressByteMode(spi_filter_address_mode Mode)
{
	return 0;
}

/**
 * Indicate if writes are allowed to all regions of flash in single flash mode.  If they are not
 * allowed, writes outside the read/write regions will be blocked.
 *
 * @param filter The SPI filter to query.
 * @param allowed Output for the single flash write permissions.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterAreAllSingleFlashWritesAllowed(bool *Allowed)
{
	return 0;
}
/**
 * Configure the SPI filter to allow or block writes outside of the defined read/write regions
 * while operating in single flash mode.
 *
 * @param filter The SPI filter to update.
 * @param allowed Flag indicating if writes to read-only regions of flash should be allowed.
 */
int SpiFilterAllowAllSingleFlashWrites(bool Allowed)
{
	return 0;
}

/**
 * Get the SPI filter write enable command status.
 *
 * @param filter The SPI filter to query.
 * @param detected Output indicating if the write enable command was detected.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetWriteEnableDetected(bool *Detected)
{
	return 0;
}

/**
 * Determine if protected flash has been updated.
 *
 * @param filter The SPI filter to query.
 * @param state Output indicating the state of protected flash.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterGetFlashDirtyState(spi_filter_flash_state *State)
{
	return 0;
}

/**
 * Clear the SPI filter flag indicating protected flash has been updated.
 *
 * @param filter The SPI filter to update.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SpiFilterClearFlashDirtyState(void)
{
	return 0;
}

/**
 * Get a SPI filter read/write region.
 *
 * @param filter The SPI filter to query.
 * @param region The filter region to select.  This is a region index, starting with 1.
 * @param start_addr The first address in the filtered region.
 * @param end_addr One past the last address in the filtered region.
 *
 * @return Completion status, 0 if success or an error code.  If the region specified is not
 * supported by the filter, SPI_FILTER_UNSUPPORTED_RW_REGION will be returned.
 */
int SpiGetFilterRwRegion(uint8_t Region, uint32_t *StartAddress, uint32_t *EndAddress)
{
	return 0;
}

/**
 * Set a SPI filter read/write region.
 *
 * @param filter The SPI filter to update.
 * @param region The filter region to modify.  This is a region index, starting with 1.
 * @param start_addr The first address in the filtered region.
 * @param end_addr One past the last address in the filtered region.
 *
 * @return Completion status, 0 if success or an error code.  If the region specified is not
 * supported by the filter, SPI_FILTER_UNSUPPORTED_RW_REGION will be returned.
 */
int SpiSetFilterRwRegion(uint8_t Region, uint32_t StartAddress, uint32_t EndAddress)
{
	int ret = 0;
	int dev_id = 0;

	uint32_t length = EndAddress - StartAddress;

	// need to modify
	ret = Set_SPI_Filter_RW_Region(spim_devs[dev_id], SPI_FILTER_WRITE_PRIV, SPI_FILTER_PRIV_ENABLE, StartAddress, length);

	return ret;
}

/**
 * Clear all read/write regions configured in the SPI filter.
 *
 * @param filter The SPI filter to update.
 *
 * @return 0 if the regions were cleared successfully or an error code.
 */
int SpiClearFilterRwRegions(void)
{
	return 0;
}
