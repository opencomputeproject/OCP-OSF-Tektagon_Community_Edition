//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//
#if CONFIG_INTEL_PFR_SUPPORT
#include "pfr/pfr_common.h"
#include "state_machine/common_smc.h"
#include "intel_pfr_recovery.h"
#include "manifest/pfm/pfm_manager.h"
#include "intel_pfr_definitions.h"
#include "intel_pfr_provision.h"
#include "intel_pfr_verification.h"
#include "CommonFlash/CommonFlash.h"
#include "flash/flash_util.h"

#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int intel_pfr_recovery_verify (struct recovery_image *image, struct hash_engine *hash,
		struct signature_verification *verification, uint8_t *hash_out, size_t hash_length,
		struct pfm_manager *pfm){
    ARG_UNUSED(hash);
    ARG_UNUSED(verification);
    ARG_UNUSED(hash_out);
    ARG_UNUSED(hash_length);
    ARG_UNUSED(pfm);

    int status  = 0;        
    struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) image;

    return pfr_recovery_verify(pfr_manifest);
}

int pfr_active_recovery_svn_validation(struct pfr_manifest *manifest){
    
    int status = 0;
    uint8_t staging_svn, active_svn;
    status = read_statging_area_pfm(manifest, &staging_svn);
    if (status != Success){
        return Failure;
    }

    if (manifest->image_type == BMC_TYPE){
        active_svn = GetBmcPfmActiveSvn();
    }else{
        active_svn = GetPchPfmActiveSvn();
    }

    if(active_svn != staging_svn){
        DEBUG_PRINTF("SVN error\r\n");
        return Failure;
    }

    return Success;
}

int pfr_recover_recovery_region(int image_type,uint32_t source_address,uint32_t target_address)
{   
    int status = 0;
    uint32_t area_size = 0;
    struct SpiEngine *spi_flash = getSpiEngineWrapper();

    if(image_type == BMC_TYPE)
    	area_size = BMC_STAGING_SIZE;
    if(image_type == PCH_TYPE)
        area_size = PCH_STAGING_SIZE;

    spi_flash->spi.device_id[0] = image_type; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1
    DEBUG_PRINTF("Recovering...");

	status = flash_copy_and_verify(&spi_flash->spi, target_address, source_address, area_size);
	if(status != Success){
        DEBUG_PRINTF("Recovery region update failed\r\n");  
        return Failure;
    }
		
    DEBUG_PRINTF("Recovery region update completed\r\n");

    return Success;
}

int pfr_recover_active_region(struct pfr_manifest *manifest){

    int status  = 0;
    uint32_t pfm_length;
    uint32_t read_address = 0;
    uint32_t area_size;
    
    DEBUG_PRINTF("Active Data Corrupted\r\n");
    if(manifest->image_type == BMC_TYPE){
        status = ufm_read(PROVISION_UFM,BMC_RECOVERY_REGION_OFFSET,(uint8_t*)&read_address,sizeof(read_address));
        if (status != Success)
            return status;
        area_size = BMC_STAGING_SIZE;
    }else if (manifest->image_type == PCH_TYPE){
        status = ufm_read(PROVISION_UFM,PCH_RECOVERY_REGION_OFFSET,(uint8_t*)&read_address,sizeof(read_address));
        if (status != Success)
            return Failure;
        area_size = PCH_STAGING_SIZE;
    }else {
        return Failure;
    }

    status = capsule_decompression(manifest->image_type, read_address, area_size);
    if (status != Success){
        DEBUG_PRINTF("Repair Failed\r\n");
        return Failure;
    }
    
    status = active_region_pfm_update(manifest);
    if(status != Success){
        DEBUG_PRINTF("Active Region PFM Update failed!!\r\n");
        return Failure;
    }

    DEBUG_PRINTF("Repair success\r\n");
    
    return Success;
}

