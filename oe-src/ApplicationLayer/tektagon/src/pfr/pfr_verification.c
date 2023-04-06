//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//


#include "StateMachineAction/StateMachineActions.h"
#include "state_machine/common_smc.h"
#include "flash/flash_aspeed.h"
#include "Smbus_mailbox/Smbus_mailbox.h"
#include "pfr/pfr_common.h"

#include "pfr_util.h"

#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_authentication.h"
#include "intel_2.0/intel_pfr_definitions.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_authentication.h"
#include "cerberus/cerberus_pfr_definitions.h"
#endif


#undef DEBUG_PRINTF
#if PFR_AUTHENTICATION_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int authentication_image(void *AoData, void *EventContext){
    
    int status = 0;
    AO_DATA *ActiveObjectData = (AO_DATA *) AoData;
	EVENT_CONTEXT *EventData = (EVENT_CONTEXT *) EventContext;

    // init_pfr_manifest();
	struct pfr_manifest *pfr_manifest = get_pfr_manifest();
	
    pfr_manifest->state = VERIFY;

    if(EventData->image == BMC_EVENT){
        //BMC SPI
        DEBUG_PRINTF("Image Type: BMC \r\n");
        pfr_manifest->image_type = BMC_TYPE;

    }else{
        //PCH SPI
        DEBUG_PRINTF("Image Type: PCH \r\n");
        pfr_manifest->image_type = PCH_TYPE;
    }
    
    if(EventData->operation == VERIFY_BACKUP){  
        status = pfr_manifest->recovery_base->verify(pfr_manifest, pfr_manifest->hash, pfr_manifest->verification->base, pfr_manifest->pfr_hash->hash_out, pfr_manifest->pfr_hash->length, pfr_manifest->recovery_pfm);
    }else if(EventData->operation == VERIFY_ACTIVE){
        status = pfr_manifest->active_image_base->verify(pfr_manifest);
    }
    
    return status;
}
