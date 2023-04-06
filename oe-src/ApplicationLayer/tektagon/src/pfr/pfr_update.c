//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <stdint.h>
#include "pfr_ufm.h"
#include "state_machine/common_smc.h"
#include "include/SmbusMailBoxCom.h"
#include <drivers/misc/aspeed/pfr_aspeed.h>
#include <StateMachineAction/StateMachineActions.h>
#include "pfr_common.h"
#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_definitions.h"
#include "intel_2.0/intel_pfr_provision.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_definitions.h"
#include "cerberus/cerberus_pfr_provision.h"
#endif

#undef DEBUG_PRINTF
#if PF_UPDATE_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int handle_update_image_action(int image_type, void* EventContext)
{
	CPLD_STATUS cpld_update_status;
	int status;
	
	status = ufm_read(UPDATE_STATUS_UFM, UPDATE_STATUS_ADDRESS, (uint8_t *)&cpld_update_status, sizeof(CPLD_STATUS));
	if (status != Success)
		return status;

	BMCBootHold();
	PCHBootHold();
	 

#if SMBUS_MAILBOX_SUPPORT
		SetPlatformState(image_type == BMC_TYPE? BMC_FW_UPDATE : (PCH_TYPE ? PCH_FW_UPDATE : CPLD_FW_UPDATE));
		if (image_type != CPLD_FW_UPDATE) {
			SetLastPanicReason(lastPanicReason(image_type));
			IncPanicEventCount();
		}
#endif
	
    status = update_firmware_image(image_type, EventContext);
    if(status != Success)
        return Failure;

    return Success;
}
