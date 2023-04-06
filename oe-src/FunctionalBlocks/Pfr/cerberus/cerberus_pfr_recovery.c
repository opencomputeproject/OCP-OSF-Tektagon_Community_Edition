//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#if CONFIG_CERBERUS_PFR_SUPPORT
#include "pfr/pfr_common.h"
#include "state_machine/common_smc.h"
#include "cerberus_pfr_recovery.h"
#include "manifest/pfm/pfm_manager.h"
#include "cerberus_pfr_authentication.h"
#include "cerberus_pfr_definitions.h"
#include "cerberus_pfr_provision.h"
#include "cerberus_pfr_verification.h"
#include "CommonFlash/CommonFlash.h"
#include "flash/flash_util.h"
#include "flash/flash_aspeed.h"
#include "keystore/KeystoreManager.h"

#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int verify_recovery_header_magic_number( struct recovery_header rec_head)
{
	int status = Success;
	if (rec_head.format == KEY_CANCELLATION_TYPE || rec_head.format == DECOMMISSION_TYPE) {
		if (rec_head.magic_number != CANCELLATION_HEADER_MAGIC) {
			status = Failure;
		}
	} else {
		if(rec_head.magic_number != RECOVERY_HEADER_MAGIC)
			status = Failure;
	}
	return status;

}

int get_signature(uint8_t flash_id, uint32_t address, uint8_t *signature, size_t signature_length)
{
	int status = Success;
	status = pfr_spi_read(flash_id, address, signature_length, signature);

	return status;
}

int cerberus_pfr_recovery_verify(struct recovery_image *image, struct hash_engine *hash,
			      struct signature_verification *verification, uint8_t *hash_out, size_t hash_length,
			      struct pfm_manager *pfm)
{
	int status = Success;
	struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) image;
	struct recovery_header recovery_header;
	struct rsa_public_key public_key;
	uint32_t signature_address;
	uint8_t sig_data[SHA256_SIGNATURE_LENGTH];
	uint8_t *hashStorage = getNewHashStorage();
	uint32_t recovery_offset;

	status = get_provision_data_in_flash(BMC_RECOVERY_REGION_OFFSET, (uint8_t *)&recovery_offset, sizeof(recovery_offset));
	if (status != Success){
		DEBUG_PRINTF("GET Recovery Offset Fail.\r\n");
		return Failure;
	}
	pfr_spi_read(BMC_RECOVERY_FLASH_ID, recovery_offset, sizeof(recovery_header), &recovery_header);

	status = verify_recovery_header_magic_number(recovery_header);
	if (status != Success){
		DEBUG_PRINTF("Recovery Header Magic Number is not Matched.\r\n");
		return Failure;
	}
	// get public key and init signature
	status = get_rsa_public_key(ROT_INTERNAL_INTEL_STATE, CERBERUS_ROOT_KEY_ADDRESS, &public_key);
	if (status != Success){
		DEBUG_PRINTF("Unable to get public Key.\r\n");
		return Failure;
	}

	//getSignature
	signature_address = recovery_header.image_length - recovery_header.sign_length;
	status = get_signature(BMC_RECOVERY_FLASH_ID, signature_address, sig_data, SHA256_SIGNATURE_LENGTH);
	if (status != Success){
		DEBUG_PRINTF("Unable to get the Signature.\r\n");
		return Failure;
	}

	//verify
	pfr_manifest->flash->device_id[0] = BMC_RECOVERY_FLASH_ID;

	status = flash_verify_contents( (struct flash *)pfr_manifest->flash,
									recovery_offset,
									(recovery_header.image_length - recovery_header.sign_length),
									get_hash_engine_instance(),
									1,
									getRsaEngineInstance(),
									sig_data,
									SHA256_SIGNATURE_LENGTH,
									&public_key,
									hashStorage,
									SHA256_SIGNATURE_LENGTH
									);
	if (status != Success){
		DEBUG_PRINTF("Recovery Data verify Fail.\r\n");
		return Failure;
	}
	DEBUG_PRINTF("Recovery Region Verify Success.\r\n");

	if (status == Success) {
		if (pfr_manifest->image_type == BMC_TYPE) {
			DEBUG_PRINTF("Authticate BMC ");
		} else {
			DEBUG_PRINTF("Authticate PCH ");
		}
		DEBUG_PRINTF("Recovery Region verification success\r\n");
	}

	return status;
}

int pfr_active_recovery_svn_validation(struct pfr_manifest *manifest)
{
	return Success;
}

