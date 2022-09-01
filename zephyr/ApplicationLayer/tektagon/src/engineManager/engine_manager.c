//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * engine_manager.c
 *
 *  Created on: Dec 13, 2021
 *      Author: presannar
 */

#include <assert.h>

#include "engine_manager.h"
#include "include/definitions.h"
#include <Common.h>
#include "I2c/I2c.h"
#include <Crypto/SignatureVerificationRsaWrapper.h>
#include <crypto/rsa.h>
#include "flash/flash_aspeed.h"

#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_verification.h"
#include "intel_2.0/intel_pfr_provision.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_verification.h"
#include "cerberus/cerberus_pfr_provision.h"

#endif
uint8_t signature[RSA_MAX_KEY_LENGTH];          /**< Buffer for the manifest signature. */
uint8_t platform_id[256];                       /**< Cache for the platform ID. */

static int initialize_I2cSlave(/*struct engine_instances *engineInstances*/)
{
	int status = 0;
	struct i2c_slave_interface *I2CSlaveEngine = getI2CSlaveEngineInstance();

	status = I2cSlaveInit(I2CSlaveEngine);
	if (status)
		return status;
		
	I2CSlaveEngine->InitSlaveDev(I2CSlaveEngine,"I2C_1",0x38);
	return status;
}


static int initialize_crypto(/*struct engine_instances *engineInstances*/)
{
	int status = 0;

	status = HashInitialize(get_hash_engine_instance());
	if (status)
		return status;
	status = RsaInit(getRsaEngineInstance());
	if (status)
		return status;

	return status;
}

static int initialize_flash(void)
{
	int status = 0;

	status = FlashMasterInit(getFlashMasterInstance());
	if (status)
		return status;
	status = FlashInit(getSpiEngineWrapper(), getFlashEngineWrapper());
	if (status)
		return status;

	return status;
}

int initialize_pfm_flash(void)
{
	int status = 0;
	status = pfm_flash_init(getPfmFlashInstance(), getFlashDeviceInstance(), get_hash_engine_instance(), PFM_FLASH_MANIFEST_ADDRESS, signature, RSA_MAX_KEY_LENGTH, platform_id, sizeof(platform_id));
	if (status)
		return status;

	return status;
}
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
int read_rsa_public_key(struct rsa_public_key *public_key)
{
	struct flash *flash_device = getFlashDeviceInstance();
	struct manifest_flash manifestFlash;
	uint32_t public_key_offset, exponent_offset;
	uint16_t module_length;
	uint8_t exponent_length;

	pfr_spi_read(0,PFM_FLASH_MANIFEST_ADDRESS, sizeof(manifestFlash.header), &manifestFlash.header);
	pfr_spi_read(0,PFM_FLASH_MANIFEST_ADDRESS + manifestFlash.header.length, sizeof(module_length), &module_length);
	public_key_offset = PFM_FLASH_MANIFEST_ADDRESS + manifestFlash.header.length + sizeof(module_length);
	public_key->mod_length = module_length;
	
	pfr_spi_read(0,public_key_offset, public_key->mod_length, public_key->modulus);
	exponent_offset = public_key_offset + public_key->mod_length;
	pfr_spi_read(0,exponent_offset, sizeof(exponent_length), &exponent_length);
	int int_exp_length = (int) exponent_length;
	pfr_spi_read(0,exponent_offset + sizeof(exponent_length), int_exp_length, &public_key->exponent);

	return 0;
}

int rsa_verify_signature(struct signature_verification *verification,
			 const uint8_t *digest, size_t length, const uint8_t *signature, size_t sig_length)
{
	struct rsa_public_key rsa_public;

	get_rsa_public_key(ROT_INTERNAL_INTEL_STATE, CERBERUS_ROOT_KEY_ADDRESS, &rsa_public);
	struct rsa_engine *rsa = getRsaEngineInstance();

	return rsa->sig_verify(&rsa, &rsa_public, signature, sig_length, digest, length);
}

int signature_verification_init(struct signature_verification *verification)
{
	int status = 0;

	memset(verification, 0, sizeof(struct signature_verification));

	verification->verify_signature = rsa_verify_signature;

	return status;
}
#endif

int initializeEngines(void)
{
	int status = 0;

	status = initialize_flash();
	assert(status == 0);
	status = initialize_crypto();
	assert(status == 0);
	status = initialize_I2cSlave();
	assert(status == 0);
	status = initialize_pfm_flash();
	if (status)
		return status;

	return status;
}

void uninitializeEngines(void)
{
	pfm_flash_release(getPfmFlashInstance());
}
