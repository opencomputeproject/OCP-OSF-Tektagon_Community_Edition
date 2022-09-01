//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <string.h>
#include "Common.h"
#include <I2c/I2c.h>

struct flash flashDevice;
struct flash_master flashMaster;                        /**< Flash master for the PFM flash. */
struct hash_engine hashEngine;                          /**< Hashing engine for validation. */
struct host_state_manager hostStateManager;
struct manifest_flash manifestFlash;
struct pfm_flash pfmFlash;                              /**< PFM instance under test. */
struct pfm_manager_flash pfmManagerFlash;
struct signature_verification signatureVerification;    /**< PFM signature verification. */
struct spi_flash spiFlash;                              /**< Flash where the PFM is stored. */
struct rsa_engine rsaEngineWrapper;
struct i2c_slave_interface I2cSlaveEnginWrapper;

// Zephyr Ported structures
struct SpiEngine spiEngineWrapper;
struct FlashMaster flashEngineWrapper;
struct SpiFilterEngine spiFilterEngineWrapper;

uint8_t hashStorage[hashStorageLength];

bool gBootCheckpointReceived;
int gBMCWatchDogTimer = -1;
int gPCHWatchDogTimer = -1;
uint32_t gMaxTimeout = MAX_BIOS_BOOT_TIME;

struct flash *getFlashDeviceInstance(void)
{
	flashDevice = spiEngineWrapper.spi.base;

	return &flashDevice;
}

struct flash_master *getFlashMasterInstance(void)
{
	return &flashMaster;
}

struct hash_engine *get_hash_engine_instance(void)
{
	return &hashEngine;
}

struct host_state_manager *getHostStateManagerInstance(void)
{
	return &hostStateManager;
}

struct manifest_flash *getManifestFlashInstance(void)
{
	return &manifestFlash;
}

struct pfm_flash *getPfmFlashInstance(void)
{
	return &pfmFlash;
}

struct pfm_manager_flash *getPfmManagerFlashInstance(void)
{
	return &pfmManagerFlash;
}

struct signature_verification *getSignatureVerificationInstance(void)
{
	return &signatureVerification;
}

struct spi_flash *getSpiFlashInstance(void)
{
	return &spiFlash;
}

struct rsa_engine *getRsaEngineInstance(void)
{
	return &rsaEngineWrapper;
}

struct SpiEngine *getSpiEngineWrapper(void)
{
	return &spiEngineWrapper;
}

struct FlashMaster *getFlashEngineWrapper(void)
{
	return &flashEngineWrapper;
}

struct i2c_slave_interface *getI2CSlaveEngineInstance(void)
{
	return &I2cSlaveEnginWrapper;
}

uint8_t *getNewHashStorage(void)
{
	memset(hashStorage, 0, hashStorageLength);

	return hashStorage;
}

struct SpiFilterEngine *getSpiFilterEngineWrapper(void)
{
	return &spiFilterEngineWrapper;
}
