//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//
#if CONFIG_INTEL_PFR_SUPPORT
#include "pfr/pfr_update.h"
#include "StateMachineAction/StateMachineActions.h"
#include "state_machine/common_smc.h" 
#include "pfr/pfr_common.h"
#include "intel_pfr_definitions.h"
#include "include/SmbusMailBoxCom.h"
#include "intel_pfr_verification.h"
#include "intel_pfr_provision.h" 
#include "intel_pfr_definitions.h"
#include "StateMachineAction/StateMachineActions.h"
#include "intel_pfr_pfm_manifest.h"
#include "flash/flash_aspeed.h"

#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

extern EVENT_CONTEXT DataContext;

int pfr_staging_verify(struct pfr_manifest *manifest){
    
    int status = 0;
    uint32_t read_address = 0;
    uint32_t target_address = 0;

    if (manifest->image_type == BMC_TYPE){
        DEBUG_PRINTF("BMC Recovery\r\n");
        status = ufm_read(PROVISION_UFM,BMC_STAGING_REGION_OFFSET,(uint8_t*)&read_address,sizeof(read_address));
        if (status != Success)
            return status;
        
        status = ufm_read(PROVISION_UFM,BMC_RECOVERY_REGION_OFFSET,(uint8_t*)&target_address,sizeof(target_address));
        if (status != Success)
            return status;
        
        manifest->pc_type = PFR_BMC_UPDATE_CAPSULE; 

    }
    else if (manifest->image_type == PCH_TYPE){
        DEBUG_PRINTF("PCH Recovery...\r\n");
        status = ufm_read(PROVISION_UFM,PCH_STAGING_REGION_OFFSET,(uint8_t*)&read_address,sizeof(read_address));
        if (status != Success)
            return Failure;

        status = ufm_read(PROVISION_UFM,PCH_RECOVERY_REGION_OFFSET,(uint8_t*)&target_address,sizeof(target_address));
        if (status != Success)
            return Failure;  

        manifest->pc_type = PFR_PCH_UPDATE_CAPSULE;  
    }else{
        return Failure;
    }

    manifest->address = read_address;
    manifest->recovery_address = target_address;   

    //manifest verification
    status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        DEBUG_PRINTF("staging verify failed\r\n");
        return Failure;
    }

    manifest->update_fw->pc_length = manifest->pc_length;

    if(manifest->image_type == BMC_TYPE){
        manifest->pc_type = PFR_BMC_PFM;
    }else if(manifest->image_type == PCH_TYPE){
        manifest->pc_type = PFR_PCH_PFM;
    }

    // Recovery region PFM verification
    manifest->address += PFM_SIG_BLOCK_SIZE;
	
	//manifest verification
    status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        DEBUG_PRINTF("staging verify failed\r\n");
        return Failure;
    }
    
	manifest->update_fw->pfm_length = manifest->pc_length;
	manifest->address = read_address;
    
    DEBUG_PRINTF("Staging area verification successful\r\n");

    return Success;
}

int intel_pfr_update_verify (struct firmware_image *fw, struct hash_engine *hash, struct rsa_engine *rsa){
    
	ARG_UNUSED(hash);
	ARG_UNUSED(rsa);

    int status = 0;
    struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) fw;

    return pfr_staging_verify(pfr_manifest);
}

int set_ufm_svn(struct pfr_manifest *manifest, uint8_t ufm_location, uint8_t svn_number){
	
	int status = 0;
	uint8_t svn_buffer[8];
    uint8_t offset = svn_number / 8;
    uint8_t remain = svn_number % 8;
    uint8_t index = 0;

    memset(svn_buffer,0xFF,sizeof(svn_buffer));
    for (index = 0; index < offset;index ++){
        svn_buffer[index] = 0x00;
    }

	svn_buffer[index] = svn_buffer[index] << remain;
	
    status = ufm_write (PROVISION_UFM,ufm_location,svn_buffer,sizeof(svn_buffer));
    if(status != Success)
		return Failure;

	return Success;
}

