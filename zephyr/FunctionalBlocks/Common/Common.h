//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef COMMON_COMMON_H_
#define COMMON_COMMON_H_

#define CERBERUS_PREVIOUS_VERSION 0x1
/* Cerberus Includes*/
#include <common/signature_verification.h>

#include <crypto/aes.h>
#include <crypto/base64.h>
#include <crypto/ecc.h>
#include <crypto/hash.h>
#include <crypto/rng.h>
#include <crypto/rsa.h>
#include <crypto/x509.h>

#include <Crypto/RsaWrapper.h>
#include <Crypto/SignatureVerificationRsaWrapper.h>

#include <flash/flash.h>
#include <flash/flash_master.h>
#include <flash/spi_flash.h>

#include "CommonFlash/CommonFlash.h"
#include "SpiFilter/SpiFilter.h"

#include <keystore/keystore.h>

#include <manifest/manifest.h>
#include <manifest/pfm/pfm_flash.h>
#include <manifest/pfm/pfm_manager_flash.h>

#include <I2c/I2cWrapper.h>

#define hashStorageLength 256

struct flash *getFlashDeviceInstance(void);
struct flash_master *getFlashMasterInstance(void);
struct hash_engine *get_hash_engine_instance(void);
struct host_state_manager *getHostStateManagerInstance(void);
struct pfm_flash *getPfmFlashInstance(void);
struct signature_verification *getSignatureVerificationInstance(void);
struct spi_flash *getSpiFlashInstance(void);
struct rsa_engine *getRsaEngineInstance(void);
struct i2c_slave_interface *getI2CSlaveEngineInstance(void);
struct SpiFilterEngine *getSpiFilterEngineWrapper(void);

#endif /* COMMON_COMMON_H_ */

#define MAX_BIOS_BOOT_TIME 300
