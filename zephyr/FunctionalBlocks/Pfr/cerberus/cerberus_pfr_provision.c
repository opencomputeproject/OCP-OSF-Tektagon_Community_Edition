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
#include "cerberus_pfr_definitions.h"
#include "pfr/pfr_util.h"
#include "cerberus_pfr_provision.h"
#include "cerberus_pfr_verification.h"
#include "include/SmbusMailBoxCom.h"
#include "flash/flash_aspeed.h"

#undef DEBUG_PRINTF
#if PFR_AUTHENTICATION_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

uint8_t cRootKeyHash[SHA384_DIGEST_LENGTH] = {0};
uint8_t cPchOffsets[12];
uint8_t cBmcOffsets[12];

int cerberus_g_provision_data;

int verify_rcerberus_magic_number( uint32_t magic_number)
{
	int status = Success;
	if(magic_number != RECOVERY_HEADER_MAGIC)
			status = Failure;
	return status;
}

int verify_cerberus_provisioning_type(uint16_t image_type){
	int status = Success;
	if(image_type != PROVISIONING_IMAGE_TYPE)
			status = Failure;
	return status;
}

unsigned char CerberusProvisionBmcOffsets(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));

	if (((UfmStatus & 1)) && ((UfmStatus & 8))) {

		Status = set_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, cBmcOffsets, sizeof(cBmcOffsets));
		if (Status == Success) {
			UfmStatus &= 0xF7;
			Status = set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));
			DEBUG_PRINTF("BMC Provisioned\r\n");

		} else {
			DEBUG_PRINTF("BMC Offsets Provision failed...\r\n");
			erase_provision_flash();
			set_provision_status(COMMAND_ERROR);
		}
		return Success;
	} else {
		return UnSupported;
	}
}

unsigned char CerberusProvisionPchOffsets(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(UfmStatus));

	if (((UfmStatus & 1)) && ((UfmStatus & 4))) {

		Status = set_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, cPchOffsets, sizeof(cPchOffsets));
		if (Status == Success) {
			DEBUG_PRINTF("Ps\r\n");
			UfmStatus &= 0xFB;
			Status = set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));
		} else {
			DEBUG_PRINTF("PCH Offsets Provision failed...\r\n");
			erase_provision_flash();
			set_provision_status(COMMAND_ERROR);
		}
		return Success;
	} else {
		return UnSupported;
	}
}

unsigned char CerberusProvisionRootKeyHash(void)
{
	uint8_t Status;
	uint32_t UfmStatus;

	get_provision_data_in_flash(UFM_STATUS, &UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));
	if (((UfmStatus & 1)) && ((UfmStatus & 2))) {
		Status = set_provision_data_in_flash(ROOT_KEY_HASH, cRootKeyHash, SHA256_DIGEST_LENGTH);
		if (Status == Success) {
			UfmStatus &= 0xFD;
			Status = set_provision_data_in_flash(UFM_STATUS, (uint8_t *)&UfmStatus, sizeof(uint32_t) / sizeof(uint8_t));
			return Success;
		} else {
			return Failure;
		}
	} else {
		printk("Unsupported error\n");
		return UnSupported;
	}
}

int getCerberusProvisionData(int offset, uint8_t *data, uint32_t length){
	int status = 0;
	status = pfr_spi_read(ROT_INTERNAL_INTEL_STATE, offset, length, data);
	return status;
}

int cerberus_provisioning_root_key_action(struct pfr_manifest *manifest){
	int status = Success;

	struct PROVISIONING_IMAGE_HEADER provision_header;
	struct PROVISIONING_MANIFEST_DATA provision_manifest;

	pfr_spi_read(manifest->flash_id, manifest->address, sizeof(provision_header), &provision_header);
	DEBUG_PRINTF("Verify Provisioning Type.\r\n");
	status = verify_cerberus_provisioning_type(provision_header.image_type);
	if (status != Success){
		DEBUG_PRINTF("Provisioning Type Error.\r\n");
		return Failure;
	}

	DEBUG_PRINTF("Verify Provisioning Magic Number.\r\n");
	status = verify_rcerberus_magic_number(provision_header.magic_num);
	if (status != Success){
		DEBUG_PRINTF("Magic Number is not Matched.\r\n");
		return Failure;
	}

	if(provision_header.provisioning_flag[0] == PROVISION_OTP_KEY_FLAG){
		//Provision OTP Key Content
	}

	if(provision_header.provisioning_flag[0] == PROVISION_ROOT_KEY_FLAG){
		//Provision root Key Content
		DEBUG_PRINTF("Provisioning ROOT Key.\r\n");
		uint16_t key_length = 0;
		
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_BMC_ACTIVE_OFFSET, 4, cBmcOffsets);
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_BMC_RECOVERY_OFFSET, 4, cBmcOffsets + 4);
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_BMC_STAGE_OFFSET, 4, cBmcOffsets + 8);
		CerberusProvisionBmcOffsets();

		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_PCH_ACTIVE_OFFSET, 4, cPchOffsets);
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_PCH_RECOVERY_OFFSET, 4, cPchOffsets + 4);
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_PCH_STAGE_OFFSET, 4, cPchOffsets + 8);
		CerberusProvisionPchOffsets();

		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_ROOT_KEY_LENGTH, sizeof(key_length), &key_length);

		uint8_t cerberus_root_key[key_length];
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_ROOT_KEY, key_length, cerberus_root_key);

		manifest->pfr_hash->start_address = manifest->address + CERBERUS_ROOT_KEY;
		manifest->pfr_hash->length = key_length;
		manifest->pfr_hash->type = HASH_TYPE_SHA256;
		manifest->base->get_hash(manifest,manifest->hash,cRootKeyHash, SHA256_DIGEST_LENGTH);
		CerberusProvisionRootKeyHash();
		//write root key to d0200

		unsigned int data_length = 0; 
		uint8_t exponent_length;

		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_ROOT_KEY + key_length, sizeof(exponent_length), &exponent_length);

		data_length = sizeof(key_length) + key_length + sizeof(exponent_length) + exponent_length;

		uint8_t key_whole_data[data_length];
		pfr_spi_read(manifest->flash_id, manifest->address + CERBERUS_ROOT_KEY_LENGTH, data_length, key_whole_data);
		pfr_spi_write(ROT_INTERNAL_INTEL_STATE, CERBERUS_ROOT_KEY_ADDRESS, data_length, key_whole_data);

		DEBUG_PRINTF("Provisioning Done.\r\n");

	}else{
		return Failure;
	}
	return status;
}

// Verify Root Key hash
int cerberus_verify_root_key_hash(struct pfr_manifest *manifest, uint8_t *root_public_key)
{
	return Success;
}

#endif //CONFIG_CERBERUS_PFR_SUPPORT