int get_ufm_svn(struct pfr_manifest *manifest, uint8_t offset){
	
	uint8_t svn_size = 8; // we have (0- 63) SVN Number in 64 bits
    uint8_t svn_buffer[8];
    uint8_t svn_number = 0 ,index1 = 0,index2 = 0;
    uint8_t mask = 0x01;
    
    ufm_read(PROVISION_UFM, offset, svn_buffer, sizeof(svn_buffer));
    for (index1 = 0; index1 < svn_size;index1 ++){
        for (index2 = 0; index2 < svn_size; index2 ++){
            if (/*!*/((svn_buffer[index1] >> index2) & mask)){
                return svn_number;
            }
            svn_number ++;
        }
    }

	return svn_number;
}

int  check_hrot_capsule_type(struct pfr_manifest *manifest)
{	
	int status = 0;
	uint32_t pc_type;

	status = pfr_spi_read(manifest->image_type, manifest->address +(2 * sizeof(pc_type)), sizeof(pc_type), (uint8_t *)&pc_type);
	if(pc_type == DECOMMISSION_CAPSULE){
		DEBUG_PRINTF("Decommission Certificate found\r\n");
		return DECOMMISSION_CAPSULE;
	}
	else if( (pc_type == CPLD_CAPSULE_CANCELLATION) || (pc_type == PCH_PFM_CANCELLATION) || (pc_type == PCH_CAPSULE_CANCELLATION)
    		|| (pc_type == BMC_PFM_CANCELLATION) || (pc_type == BMC_CAPSULE_CANCELLATION)){
		return KEY_CANCELLATION_CAPSULE;
	}
	else if (pc_type == PFR_CPLD_UPDATE_CAPSULE)
		return PFR_CPLD_UPDATE_CAPSULE;
	else if(pc_type == PFR_PCH_CPU_Seamless_Update_Capsule)
		return PFR_PCH_CPU_Seamless_Update_Capsule;
	else
		return 7;
}

int pfr_decommission(struct pfr_manifest *manifest)
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

	//DEBUG_PRINTF("Flash the CPLD Update Capsule to do UFM Provision\r\n");

	return Success;
}

int update_rot_fw(uint32_t address, uint32_t length){
	int status = 0;
	uint32_t source_address = address;
	uint32_t target_address = 0;

	uint32_t rot_recovery_address = 0;
	uint32_t rot_active_address= 0;
	uint32_t active_length = 0x60000;

	for(int i = 0; i < (active_length / PAGE_SIZE); i++){
		pfr_spi_erase_4k(ROT_INTERNAL_RECOVERY, rot_recovery_address);
		status = pfr_spi_page_read_write_between_spi(ROT_INTERNAL_ACTIVE, &rot_active_address, ROT_INTERNAL_RECOVERY, &rot_recovery_address);
		if(status != Success)
			return Failure;
	}

	for(int i = 0; i <= (length / PAGE_SIZE); i++){
		pfr_spi_erase_4k(ROT_INTERNAL_ACTIVE, target_address);
		status = pfr_spi_page_read_write_between_spi(BMC_SPI, &source_address, ROT_INTERNAL_ACTIVE, &target_address);
		if(status != Success)
			return Failure;
	}

	return Success;
}

int rot_svn_policy_verify(struct pfr_manifest *manifest, uint32_t hrot_svn)
{
	uint8_t current_svn;
	current_svn = get_ufm_svn(manifest, SVN_POLICY_FOR_CPLD_UPDATE);
	printk("\r\n *** current_svn:%d ***\r\n", current_svn);
	if (hrot_svn > SVN_MAX){
		DEBUG_PRINTF("Invalid Staging area SVN Number\r\n");
		return Failure;
	}else if (hrot_svn <= current_svn){
		DEBUG_PRINTF("Can't update with older version of SVN\r\n");
		return Failure;
	}else {
		set_ufm_svn(manifest, SVN_POLICY_FOR_CPLD_UPDATE, hrot_svn);
		SetCpldRotSvn((uint8_t)hrot_svn);
	}

	return Success;
}

