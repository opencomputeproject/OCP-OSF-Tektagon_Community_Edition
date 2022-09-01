//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the GPIO Handling functions
 */

#include "SpiFilter.h"
#include <stddef.h>

/**
 * Get the port identifier of the SPI filter.
 *
 * @param filter The SPI filter to query.
 *
 * @return The port identifier or an error code.  Use ROT_IS_ERROR to check the return value.
 */
int GetPort(struct spi_filter_interface *Filter)
{
	return SpiFilterGetPort();
}

/**
 * Get SPI filter manufacturer ID
 *
 * @param filter The SPI filter instance to use
 * @param mfg_id ID buffer to fill
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetMfgId(struct spi_filter_interface *Filter, uint8_t *MfgId)
{
	return SpiFilterGetMfgId(MfgId);
}

/**
 * Set SPI filter manufacturer ID
 *
 * @param filter The SPI filter instance to use
 * @param mfg_id ID to set
 *
 * @return Completion status, 0 if success or an error code.
 */
int SetMfgId(struct spi_filter_interface *Filter, uint8_t MfgId)
{
	return SpiFilterSetMfgId(MfgId);
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
int GetFlashSize(struct spi_filter_interface *Filter, uint32_t *Bytes)
{
	return SpiFilterGetFlashSize(Bytes);
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
int SetFlashSize(struct spi_filter_interface *Filter, uint32_t Bytes)
{
	return SpiFilterSetFlashSize(Bytes);
}

/**
 * Get the flash management mode of the SPI filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode Output for the flash management mode.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetFilterEnabled(struct spi_filter_interface *Filter, spi_filter_flash_mode *Mode)
{
	return SpiGetFilterEnabled(Mode);
}

/**
 * Set the flash management mode for the SPI  filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode The flash management mode
 */
int SetFilterEnabled(struct spi_filter_interface *Filter, spi_filter_flash_mode Mode)
{
	return SpiSetFilterEnabled(Mode);
}

/**
 * Get the flash management mode of the SPI filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode Output for the flash management mode.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetFilterMode(struct spi_filter_interface *Filter, spi_filter_flash_mode *Mode)
{
	return SpiGetFilterMode(Mode);
}

/**
 * Set the flash management mode for the SPI  filter.
 *
 * @param filter The SPI filter instance to use
 * @param mode The flash management mode
 */
int SetFilterMode(struct spi_filter_interface *Filter, spi_filter_flash_mode Mode)
{
	return SpiSetFilterMode(Mode);
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
int EnableFilter(struct spi_filter_interface *Filter, bool Enable)
{
	return SpiEnableFilter(Enable);
}

/**
 * Get SPI filter read-only select for dual flash operation.
 *
 * @param filter The SPI filter to query.
 * @param act_sel Output for the current active RO CS.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetRoCs(struct spi_filter_interface *filter, spi_filter_cs *ActiveSelect)
{
	return SpiFilterGetRoCs(ActiveSelect);
}

/**
 * Set SPI filter read-only select for dual flash operation.
 *
 * @param filter The SPI filter to update.
 * @param act_sel The RO CS to set.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SetRoCs(struct spi_filter_interface *Filter, spi_filter_cs ActiveSelect)
{
	return SpiFilterSetRoCs(ActiveSelect);
}

/**
 * Get the current SPI filter byte address mode.
 *
 * @param filter The SPI filter to query.
 * @param mode Output for the current SPI filter address mode.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetAddressByteMode(struct spi_filter_interface *Filter, spi_filter_address_mode *Mode)
{
	return SpiFilterGetAddressByteMode(Mode);
}

/**
 * Indicate if the SPI filter is configured to prevent address mode switching.
 *
 * @param filter The SPI filter to query.
 * @param fixed Output boolean indicating if the address byte mode is fixed.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetFixedAddressByteMode(struct spi_filter_interface *Filter, bool *Fixed)
{
	return SpiFilterGetFixedAddressByteMode(Fixed);
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
int SetAddressByteMode(struct spi_filter_interface *filter, spi_filter_address_mode Mode)
{
	return SpiFilterSetAddressByteMode(Mode);
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
int SetFixedAddressByteMode(struct spi_filter_interface *Filter,
	spi_filter_address_mode Mode)
{
	return SpiFilterSetFixedAddressByteMode(Mode);	
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
int GetAddressByteModeWriteEnableRequired(struct spi_filter_interface *Filter,
	bool *Required)
{
	return SpiFilterGetAddressByteModeWriteEnableRequired(Required);	
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
int RequireAddressByteModeWriteEnable(struct spi_filter_interface *Filter, bool Require)
{
	return SpiFilterRequireAddressByteModeWriteEnable(Require);
}

/**
 * Get the SPI filter mode that indicates the address byte mode after device reset
 *
 * @param filter The SPI filter to query.
 * @param mode Output for the address mode on reset.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetResetAddressByteMode(struct spi_filter_interface *Filter,
	spi_filter_address_mode *Mode)
{
	return SpiFilterGetResetAddressByteMode(Mode);
}

/**
 * Set the SPI filter mode that indicates the address mode after reset
 *
 * @param filter The SPI filter to update.
 * @param mode The reset address mode to set.
 *
 * @return Completion status, 0 if success or an error code.
 */
int SetResetAddressByteMode(struct spi_filter_interface *Filter,
	spi_filter_address_mode Mode)
{
	return SpiFilterSetResetAddressByteMode(Mode);
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
int AreAllSingleFlashWritesAllowed(struct spi_filter_interface *Filter, bool *Allowed)
{
	return SpiFilterAreAllSingleFlashWritesAllowed(Allowed);
}
/**
 * Configure the SPI filter to allow or block writes outside of the defined read/write regions
 * while operating in single flash mode.
 *
 * @param filter The SPI filter to update.
 * @param allowed Flag indicating if writes to read-only regions of flash should be allowed.
 */
int AllowAllSingleFlashWrites(struct spi_filter_interface *Filter, bool Allowed)
{
	return SpiFilterAllowAllSingleFlashWrites(Allowed);
}

/**
 * Get the SPI filter write enable command status.
 *
 * @param filter The SPI filter to query.
 * @param detected Output indicating if the write enable command was detected.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetWriteEnableDetected(struct spi_filter_interface *Filter, bool *Detected)
{
	return SpiFilterGetWriteEnableDetected(Detected);
}

/**
 * Determine if protected flash has been updated.
 *
 * @param filter The SPI filter to query.
 * @param state Output indicating the state of protected flash.
 *
 * @return Completion status, 0 if success or an error code.
 */
int GetFlashDirtyState(struct spi_filter_interface *Filter,
	spi_filter_flash_state *State)
{
	return SpiFilterGetFlashDirtyState(State);
}

/**
 * Clear the SPI filter flag indicating protected flash has been updated.
 *
 * @param filter The SPI filter to update.
 *
 * @return Completion status, 0 if success or an error code.
 */
int ClearFlashDirtyState(struct spi_filter_interface *ilter)
{
	return SpiFilterClearFlashDirtyState();
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
int GetFilterRwRegion(struct spi_filter_interface *Filter, uint8_t Region,
	uint32_t *StartAddress, uint32_t *EndAddress)
{
	return SpiGetFilterRwRegion(Region,StartAddress,EndAddress);
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
int SetFilterRwRegion(struct spi_filter_interface *Filter, uint8_t Region,
	uint32_t StartAddress, uint32_t EndAddress)
{
	return SpiSetFilterRwRegion(Region,StartAddress,EndAddress);
}

/**
 * Clear all read/write regions configured in the SPI filter.
 *
 * @param filter The SPI filter to update.
 *
 * @return 0 if the regions were cleared successfully or an error code.
 */
int ClearFilterRwRegions(struct spi_filter_interface *Filter)
{
	return SpiClearFilterRwRegions();
}
/**
 *  Initialize SpiFilter
 */
int SpiFilterInit (struct SpiFilterEngine *SpiFilter)
{
	if (SpiFilter == NULL) {
		return SPI_FILTER_INVALID_ARGUMENT;
	}

	memset (SpiFilter, 0, sizeof (struct SpiFilterEngine));

	SpiFilter->base.get_port = GetPort;
	SpiFilter->base.get_mfg_id = GetMfgId;
	SpiFilter->base.set_mfg_id = SetMfgId;
	SpiFilter->base.get_flash_size = GetFlashSize;
	//SpiFilter->set_flash_size = SetFlashSize;
	SpiFilter->base.get_filter_mode = GetFilterMode;
	SpiFilter->base.set_filter_mode = SetFilterMode;
	SpiFilter->base.get_filter_enabled = GetFilterEnabled;
	SpiFilter->base.enable_filter = EnableFilter;
	SpiFilter->base.get_ro_cs = GetRoCs;
	SpiFilter->base.set_ro_cs = SetRoCs;
	SpiFilter->base.get_addr_byte_mode = GetAddressByteMode;
	SpiFilter->base.get_fixed_addr_byte_mode = GetFixedAddressByteMode;
	SpiFilter->base.set_addr_byte_mode = SetAddressByteMode;
	SpiFilter->base.set_fixed_addr_byte_mode = SetFixedAddressByteMode;
	SpiFilter->base.get_addr_byte_mode_write_enable_required = GetAddressByteModeWriteEnableRequired;
	SpiFilter->base.require_addr_byte_mode_write_enable = RequireAddressByteModeWriteEnable;
	SpiFilter->base.get_reset_addr_byte_mode = GetResetAddressByteMode;
	SpiFilter->base.set_reset_addr_byte_mode = SetResetAddressByteMode;
	SpiFilter->base.are_all_single_flash_writes_allowed = AreAllSingleFlashWritesAllowed;
	SpiFilter->base.allow_all_single_flash_writes = AllowAllSingleFlashWrites;
	SpiFilter->base.get_write_enable_detected = GetWriteEnableDetected;
	SpiFilter->base.get_flash_dirty_state = GetFlashDirtyState;
	SpiFilter->base.clear_flash_dirty_state = ClearFlashDirtyState;
	SpiFilter->base.get_filter_rw_region = GetFilterRwRegion;
	SpiFilter->base.set_filter_rw_region = SetFilterRwRegion;
	SpiFilter->base.clear_filter_rw_regions = ClearFilterRwRegions;
	
	return 0;
}