int active_region_pfm_update(struct pfr_manifest *manifest) {

	int status = 0;	
	uint32_t active_offset = 0, capsule_offset = 0;

	//Getting offsets based on ImageType
	if (manifest->image_type == PCH_TYPE) {
		status = ufm_read(PROVISION_UFM, PCH_ACTIVE_PFM_OFFSET, (uint8_t*) &active_offset, sizeof(active_offset));
		if (status != Success)
			return Failure;

		if(manifest->state == RECOVERY)
			status = ufm_read(PROVISION_UFM, PCH_RECOVERY_REGION_OFFSET, (uint8_t*) &capsule_offset, sizeof(capsule_offset));
		else
			status = ufm_read(PROVISION_UFM, PCH_STAGING_REGION_OFFSET, (uint8_t*) &capsule_offset, sizeof(capsule_offset));
		
		if (status != Success)
			return Failure;

	} else if (manifest->image_type == BMC_TYPE) {
		status = ufm_read(PROVISION_UFM, BMC_ACTIVE_PFM_OFFSET, (uint8_t*) &active_offset, sizeof(active_offset));
		if (status != Success)
			return status;

		if (manifest->state == RECOVERY)
			status = ufm_read(PROVISION_UFM, BMC_RECOVERY_REGION_OFFSET,(uint8_t*) &capsule_offset, sizeof(capsule_offset));
		else
			status = ufm_read(PROVISION_UFM, BMC_STAGING_REGION_OFFSET, (uint8_t*) &capsule_offset, sizeof(capsule_offset));
		
		if (status != Success)
			return status;
	}else{
        return Failure;
    }

	//Adjusting capsule offset size to PFM Signing chain
	capsule_offset += PFM_SIG_BLOCK_SIZE;
	
    struct SpiEngine *spi_flash = getSpiEngineWrapper();
    spi_flash->spi.device_id[0] = manifest->image_type; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1
    
    //Updating PFM from capsule to active region
	status = flash_copy_and_verify(&spi_flash->spi, active_offset, capsule_offset, PAGE_SIZE);
	if(status != Success){
        return Failure;
    }

	DEBUG_PRINTF("PFM Updated!!\r\n");

	return Success;
}

int pfr_staging_pch_staging(struct pfr_manifest *manifest){

    int status = 0;

    uint32_t source_address = 0; 			                    // BMC_CAPSULE_PCH_STAGING_ADDRESS;
	uint32_t target_address = 0;                                // PCH_CAPSULE_STAGING_ADDRESS;
	uint32_t area_size     = BMC_PCH_STAGING_SIZE;
	uint32_t PfmLength, PcLength;

	uint8_t  active_svn = 0;

	status = ufm_read(PROVISION_UFM,BMC_STAGING_REGION_OFFSET,(uint8_t*)&source_address,sizeof(source_address));
	if (status != Success)
		return Failure;

    status = ufm_read(PROVISION_UFM,PCH_STAGING_REGION_OFFSET,(uint8_t*)&target_address,sizeof(target_address));
	if (status != Success)
		return Failure;


	source_address += BMC_STAGING_SIZE;          // PFR Staging - PCH Staging offset is 32MB after BMC staging offset

    uint32_t image_type = manifest->image_type;
    manifest->image_type = BMC_TYPE;
    
    manifest->address = source_address;
    manifest->pc_type = PFR_PCH_UPDATE_CAPSULE;

    DEBUG_PRINTF("BMC(PCH) Staging Area verification\r\n");
    //manifest verifcation
    status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        DEBUG_PRINTF("verify failed\r\n");
        return Failure;
    }

    // Recovery region PFM verification
    manifest->address += PFM_SIG_BLOCK_SIZE;
    manifest->pc_type = PFR_PCH_PFM;
    //manifest verifcation
    status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        return Failure;
    }
    printk("BMC PCH Staging verification successful\r\n");
    manifest->address = target_address;
    manifest->image_type = image_type;

	uint32_t erase_address = target_address;

	for (int i = 0; i < (area_size / PAGE_SIZE); i++) {

		status = pfr_spi_erase_4k(manifest->image_type, erase_address);
		if(status != Success){
			return Failure;
		}

		erase_address += PAGE_SIZE;
	}

	for(int i = 0; i < (area_size / PAGE_SIZE); i++){
        status = pfr_spi_page_read_write_between_spi(BMC_TYPE, &source_address,PCH_TYPE, &target_address);
		if (status != Success)
			return Failure;
	}

	if (manifest->state == RECOVERY) {
        DEBUG_PRINTF("PCH staging region verification\r\n");
        status = manifest->update_fw->base->verify(manifest, NULL, NULL);
        if(status != Success)
            return Failure;
	}

	DEBUG_PRINTF("BMC PCH Recovery region Update completed\r\n");

	return Success;
}

int intel_pfr_recover_update_action(struct pfr_manifest *manifest){
    return Success;
}
#endif