int hrot_update(struct pfr_manifest *manifest)
{   
    int status = 0;
    uint8_t buffer;
    uint32_t pc_length = 0, pc_type, pc_type_status;
    uint32_t payload_address;

	// Checking the PC type
    status = pfr_spi_read(manifest->image_type, manifest->address + (2*sizeof(pc_type)), sizeof(pc_type), (uint8_t *)&pc_type);
    if(status != Success){
    	DEBUG_PRINTF("HROT update failed\r\n");
    	return Failure;
    }

    manifest->pc_type = pc_type;
	status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
	if(status != Success){
		DEBUG_PRINTF("HROT update capsule verification failed\r\n");
		SetMinorErrorCode(CPLD_UPD_CAPSULE_AUTH_FAIL);
		return Failure;
	}

	pc_type_status = check_hrot_capsule_type(manifest);
	payload_address = manifest->address + PFM_SIG_BLOCK_SIZE;

	if(pc_type_status == DECOMMISSION_CAPSULE){
		// Decommission validation
		manifest->address = payload_address;
		status = pfr_decommission(manifest);
		return status;
	} 
	else if(pc_type_status ==  KEY_CANCELLATION_CAPSULE){
		uint32_t cancelled_id = 0;

	    status = pfr_spi_read(manifest->image_type, payload_address, sizeof(uint32_t), (uint8_t *)&cancelled_id);
	    if(status != Success){
	    	DEBUG_PRINTF("HROT update failed\r\n");
	    	return Failure;
	    }
		
		status = manifest->keystore->kc_flag->cancel_kc_flag(manifest, cancelled_id);
		if(status == Success)
			DEBUG_PRINTF("Key cancellation success. Key Id :%d was cancelled\r\n",cancelled_id);
		
		return status;
	}
	else if (pc_type_status == PFR_CPLD_UPDATE_CAPSULE) {

		uint32_t hrot_svn = 0 ;
		status = pfr_spi_read(manifest->image_type , payload_address, sizeof(uint32_t), &hrot_svn);  
		if(status != Success)
			return Failure;

		status = rot_svn_policy_verify(manifest, hrot_svn);
		if (status != Success) {
			SetMinorErrorCode(CPLD_INVALID_SVN);
			return Failure;
		}

		pc_length = manifest->pc_length;
		pc_length = pc_length - (sizeof(uint32_t) + HROT_UPDATE_RESERVED);
		payload_address = payload_address + sizeof(uint32_t) + HROT_UPDATE_RESERVED;
		
		status = update_rot_fw(payload_address, pc_length);
		if(status != Success){
			DEBUG_PRINTF("Cpld update failed.\r\n");
			return Failure;
		}
	}

	return Success;
}

int check_svn_number(struct pfr_manifest *manifest, uint32_t read_address, uint8_t current_svn_number)
{	
	int status = 0;
	uint32_t pfm_start_address = read_address + PFM_SIG_BLOCK_SIZE + PFM_SIG_BLOCK_SIZE ;
	uint8_t buffer[sizeof(PFM_STRUCTURE_1)] = {0};
	uint8_t staging_svn_number = 0;
	printk("\r\n *** current_svn_number:%d ***\r\n", current_svn_number);	

	status = pfr_spi_read(manifest->image_type, pfm_start_address, sizeof(PFM_STRUCTURE_1), (uint8_t *)buffer);
	if(status != Success)
		return Failure;

	staging_svn_number = ((PFM_STRUCTURE_1 *)buffer)->SVN;

	if (staging_svn_number > SVN_MAX){
		DEBUG_PRINTF("Invalid Staging area SVN Number\r\n");
		return Failure;
	}else if (staging_svn_number < current_svn_number){
		DEBUG_PRINTF("Can't update with older version of SVN\r\n");
		return Failure;
	}else {

		if(manifest->image_type == PCH_TYPE) {
			status = set_ufm_svn(manifest, SVN_POLICY_FOR_PCH_FW_UPDATE, staging_svn_number);
		} else {
			status = set_ufm_svn(manifest, SVN_POLICY_FOR_BMC_FW_UPDATE, staging_svn_number);
		} 
		
		return status;
	}

	return Success;
}

