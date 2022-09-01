//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//
#if CONFIG_CERBERUS_PFR_SUPPORT
#include "pfr/pfr_update.h"
#include "StateMachineAction/StateMachineActions.h"
#include "state_machine/common_smc.h"
#include "pfr/pfr_common.h"
#include "include/SmbusMailBoxCom.h"
#include "StateMachineAction/StateMachineActions.h"
#include "cerberus_pfr_definitions.h"
#include "cerberus_pfr_verification.h"
#include "cerberus_pfr_provision.h"
#include "cerberus_pfr_definitions.h"
#include "cerberus_pfr_pfm_manifest.h"
#include "cerberus_pfr_recovery.h"
#include "cerberus_pfr_common.h"
#include "flash/flash_aspeed.h"
#include "keystore/KeystoreManager.h"

#define DECOMMISSION_PC_SIZE		128

#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

extern EVENT_CONTEXT DataContext;

int cerberus_pfr_staging_verify(struct pfr_manifest *manifest)
{
	int status = Success;
	struct recovery_header image_header;
	struct rsa_public_key public_key;

	uint32_t signature_address;
	uint8_t sig_data[SHA256_SIGNATURE_LENGTH];
	uint8_t *hashStorage = getNewHashStorage();

	DEBUG_PRINTF("Stage Image Verification Start\r\n");

	pfr_spi_read(manifest->flash_id, manifest->address, sizeof(image_header), &image_header);

	status = verify_recovery_header_magic_number(image_header);
	if (status != Success){
		DEBUG_PRINTF("Image Header Magic Number is not Matched.\r\n");
		return Failure;
	}
	// get public key and init signature
	status = get_rsa_public_key(ROT_INTERNAL_INTEL_STATE, CERBERUS_ROOT_KEY_ADDRESS, &public_key);
	
	if (status != Success){
		DEBUG_PRINTF("Unable to get public Key.\r\n");
		return Failure;
	}

	//getSignature
	signature_address = manifest->address + image_header.image_length - image_header.sign_length;
	status = get_signature(manifest->flash_id, signature_address, sig_data, SHA256_SIGNATURE_LENGTH);
	if (status != Success){
		DEBUG_PRINTF("Unable to get the Signature.\r\n");
		return Failure;
	}

	//verify
	manifest->flash->device_id[0] = manifest->flash_id;

	status = flash_verify_contents( (struct flash *)manifest->flash,
									manifest->address,
									(image_header.image_length - image_header.sign_length),
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
		DEBUG_PRINTF("Image verify Fail.\r\n");
		return Failure;
	}
	DEBUG_PRINTF("Stage Image Verify Success.\r\n");

	return Success;
}

int cerberus_pfr_update_verify(struct firmware_image *fw, struct hash_engine *hash, struct rsa_engine *rsa)
{
	int status = 0;
	struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) fw;
	if(pfr_manifest->image_type == BMC_TYPE){
		get_provision_data_in_flash(BMC_STAGING_REGION_OFFSET, (uint8_t *)&pfr_manifest->address, sizeof(pfr_manifest->address));
		get_provision_data_in_flash(BMC_RECOVERY_REGION_OFFSET, (uint8_t *)&pfr_manifest->recovery_address, sizeof(pfr_manifest->recovery_address));
		pfr_manifest->flash_id = BMC_FLASH_ID;  
	}else if(pfr_manifest->image_type == PCH_TYPE){
		get_provision_data_in_flash(PCH_STAGING_REGION_OFFSET, (uint8_t *)&pfr_manifest->address, sizeof(pfr_manifest->address));
		get_provision_data_in_flash(PCH_RECOVERY_REGION_OFFSET, (uint8_t *)&pfr_manifest->recovery_address, sizeof(pfr_manifest->recovery_address));
		pfr_manifest->flash_id = PCH_FLASH_ID;
	}

	return cerberus_pfr_staging_verify(pfr_manifest);
}

int cerberus_set_ufm_svn(struct pfr_manifest *manifest, uint8_t ufm_location, uint8_t svn_number)
{
	return Success;
}

int get_ufm_svn(struct pfr_manifest *manifest, uint8_t offset){

	return 0;
}