int pfr_recover_recovery_region(int image_type, uint32_t source_address, uint32_t target_address)
{
	uint8_t status = Success;
	uint8_t source_flash_id, target_flash_id;
	struct recovery_header recovery_header;
	uint32_t data_offset = 0;
	uint32_t erase_address;

	if(image_type == BMC_TYPE){
		source_flash_id = BMC_FLASH_ID;
		target_flash_id = BMC_RECOVERY_FLASH_ID;
	}else if(image_type == PCH_TYPE){
		source_flash_id = PCH_FLASH_ID;
		target_flash_id = PCH_RECOVERY_FLASH_ID;
	}else if(image_type == HROT_TYPE){
		source_flash_id = ROT_INTERNAL_ACTIVE;
		target_flash_id = ROT_INTERNAL_RECOVERY;
	}else{
		return Failure;
	}

	erase_address = target_address;

	status = pfr_spi_read(source_flash_id, source_address, sizeof(recovery_header), &recovery_header);

	//will remove after provisioning done
	uint32_t total_image_length = recovery_header.image_length + 0x100 + 0x06;
	
	for (int i = 0; i < (total_image_length / PAGE_SIZE); i++) {

		status = pfr_spi_erase_4k(target_flash_id, erase_address);
		if(status != Success){
			return Failure;
		}

		erase_address += PAGE_SIZE;
	}

	for(int i = 0; i < (total_image_length / PAGE_SIZE); i++){
		status = pfr_spi_page_read_write_between_spi(source_flash_id, &source_address, target_flash_id, &target_address);
		if (status != Success)
			return Failure;
	}

	return Success;
}

int pfr_recover_active_region(struct pfr_manifest *manifest)
{
	struct recovery_header recovery_header;
	int status = Success;
	uint32_t sig_address, recovery_offset, data_offset, start_address = 0, erase_address = 0, section_length;
	struct recovery_section recovery_section;
	uint8_t platform_length;

	uint8_t source_flash_id, target_flash_id;
	uint32_t source_address, target_address; 

	if(manifest->image_type == BMC_TYPE){
		source_flash_id = BMC_RECOVERY_FLASH_ID;
		target_flash_id = BMC_FLASH_ID;
		get_provision_data_in_flash(BMC_RECOVERY_REGION_OFFSET, (uint8_t *)&source_address, sizeof(source_address));
	}else if(manifest->image_type == PCH_TYPE){
		source_flash_id = PCH_RECOVERY_FLASH_ID;
		target_flash_id = PCH_FLASH_ID;
		get_provision_data_in_flash(PCH_RECOVERY_REGION_OFFSET, (uint8_t *)&source_address, sizeof(source_address));
	}else{
		return Failure;
	}

	//read recovery header
	status = pfr_spi_read(source_flash_id, source_address, sizeof(recovery_header), &recovery_header);
	sig_address = recovery_header.image_length - recovery_header.sign_length;
	recovery_offset = source_address + sizeof(recovery_header);
	status = pfr_spi_read(source_flash_id, recovery_offset, sizeof(platform_length), &platform_length);
	recovery_offset = recovery_offset + platform_length + 1;

	while(recovery_offset != sig_address)
	{
		status = pfr_spi_read(source_flash_id, recovery_offset, sizeof(recovery_section), &recovery_section);
		if(recovery_section.magic_number != RECOVERY_SECTION_MAGIC)
		{
			DEBUG_PRINTF("Recovery Section not matched..\n");
			break;
		}
		start_address = recovery_section.start_addr;
		section_length = recovery_section.section_length;
		erase_address = start_address;
		recovery_offset = recovery_offset + sizeof(recovery_section);
		data_offset = recovery_offset;
		
		//erase active region data BMC_FLASH_ID
		for (int i = 0; i < (section_length / PAGE_SIZE); i++) {

			status = pfr_spi_erase_4k(target_flash_id, erase_address);
			if(status != Success){
				return Failure;
			}

			erase_address += PAGE_SIZE;
		}

		//copy data from BMC_RECOVERY_FLASH_ID to BMC_FLASH_ID
		for(int i = 0; i < (section_length / PAGE_SIZE); i++){
			status = pfr_spi_page_read_write_between_spi(source_flash_id, &data_offset, target_flash_id, &start_address);
			if (status != Success)
				return Failure;
		}
		recovery_offset = recovery_offset + recovery_section.section_length;
	}

	DEBUG_PRINTF("Repair success\r\n");

	return Success;
}

int active_region_pfm_update(struct pfr_manifest *manifest)
{
	return Success;
}

int pfr_staging_pch_staging(struct pfr_manifest *manifest)
{
	return Success;
}

int pfr_recover_update_action(struct pfr_manifest *manifest)
{
	return Success;
}
#endif