int update_recovery_region(int image_type,uint32_t source_address,uint32_t target_address){
    return pfr_recover_recovery_region(image_type,source_address,target_address);
}

int update_firmware_image(uint32_t image_type, void* EventContext)
{   
    int status = 0;
    uint32_t source_address, target_address, pfm_length, area_size, pc_length;
    uint32_t address = 0;
    uint32_t pc_type_status = 0;
	uint8_t active_update_needed = 0;
    uint8_t active_svn_number = 0;
    CPLD_STATUS cpld_update_status;
    
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

	uint32_t flash_select = ((EVENT_CONTEXT*)EventContext)->flash;

	struct pfr_manifest *pfr_manifest = get_pfr_manifest();
    
	pfr_manifest->state = UPDATE;
	pfr_manifest->image_type = image_type;
	pfr_manifest->flash_id = flash_select;

    if(pfr_manifest->image_type == HROT_TYPE){
        pfr_manifest->image_type = BMC_TYPE;
        pfr_manifest->address = BMC_CPLD_STAGING_ADDRESS;
    	return hrot_update(pfr_manifest);
    }

    if (pfr_manifest->image_type == BMC_TYPE) {
    	DEBUG_PRINTF("BMC Update in Progress\r\n");
    	status = ufm_read(PROVISION_UFM, BMC_STAGING_REGION_OFFSET, (uint8_t *)&source_address,sizeof(source_address));
    	if (status != Success)
			return Failure;

	} else if (pfr_manifest->image_type == PCH_TYPE) {
		DEBUG_PRINTF("PCH Update in Progress\r\n");
		status = ufm_read(PROVISION_UFM, PCH_STAGING_REGION_OFFSET, (uint8_t *)&source_address,sizeof(source_address));
		if (status != Success)
			return status;
	}

    status = ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
    if (status != Success)
		return status;

    if(cpld_update_status.BmcToPchStatus == 1){
    	
		cpld_update_status.BmcToPchStatus = 0;
    	status = ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS,(uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
    	if (status != Success)
			return Failure;

    	status = ufm_read(PROVISION_UFM,BMC_STAGING_REGION_OFFSET,(uint8_t*)&address,sizeof(address));
		if (status != Success)
			return Failure;

		address += BMC_STAGING_SIZE;          // PFR Staging - PCH Staging offset is 32MB after BMC staging offset
		pfr_manifest->address = address;
    	
		//Checking for key cancellation
		pc_type_status = check_hrot_capsule_type(pfr_manifest);

    	status = pfr_staging_pch_staging(pfr_manifest);
    	if (status != Success)
			return Failure;
    }

	pfr_manifest->address = source_address;
    //Checking for key cancellation
	pc_type_status = check_hrot_capsule_type(pfr_manifest);
	if(pc_type_status ==  KEY_CANCELLATION_CAPSULE){
	    return hrot_update(pfr_manifest);
	}

	//Staging area verification
	DEBUG_PRINTF("Staging Area verification \r\n");
	// status = pfr_staging_verify(pfr_manifest);
	status = pfr_manifest->update_fw->base->verify(pfr_manifest, NULL, NULL);
	if(status != Success){
		DEBUG_PRINTF("Staging Area verification failed\r\n");
		SetMinorErrorCode(FW_UPD_CAPSULE_AUTH_FAIL);
		return Failure;
	}

	DEBUG_PRINTF("Staging Area verification success \r\n");

	// After staging manifest, Compression header will start
	area_size = pfr_manifest->update_fw->pc_length - (PFM_SIG_BLOCK_SIZE + pfr_manifest->update_fw->pfm_length);

	if(flash_select == PRIMARY_FLASH_REGION){
		status = pfr_manifest->recovery_base->verify(pfr_manifest, pfr_manifest->hash, pfr_manifest->verification->base, pfr_manifest->pfr_hash->hash_out, pfr_manifest->pfr_hash->length, pfr_manifest->recovery_pfm);
		if(status == Failure){
			DEBUG_PRINTF("Recovery Region Verify Fail, Update Active Region is not allowed. \r\n");
			return Failure;
		}
		pfr_manifest->address = source_address;
	}
	
	//SVN number validation
    if(pfr_manifest->image_type ==  BMC_TYPE){
		active_svn_number = get_ufm_svn(pfr_manifest, SVN_POLICY_FOR_BMC_FW_UPDATE);
    }
    else {
		active_svn_number = get_ufm_svn(pfr_manifest, SVN_POLICY_FOR_PCH_FW_UPDATE);
    }

	status = check_svn_number(pfr_manifest, source_address, active_svn_number);
	if(status != Success) {
		SetMinorErrorCode(PCH_BMC_FW_INVALID_SVN);
		DEBUG_PRINTF("Anti rollback\r\n");
		return Failure;
	}
	
	if(flash_select == PRIMARY_FLASH_REGION){
		//Active Update
		DEBUG_PRINTF("Active Region Update\r\n");

		status = capsule_decompression(pfr_manifest->image_type, source_address/* + PFM_SIG_BLOCK_SIZE + PFM_SIG_BLOCK_SIZE + PfmLength*/, area_size);
		if(status != Success)
			return Failure;

		status = active_region_pfm_update(pfr_manifest);
		if (status != Success) {
			DEBUG_PRINTF("Active Region PFM Update failed!!\r\n");
			return Failure;
		}
	}
	else{
			if (pfr_manifest->image_type == BMC_TYPE){
				DEBUG_PRINTF("BMC Recovery Region Update\r\n");
				status = ufm_read(PROVISION_UFM,BMC_RECOVERY_REGION_OFFSET, (uint8_t *)&target_address,sizeof(target_address));
			}else if (pfr_manifest->image_type == PCH_TYPE){
				DEBUG_PRINTF("PCH Recovery Region Update\r\n");
				status = ufm_read(PROVISION_UFM,PCH_RECOVERY_REGION_OFFSET, (uint8_t *)&target_address,sizeof(target_address));
			}

			if (status != Success){
				return status;
			}

			status = update_recovery_region(pfr_manifest->image_type,source_address,target_address);
			if (status != Success){
				DEBUG_PRINTF("Recovery capsule update failed\r\n");
				return Failure;
			}
    }

	return Success;
}

void get_region_type(CPLD_STATUS region){
	
	if(region.CpldStatus == 1) {
		if(region.Region[0].ActiveRegion == 1) {
				DataContext.flash = PRIMARY_FLASH_REGION;
		}
		if(region.Region[0].Recoveryregion == 1 && region.Region[0].ActiveRegion == 0) {
			DataContext.flash = SECONDARY_FLASH_REGION;
		}
	}
	if(region.BmcStatus == 1) {
		if(region.Region[1].ActiveRegion == 1 || (region.Region[1].Recoveryregion == 1 && region.Region[1].ActiveRegion == 1)) {
				DataContext.flash = PRIMARY_FLASH_REGION;
		}
		if(region.Region[1].Recoveryregion == 1 && region.Region[1].ActiveRegion == 0) {
			DataContext.flash = SECONDARY_FLASH_REGION;
		}
	}
	if(region.PchStatus == 1) {
		if((region.Region[2].ActiveRegion == 1) || (region.Region[2].Recoveryregion == 1 && region.Region[2].ActiveRegion == 1)) {
				DataContext.flash = PRIMARY_FLASH_REGION;
		}
		if(region.Region[2].Recoveryregion == 1 && region.Region[2].ActiveRegion == 0) {
			DataContext.flash = SECONDARY_FLASH_REGION;
		}
	}
}

void watchdog_timer(uint32_t image_type){
#if 0
	if(image_type == BMC_TYPE)
		printk("Watchdog timer BMC TYPE\r\n");
	else
		printk("Watchdog timer PCH TYPE\r\n");
#endif
}

int check_staging_area() {

	int status = 0;
    uint32_t pc_length, read_address, pc_type;
    uint8_t	active_svn_number = 0;
    int flash_id;
	void *AoData = NULL;
    
	DEBUG_PRINTF("Check Staging Area\r\n");
    CPLD_STATUS cpld_update_status;
	
	status = ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
    if (status != Success)
		return Failure;

    if(cpld_update_status.CpldStatus == 1) {
		DEBUG_PRINTF("CPLD Check\r\n");
    	get_region_type(cpld_update_status);
		DataContext.image = HROT_TYPE;
		flash_id = BMC_TYPE;
		
		// Checking CPLD staging area
		status = handle_update_image_action(HROT_TYPE, &DataContext);
		if(status == Success) {
			DEBUG_PRINTF("SuccessFully updated cpld\r\n");
			cpld_update_status.CpldStatus = 0;
			cpld_update_status.Region[0].ActiveRegion = 0;
			ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS,(uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
		}
    }

    if(cpld_update_status.BmcStatus == 1) {
    	DEBUG_PRINTF("BMC check\r\n");
		get_region_type(cpld_update_status);
		flash_id = BMC_TYPE;
		DataContext.image = flash_id;
		// Checking BMC staging area
		status = handle_update_image_action(flash_id, &DataContext);
		if(status == Success){
			if(cpld_update_status.Region[1].ActiveRegion == 1 && cpld_update_status.Region[1].Recoveryregion == 1){
				watchdog_timer(BMC_TYPE);
			}
			else if((cpld_update_status.Region[1].ActiveRegion == 1)
				||(cpld_update_status.Region[1].Recoveryregion == 1)){
				cpld_update_status.BmcStatus = 0;
				cpld_update_status.Region[1].ActiveRegion = 0;
				cpld_update_status.Region[1].Recoveryregion = 0;
				status = ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
			}
		} else {
			cpld_update_status.BmcStatus = 0;
			cpld_update_status.Region[1].ActiveRegion = 0;
			cpld_update_status.Region[1].Recoveryregion = 0;
			status = ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS,(uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
		}
    }

    if(cpld_update_status.PchStatus == 1) {
    	DEBUG_PRINTF("PCH check\r\n");
		// Checking PCH staging area
    	flash_id = PCH_TYPE;
		
		get_region_type(cpld_update_status);
		DataContext.image = flash_id;
		status = handle_update_image_action(flash_id, &DataContext);
		if(status == Success){
			if(cpld_update_status.Region[2].ActiveRegion == 1 && cpld_update_status.Region[2].Recoveryregion == 1){
				watchdog_timer(PCH_TYPE);
			}
			else if((cpld_update_status.Region[2].ActiveRegion == 1)||(cpld_update_status.Region[2].Recoveryregion == 1)){
				cpld_update_status.PchStatus = 0;
				cpld_update_status.Region[2].ActiveRegion = 0;
				cpld_update_status.Region[2].Recoveryregion = 0;
				status = ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
			}
		} else {
			cpld_update_status.PchStatus = 0;
			cpld_update_status.Region[2].ActiveRegion = 0;
			cpld_update_status.Region[2].Recoveryregion = 0;
			status = ufm_write(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS,(uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
		}
	}

   return status;
}
#endif