int cerberus_pfr_decommission(struct pfr_manifest *manifest)
{
	int status = 0;
	uint8_t decom_buffer[DECOMMISSION_PC_SIZE] = {0};
	uint8_t read_buffer[DECOMMISSION_PC_SIZE] = {0};

	CPLD_STATUS cpld_update_status;

	status = pfr_spi_read(manifest->image_type, manifest->address, manifest->pc_length, read_buffer);
	if(status != Success ){
		DEBUG_PRINTF("PfrDecommission failed\r\n");
		return Failure;
	}

	status = compare_buffer(read_buffer, decom_buffer, sizeof(read_buffer));
	if(status != Success){
		DEBUG_PRINTF("Invalid decommission capsule data\r\n");
		return Failure;
	}

	// Erasing provisioned data
	DEBUG_PRINTF("Decommission Success.Erasing the provisioned UFM data\r\n");
	
	status = ufm_erase(PROVISION_UFM);
	if (status != Success)
		return Failure;
	
	memset(&cpld_update_status,0, sizeof(cpld_update_status));
	
	cpld_update_status.DecommissionFlag = 1;
	status = ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS,(uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
	if (status != Success)
		return Failure;	
	return Success;
}

int cerberus_update_HRoT_recovery_region(void)
{
	uint32_t active_length = HROT_ACTIVE_AREA_SIZE;
	uint32_t rot_recovery_address = 0;
	uint32_t rot_active_address= 0;
	int status = 0;	

	for(int i = 0; i < (active_length / PAGE_SIZE); i++){
		pfr_spi_erase_4k(ROT_INTERNAL_RECOVERY, rot_recovery_address);
		status = pfr_spi_page_read_write_between_spi(ROT_INTERNAL_ACTIVE, &rot_active_address, ROT_INTERNAL_RECOVERY, &rot_recovery_address);
		if(status != Success)
			return Failure;
	}
	return Success;
}

int cerberus_update_HRoT_active_region(struct pfr_manifest *manifest)
{
	uint32_t length = 0;
	uint32_t target_address = 0;
	uint32_t source_address = manifest->address;
	int status = 0;	
	struct recovery_header image_header;
	struct recovery_section image_section;	

	pfr_spi_read(manifest->flash_id, source_address, sizeof(image_header), &image_header);
	source_address = source_address + image_header.header_length;
	pfr_spi_read(manifest->flash_id, source_address, sizeof(image_section), &image_section);
	source_address = source_address + image_section.header_length;

	length = image_section.section_length;

	for(int i = 0; i <= (length / PAGE_SIZE); i++){
		pfr_spi_erase_4k(ROT_INTERNAL_ACTIVE, target_address);
		status = pfr_spi_page_read_write_between_spi(BMC_SPI, &source_address, ROT_INTERNAL_ACTIVE, &target_address);
		if(status != Success)
			return Failure;
	}	
	return Success;
}

int cerberus_update_rot_fw(struct pfr_manifest *manifest)
{
	uint32_t rot_recovery_address = 0;
	uint32_t rot_active_address= 0;
	int status = 0;	
	uint8_t target_flash_id = manifest->flash_id;

	status = cerberus_update_HRoT_recovery_region();
	if(status != Success){
		DEBUG_PRINTF("HRoT update recovery failed\r\n");
		return Failure;
	}	
	status = cerberus_update_HRoT_active_region(manifest);
	if(status != Success){
		DEBUG_PRINTF("HRoT update active failed\r\n");
		return Failure;
	}	
	return Success;
}

int cerberus_hrot_update(struct pfr_manifest *manifest)
{
	int status = 0;
	byte provision_state = get_provision_status();
	if (provision_state == UFM_PROVISIONED){
		struct recovery_header image_header;
		pfr_spi_read(manifest->flash_id, manifest->address, sizeof(image_header), &image_header);
		status = cerberus_pfr_staging_verify(manifest);
		if(status != Success){
			DEBUG_PRINTF("HRoT update pfr verification failed\r\n");
			return Failure;
		}

		if (image_header.format == UPDATE_FORMAT_TPYE_HROT) {
			status = cerberus_update_rot_fw(manifest);
			if(status != Success){
				DEBUG_PRINTF("HRoT update failed.\r\n");
				return Failure;
			}	
		}else if (image_header.format == DECOMMISSION_TYPE) {		
			status = cerberus_pfr_decommission(manifest);
		}
	}else{
		DEBUG_PRINTF("Start HROT Provisioning \r\n");
		return cerberus_provisioning_root_key_action(manifest);
	}
	return Success;
}


int cerberus_rot_svn_policy_verify(struct pfr_manifest *manifest, uint32_t hrot_svn)
{
	return Success;
}

int cerberus_keystore_update(struct pfr_manifest *manifest, uint16_t image_format)
{	
	int status = 0;
    uint8_t buffer;
    uint16_t header_length;
	uint16_t capsule_type;
	uint16_t section_header_length;
	uint32_t pc_type_status;    

	// Get the Header Length
    status = pfr_spi_read(manifest->image_type, manifest->address, sizeof(header_length), (uint16_t *)&header_length);
    if(status != Success){
    	DEBUG_PRINTF("HROT update failed\r\n");
    	return Failure;
    }
	
	status = pfr_spi_read(manifest->image_type, manifest->address + header_length, sizeof(section_header_length), (uint16_t *)&section_header_length);
    if(status != Success){
    	DEBUG_PRINTF("HROT update failed\r\n");
    	return Failure;
    }

	if (image_format == KEY_CANCELLATION_TYPE) {		
		uint8_t cancelled_key;
		int get_key_id = 0xFF;
		int last_key_id = 0xFF;
		uint8_t pub_key[256];
		struct Keystore_Manager keystore_manager;
		keystoreManager_init(&keystore_manager);

		status = pfr_spi_read(manifest->image_type, manifest->address + header_length + section_header_length - 2, sizeof(capsule_type), (uint16_t *)&capsule_type);		
		manifest->pc_type = capsule_type;
		printk("capsule_type is %x \n",capsule_type);
		status = pfr_spi_read(manifest->image_type, manifest->address + header_length + section_header_length, 256, (uint8_t *)&pub_key);
	    if(status != Success){
	    	DEBUG_PRINTF("HROT update failed\r\n");
	    	return Failure;
	    }
		status = keystore_get_key_id(&keystore_manager.base, &pub_key, &get_key_id, &last_key_id);		
		if (get_key_id != 0xFF) {
			printk("Key Id %x should be cancelled \n",get_key_id);
			status = manifest->keystore->kc_flag->cancel_kc_flag(manifest, get_key_id);
			if(status == Success)
				DEBUG_PRINTF("Key cancellation success. Key Id :%d was cancelled\r\n",get_key_id);
		} else {
			status = KEYSTORE_NO_KEY;
		}
	} else if (image_format == DECOMMISSION_TYPE) {		
		status = cerberus_pfr_decommission(manifest);
	} 
	return status;
}

int cerberus_check_svn_number(struct pfr_manifest *manifest, uint32_t read_address, uint8_t current_svn_number)
{
	return Success;
}

int cerberus_update_recovery_region(int image_type, uint32_t source_address, uint32_t target_address)
{
	return pfr_recover_recovery_region(image_type, source_address, target_address);
}

/******************************
|cerberus_update_active_region |
|******************************|
|Description: 
|This function is used for updating from stage to active region
|Require:
|pfr_manifest->address:  Stage Region Offset
|pfr_manifest->flash:  Stage Region Flash ID
|target_flash: Target Flash ID
*/

int cerberus_update_active_region(struct pfr_manifest *manifest, uint8_t target_flash){
	int status = Success;

	struct recovery_header image_header;
	struct recovery_section image_section;

	uint32_t sig_address, recovery_offset, data_offset, start_address = 0, erase_address = 0, section_length;
	uint32_t target_address; 
	uint8_t platform_length;

	//read recovery header
	status = pfr_spi_read(manifest->flash_id, manifest->address, sizeof(image_header), &image_header);	
	if (image_header.format == KEY_CANCELLATION_TYPE || image_header.format == DECOMMISSION_TYPE) {		
		status = cerberus_keystore_update(manifest, image_header.format);
	} else {		
		sig_address = manifest->address + image_header.image_length - image_header.sign_length;
		recovery_offset = manifest->address + sizeof(image_header);
		status = pfr_spi_read(manifest->flash_id, recovery_offset, sizeof(platform_length), &platform_length);
		recovery_offset = recovery_offset + platform_length + 1;

		while(recovery_offset != sig_address)
		{
			status = pfr_spi_read(manifest->flash_id, recovery_offset, sizeof(image_section), &image_section);
			if(image_section.magic_number != RECOVERY_SECTION_MAGIC)
			{
				status = Failure;
				DEBUG_PRINTF("Recovery Section not matched..\n");
				break;
			}
			start_address = image_section.start_addr;
			section_length = image_section.section_length;
			erase_address = start_address;
			recovery_offset = recovery_offset + sizeof(image_section);
			data_offset = recovery_offset;
			
			//erase active region data BMC_FLASH_ID
			for (int i = 0; i < (section_length / PAGE_SIZE); i++) {

				status = pfr_spi_erase_4k(target_flash, erase_address);
				if(status != Success){
					return Failure;
				}

				erase_address += PAGE_SIZE;
			}

			//copy data from BMC_RECOVERY_FLASH_ID to BMC_FLASH_ID
			for(int i = 0; i < (section_length / PAGE_SIZE); i++){
				status = pfr_spi_page_read_write_between_spi(manifest->flash_id, &data_offset,target_flash, &start_address);
				if (status != Success)
					return Failure;
			}
			recovery_offset = recovery_offset + image_section.section_length;
		}
	}

	return status;
}

/*
TODO:
After provisioning, need to change the way to get stage offset
*/
int update_firmware_image(uint32_t image_type, void *EventContext)
{
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	uint8_t status = Success;
	uint8_t target_flash_id = -1;
	uint32_t source_address = 0, target_address = 0;
	uint8_t flash_select = EventData->flash;
	struct pfr_manifest *pfr_manifest = get_pfr_manifest();
	pfr_manifest->image_type = image_type;

	DEBUG_PRINTF("Firmware Update Start.\r\n");

	if(image_type == BMC_TYPE){  //BMC Update/Provisioning
		DEBUG_PRINTF("Image Type: BMC.\r\n");
		get_provision_data_in_flash(BMC_STAGING_REGION_OFFSET, (uint8_t *)&pfr_manifest->address, sizeof(pfr_manifest->address));
		pfr_manifest->flash_id = BMC_FLASH_ID;
	}else if(image_type == PCH_TYPE){  //PCH Update
		DEBUG_PRINTF("Image Type: PCH.\r\n");
		pfr_manifest->flash_id = PCH_FLASH_ID;
		get_provision_data_in_flash(PCH_STAGING_REGION_OFFSET, (uint8_t *)&pfr_manifest->address, sizeof(pfr_manifest->address));
		
	}else if(image_type == HROT_TYPE){  //HROT Update/Decommisioning
		DEBUG_PRINTF("Image Type: HRoT \r\n");
		pfr_manifest->image_type = BMC_TYPE;
		pfr_manifest->address = BMC_CPLD_STAGING_ADDRESS;
		pfr_manifest->flash_id = BMC_FLASH_ID;
		return cerberus_hrot_update(pfr_manifest);
	}

	//BMC/PCH Firmware Update for Active/Recovery Region
	status = pfr_manifest->update_fw->base->verify(pfr_manifest, NULL, NULL);
	if(status != Success){
		DEBUG_PRINTF("Staging Area verification failed\r\n");
		SetMinorErrorCode(FW_UPD_CAPSULE_AUTH_FAIL);
		return Failure;
	}

	if(flash_select == PRIMARY_FLASH_REGION){ //Update Active
		DEBUG_PRINTF("Update Type: Active Update.\r\n");
		if(image_type == BMC_TYPE){  //BMC Update/Provisioning
			target_flash_id = BMC_FLASH_ID;
		}else if(image_type == PCH_TYPE){  //PCH Update
			target_flash_id = PCH_FLASH_ID;
		}
		
		status = cerberus_update_active_region(pfr_manifest, target_flash_id);
	}else{ //Update Recovery
		DEBUG_PRINTF("Update Type: Recovery Update.\r\n");
		if(image_type == BMC_TYPE){  //BMC Update/Provisioning
			get_provision_data_in_flash(BMC_STAGING_REGION_OFFSET, (uint8_t *)&source_address, sizeof(source_address));
			get_provision_data_in_flash(BMC_RECOVERY_REGION_OFFSET, (uint8_t *)&target_address, sizeof(target_address));
		}else if(image_type == PCH_TYPE){  //PCH Update
			get_provision_data_in_flash(PCH_STAGING_REGION_OFFSET, (uint8_t *)&source_address, sizeof(source_address));
			get_provision_data_in_flash(PCH_RECOVERY_REGION_OFFSET, (uint8_t *)&target_address, sizeof(target_address));
		}

		status = cerberus_update_recovery_region(image_type, source_address, target_address);
	}
	return status;
}

void cerberus_watchdog_timer(uint32_t image_type)
{
#if 0
	if (image_type == BMC_TYPE)
		printk("Watchdog timer BMC TYPE\r\n");
	else
		printk("Watchdog timer PCH TYPE\r\n");
#endif
}

int check_staging_area(void)
{

	int status = 0;

	return status;
}

#endif
