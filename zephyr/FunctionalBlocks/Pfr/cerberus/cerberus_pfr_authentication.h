//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef CERBERUS_PFR_AUTHENTICATION_H_
#define CERBERUS_PFR_AUTHENTICATION_H_

#if CONFIG_CERBERUS_PFR_SUPPORT
#include "pfr/pfr_common.h"

int cerberus_auth_pfr_active_verify(struct pfr_manifest *manifest);
int cerberus_auth_pfr_recovery_verify(struct pfr_manifest *manifest);
#endif CONFIG_CERBERUS_PFR_SUPPORT
#endif /*CERBERUS_PFR_AUTHENTICATION_H_*/

