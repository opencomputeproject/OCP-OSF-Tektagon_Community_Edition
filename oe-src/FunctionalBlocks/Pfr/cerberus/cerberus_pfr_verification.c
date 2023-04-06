
//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//
#if CONFIG_CERBERUS_PFR_SUPPORT
#include <stdint.h>
#include "state_machine/common_smc.h"
#include "pfr/pfr_common.h"
#include "include/definitions.h"
#include "pfr/pfr_util.h"
#include "cerberus_pfr_definitions.h"
#include "cerberus_pfr_provision.h"
#include "cerberus_pfr_key_cancellation.h"
#include "cerberus_pfr_verification.h"
#include <Common.h>
#include "Definition.h"
#include "flash/flash_store.h"
#include "keystore/keystore_flash.h"
#include <crypto/rsa.h>
#include "keystore/KeystoreManager.h"
#include "engineManager/engine_manager.h"

#undef DEBUG_PRINTF
#if PFR_AUTHENTICATION_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

#ifdef CONFIG_DUAL_SPI
#define DUAL_SPI 1
#else
#define DUAL_SPI 0
#endif

int cerberus_pfr_manifest_verify(struct manifest *manifest, struct hash_engine *hash,
			      struct signature_verification *verification, uint8_t *hash_out, uint32_t hash_length)
{
	int status = 0;	
	uint8_t *hashStorage = getNewHashStorage();
	struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) manifest;
	struct manifest_flash *manifest_flash = getManifestFlashInstance();
	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	status = signature_verification_init(getSignatureVerificationInstance());
	if(status){
		return Failure;
	}

	if(pfr_manifest->image_type == BMC_TYPE){
		get_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, (uint8_t *)&manifest_flash->addr, sizeof(manifest_flash->addr));
		spi_flash->spi.device_id[0] = BMC_FLASH_ID;
	}else if(pfr_manifest->image_type == PCH_TYPE){
		get_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, (uint8_t *)&manifest_flash->addr, sizeof(manifest_flash->addr));
		spi_flash->spi.device_id[0] = PCH_FLASH_ID;
	}

	manifest_flash->flash = &spi_flash->spi.base;
	status = manifest_flash_verify(manifest_flash, get_hash_engine_instance(),
				       getSignatureVerificationInstance(), hashStorage, HASH_STORAGE_LENGTH);

	if (true == manifest_flash->manifest_valid) {
		printk("Manifest Verification Successful\n");
		status = Success;
	}
	else {
		printk("Manifest Verification Failure \n");
		status = Failure;
	}
	return status;
}

int cerberus_read_public_key(struct rsa_public_key *public_key)
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


int cerberus_verify_signature(struct signature_verification *verification,
			 const uint8_t *digest, size_t length, const uint8_t *signature, size_t sig_length)
{
	struct rsa_public_key rsa_public;
	cerberus_read_public_key(&rsa_public);
	struct rsa_engine *rsa = getRsaEngineInstance();
	return rsa->sig_verify(&rsa, &rsa_public, signature, sig_length, digest, length);
}

