#if CONFIG_CERBERUS_PFR_SUPPORT
//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include "pfr/pfr_common.h"
#include "Smbus_mailbox/Smbus_mailbox.h"
#include <include/definitions.h>
#include "cerberus_pfr_definitions.h"
#include "cerberus_pfr_verification.h"
#include "cerberus_pfr_provision.h"
#include "cerberus_pfr_key_cancellation.h"

#if PF_STATUS_DEBUG
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF printk
#endif
#else
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(...)
#endif
#endif

int cerberus_auth_pfr_recovery_verify(struct pfr_manifest *manifest)
{
	return Success;
}

int cerberus_auth_pfr_active_verify(struct pfr_manifest *manifest)
{
	printk("Active Region Verify ... \n");
	int status = 0;
	//manifest->address = PFM_FLASH_MANIFEST_ADDRESS;
    if (manifest->image_type == BMC_TYPE){
        get_provision_data_in_flash(BMC_ACTIVE_PFM_OFFSET, (uint8_t *)&manifest->address, sizeof(manifest->address));
        manifest->flash_id = BMC_FLASH_ID;
    }else{
        get_provision_data_in_flash(PCH_ACTIVE_PFM_OFFSET, (uint8_t *)&manifest->address, sizeof(manifest->address));
        manifest->flash_id = PCH_FLASH_ID;
    }

	status = manifest->base->verify(manifest,manifest->hash,manifest->verification->base, manifest->pfr_hash->hash_out, manifest->pfr_hash->length);

	if(status != Success){
        DEBUG_PRINTF("Verify active pfm failed\r\n");
        SetMajorErrorCode(manifest->image_type == BMC_TYPE ? BMC_AUTH_FAIL : PCH_AUTH_FAIL);
        return Failure;
    }
	status = cerberus_verify_regions(manifest);
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

#endif //CONFIG_CERBERUS_PFR_SUPPORT
