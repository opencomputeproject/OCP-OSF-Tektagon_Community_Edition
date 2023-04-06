//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <zephyr.h>
#include "state_machine.h"
#include "common_smc.h"
#include <Common.h>
#include "include/SmbusMailBoxCom.h"
#include "Smbus_mailbox/Smbus_mailbox.h"
#include "pfr/pfr_common.h"
#include <CommonLogging/CommonLogging.h>
#include <I2c/I2c.h>

#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_verification.h"
#include "intel_2.0/intel_pfr_provision.h"
#include "intel_2.0/intel_pfr_definitions.h"
#include "intel_2.0/intel_pfr_pfm_manifest.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_verification.h"
#include "cerberus/cerberus_pfr_provision.h"
#include "cerberus/cerberus_pfr_definitions.h"
#include "cerberus/cerberus_pfr_pfm_manifest.h"
#endif

#define DEBUG_HALT() {				\
	volatile int halt = 1;			\
	while (halt)				\
	{ 					\
		__asm__ volatile("nop");	\
	}					\
}

void main(void)
{
	int status = 0;
	
	printk("\r\n *** Tektagon OE version 1.1.00 ***\r\n");
	// DEBUG_HALT();
	status = initializeEngines();
	status = initializeManifestProcessor();
	DebugInit();//State Machine log saving

	BMCBootHold();
	PCHBootHold();

	#if SMBUS_MAILBOX_SUPPORT
    	InitializeSmbusMailbox();
    	SetPlatformState(ENTER_T_MINUS_1);
	#endif

	StartHrotStateMachine();
}