int cerberus_verify_regions(struct manifest *manifest)
{
	int status = 0;
	int get_key_status = 0;
	struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) manifest;
	uint8_t platfprm_id_length, fw_id_length;
	uint32_t read_address = pfr_manifest->address;
	uint8_t fw_element_header[4], fw_list_header[4];
	uint8_t sign_image_count, rw_image_count, fw_version_length;
	uint8_t signature[HASH_STORAGE_LENGTH];
	
	struct rsa_public_key pub_key;
	
	uint32_t start_address;
	uint32_t end_address;
	uint8_t *hashStorage = getNewHashStorage();
	struct CERBERUS_PFM_RW_REGION rw_region_data;
	struct CERBERUS_SIGN_IMAGE_HEADER sign_region_header;

	status = pfr_spi_read(pfr_manifest->flash_id, read_address + CERBERUS_PLATFORM_HEADER_OFFSET, sizeof(platfprm_id_length), &platfprm_id_length);

	read_address +=  CERBERUS_PLATFORM_HEADER_OFFSET + PLATFORM_ID_HEADER_LENGTH + platfprm_id_length + 2; //2 byte alignment
	read_address += CERBERUS_FLASH_DEVICE_OFFSET_LENGTH;

	status = pfr_spi_read(pfr_manifest->flash_id, read_address, sizeof(fw_element_header), fw_element_header);
	fw_id_length = fw_element_header[1];	
	read_address += sizeof(fw_element_header) + fw_id_length + 1; // 1 byte alignment

	//fw_list
	status = pfr_spi_read(pfr_manifest->flash_id, read_address, sizeof(fw_list_header), fw_list_header);
	sign_image_count = fw_list_header[0];
	rw_image_count = fw_list_header[1];
	fw_version_length = fw_list_header[2];

	read_address += sizeof(fw_list_header) + CERRBERUS_FW_VERSION_ADDR_LENGTH + fw_version_length + 2; // 2 byte alignment
	read_address += rw_image_count * sizeof(rw_region_data);

	struct Keystore_Manager keystore_manager;

	keystoreManager_init(&keystore_manager);

	for (int sig_index = 0; sig_index < sign_image_count ; sig_index++) {
		read_address += sizeof(sign_region_header);
		pfr_spi_read(pfr_manifest->flash_id, read_address, sizeof(signature), signature);
		read_address += sizeof(signature);

		pfr_spi_read(pfr_manifest->flash_id, read_address, sizeof(pub_key)-1, &pub_key);    
		read_address += sizeof(pub_key)-1;


		pfr_spi_read(pfr_manifest->flash_id, read_address,sizeof(start_address), &start_address);
		read_address += sizeof(start_address);
		printk("start_address:%x \r\n",start_address);

		pfr_spi_read(pfr_manifest->flash_id, read_address,sizeof(end_address), &end_address);
		read_address += sizeof(end_address);
		printk("end_address:%x \r\n",end_address);

		pfr_manifest->flash->device_id[0] = pfr_manifest->flash_id;	  // device_id will be changed by save_key function
		status = flash_verify_contents((struct flash *)pfr_manifest->flash,
					       start_address,
					       end_address - start_address + sizeof(uint8_t),
					       get_hash_engine_instance(),
					       HASH_TYPE_SHA256,
					       getRsaEngineInstance(),
					       signature,
					       256,
					       &pub_key,
					       hashStorage,
					       256
					       );
		if (status == Success) {
			int get_key_id = 0xFF;
			int last_key_id = 0xFF;
	
			get_key_status = keystore_get_key_id(&keystore_manager.base, &pub_key.modulus, &get_key_id, &last_key_id);
			printk("get_key_id is %x \n",get_key_id);
			if (get_key_status == KEYSTORE_NO_KEY) {
				get_key_status = keystore_manager.base.save_key(&keystore_manager.base, sig_index , &pub_key.modulus, pub_key.mod_length);
			} else {				
				// if key exist and be cancelled. return false.				
				status = pfr_manifest->keystore->kc_flag->verify_kc_flag(pfr_manifest, get_key_id);				
			}
		}
		

		if (status != Success) {
			printk("cerberus_verify_image %d Verification Fail\n", sig_index);
			printk("  region start_address:%x \r\n",start_address);
			printk("  region end_address:%x \r\n",end_address);
			return Failure;
		} else {
			printk("cerberus_verify_image %d Verification Successful\n", sig_index);
		}
		
	}	

	return status;
}

void cerberus_init_pfr_authentication(struct pfr_authentication *pfr_authentication)
{	
	pfr_authentication->verify_pfm_signature = cerberus_verify_signature;
	pfr_authentication->verify_regions = cerberus_verify_regions;
}
#endif
