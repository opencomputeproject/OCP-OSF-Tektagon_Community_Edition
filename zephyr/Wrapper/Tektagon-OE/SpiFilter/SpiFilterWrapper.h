// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Timer Handling functions
 */

#ifndef SPIFILTER_WRAPPER_H_
#define SPIFILTER_WRAPPER_H_

#include "spi_filter/spi_filter_interface.h"
#include "spi_filter/spi_filter_aspeed.h"

int SpiFilterGetPort(void);
int SpiFilterGetMfgId(uint8_t *MfgId);
int SpiFilterSetMfgId(uint8_t MfgId);
int SpiFilterGetFlashSize(uint32_t *Bytes);
int SpiFilterSetFlashSize(uint32_t Bytes);
int SpiGetFilterEnabled(spi_filter_flash_mode *Mode);
int SpiSetFilterEnabled(spi_filter_flash_mode Mode);
int SpiGetFilterMode(spi_filter_flash_mode *Mode);
int SpiSetFilterMode(spi_filter_flash_mode Mode);
int SpiEnableFilter(bool Enable);
int SpiFilterGetRoCs(spi_filter_cs *ActiveSelect);
int SpiFilterSetRoCs(spi_filter_cs ActiveSelect);
int SpiFilterGetAddressByteMode(spi_filter_address_mode *Mode);
int SpiFilterGetFixedAddressByteMode(bool *Fixed);
int SpiFilterSetAddressByteMode(spi_filter_address_mode Mode);
int SpiFilterSetFixedAddressByteMode(spi_filter_address_mode Mode);
int SpiFilterGetAddressByteModeWriteEnableRequired(bool *Required);
int SpiFilterRequireAddressByteModeWriteEnable(bool Require);
int SpiFilterGetResetAddressByteMode(spi_filter_address_mode *Mode);
int SpiFilterSetResetAddressByteMode(spi_filter_address_mode Mode);
int SpiFilterAreAllSingleFlashWritesAllowed(bool *Allowed);
int SpiFilterAllowAllSingleFlashWrites(bool Allowed);
int SpiFilterGetWriteEnableDetected(bool *Detected);
int SpiFilterGetFlashDirtyState(spi_filter_flash_state *State);
int SpiFilterClearFlashDirtyState(void);
int SpiGetFilterRwRegion(uint8_t Region, uint32_t *StartAddress, uint32_t *EndAddress);
int SpiSetFilterRwRegion(uint8_t Region, uint32_t StartAddress, uint32_t EndAddress);
int SpiClearFilterRwRegions(void);

#endif /* SPIFILTER_WRAPPER_H_ */
