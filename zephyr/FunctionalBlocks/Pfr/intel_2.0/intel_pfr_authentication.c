//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//
#if CONFIG_INTEL_PFR_SUPPORT
#include "pfr/pfr_common.h"
#include "intel_pfr_definitions.h"
#include "intel_pfr_verification.h"
#include "Smbus_mailbox/Smbus_mailbox.h"
#include "intel_pfr_provision.h"

#if PF_STATUS_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif


int pfr_recovery_verify(struct pfr_manifest *manifest){
    int status = 0;
    uint32_t read_address;
    DEBUG_PRINTF("Verify recovery\r\n");

    // Recovery region verification
    if(manifest->image_type == BMC_TYPE){
        ufm_read(PROVISION_UFM, BMC_RECOVERY_REGION_OFFSET, &read_address, sizeof(read_address));
        manifest->pc_type = PFR_BMC_UPDATE_CAPSULE;
    }else if(manifest->image_type == PCH_TYPE){
        ufm_read(PROVISION_UFM,PCH_RECOVERY_REGION_OFFSET, &read_address, sizeof(read_address));
        manifest->pc_type = PFR_PCH_UPDATE_CAPSULE;
    }
    manifest->address = read_address;

    //Block0-Block1 verifcation
    status = manifest->base->verify(manifest, manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        DEBUG_PRINTF("Verify recovery failed\r\n");
        return Failure;
    }
        
    if(manifest->image_type == BMC_TYPE){
        manifest->pc_type = PFR_BMC_PFM;
    }else if(manifest->image_type == PCH_TYPE){
        manifest->pc_type = PFR_PCH_PFM;
    }

    // Recovery region PFM verification
    manifest->address += PFM_SIG_BLOCK_SIZE;

    //manifest verifcation
    status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        DEBUG_PRINTF("Verify recovery pfm failed\r\n");
        return Failure;
    }
    
    status = get_recover_pfm_version_details(manifest, read_address);
    if(status != Success)
        return Failure;

    if (manifest->image_type == BMC_TYPE) {
        DEBUG_PRINTF("Authticate BMC ");
    } else {
        DEBUG_PRINTF("Authticate PCH ");
    }
    DEBUG_PRINTF("Recovery Region verification success\r\n");

    return Success;
}
                
int pfr_active_verify(struct pfr_manifest *manifest)
{
    int status = 0;
    uint32_t read_address;
    
     if(manifest->image_type == BMC_TYPE){
        get_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, &read_address, sizeof(read_address));
        manifest->pc_type = PFR_BMC_PFM;
    }else if(manifest->image_type == PCH_TYPE){
        get_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, &read_address, sizeof(read_address));
        manifest->pc_type = PFR_PCH_PFM;
    }		

    manifest->address = read_address;

    DEBUG_PRINTF("PFM Verification\r\n");

    status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);
    if(status != Success){
        DEBUG_PRINTF("Verify active pfm failed\r\n");
        SetMajorErrorCode(manifest->image_type == BMC_TYPE ? BMC_AUTH_FAIL : PCH_AUTH_FAIL);
        return Failure;
    }

    read_address = read_address + PFM_SIG_BLOCK_SIZE;
    status = pfm_version_set(manifest, read_address);
    if(status != Success){
        return Failure;
    }

    status = pfm_spi_region_verification(manifest);
    if(status != Success){
        SetMajorErrorCode(manifest->image_type == BMC_TYPE ? BMC_AUTH_FAIL : PCH_AUTH_FAIL);
        DEBUG_PRINTF("Verify active spi failed\r\n");
        return Failure;
    }

    if (manifest->image_type == BMC_TYPE) {
        DEBUG_PRINTF("Authticate BMC ");
    } else {
        DEBUG_PRINTF("Authticate PCH ");
    }
    DEBUG_PRINTF("Active Region verification success\r\n");
	return Success;
}

#endif
       
