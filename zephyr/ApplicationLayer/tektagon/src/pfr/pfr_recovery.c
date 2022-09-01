//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include "pfr_recovery.h"
#include "StateMachineAction/StateMachineActions.h"
#include "state_machine/common_smc.h" 
#include "pfr/pfr_common.h"
#include "include/SmbusMailBoxCom.h"
#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_definitions.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_definitions.h"
#endif
#undef DEBUG_PRINTF
#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int recover_image(void *AoData, void *EventContext){

    int status = 0;
    AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

    // init_pfr_manifest();
	struct pfr_manifest *pfr_manifest = get_pfr_manifest();
	
    pfr_manifest->state = RECOVERY;

    if(EventData->image == BMC_EVENT){
        //BMC SPI
        DEBUG_PRINTF("Image Type: BMC \r\n");
        pfr_manifest->image_type = BMC_TYPE;

    }else{
        //PCH SPI
        DEBUG_PRINTF("Image Type: PCH \r\n");
        pfr_manifest->image_type = PCH_TYPE;
    }
        
    if(ActiveObjectData->RecoveryImageStatus != Success){
        // status = pfr_staging_verify(pfr_manifest);
        status = pfr_manifest->update_fw->base->verify(pfr_manifest, NULL, NULL);
        if(status != Success){
            DEBUG_PRINTF("PFR Staging Area Corrupted\r\n");
            if (ActiveObjectData ->ActiveImageStatus != Success){
                SetMajorErrorCode(pfr_manifest->image_type == BMC_TYPE ? BMC_AUTH_FAIL : PCH_AUTH_FAIL);
                SetMinorErrorCode(ACTIVE_RECOVERY_STAGING_AUTH_FAIL);
                if (pfr_manifest->image_type == PCH_TYPE) {
                    status = pfr_staging_pch_staging(pfr_manifest);
                    if(status != Success)
                        return Failure;
                }else{
                    return Failure;
                }
            }else{
                return Failure;
            }
        }
        if(ActiveObjectData ->ActiveImageStatus == Success){
            status = pfr_active_recovery_svn_validation(pfr_manifest);
            if(status != Success)
                return Failure;               
        }

        status = pfr_recover_recovery_region(pfr_manifest->image_type, pfr_manifest->address, pfr_manifest->recovery_address);
        if(status != Success)
            return Failure;
        ActiveObjectData->RecoveryImageStatus = Success;
        return VerifyRecovery;
    }
    
    if(ActiveObjectData->ActiveImageStatus != Success){
        status = pfr_recover_active_region(pfr_manifest);
        if(status != Success)
            return Failure;
        ActiveObjectData->ActiveImageStatus = Success;
        return VerifyActive;
    }

    return Success;
}
