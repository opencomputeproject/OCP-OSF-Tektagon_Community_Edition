//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef COMMON_SMC_H
#define COMMON_SMC_H

/* List of HRoT states */
enum HRoT_state { IDLE, INITIALIZE, I2C, VERIFY, RECOVERY, UPDATE, LOCKDOWN };

typedef enum {
	Success,
	Failure,
	ManifestCorruption,
	VerifyRecovery,
	VerifyActive,
	UnSupported,
	Decommission_Success,
	Lockdown
} Verification_Status;

enum _hrot_event {
	BMC_EVENT = 1,
	PCH_EVENT,
	I2C_EVENT
};

#endif  // #ifndef COMMON_SMC_H
