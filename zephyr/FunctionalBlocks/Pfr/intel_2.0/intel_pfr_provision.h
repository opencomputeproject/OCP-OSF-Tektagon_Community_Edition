//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef INTEL_PFR_PROVISION_H_
#define INTEL_PFR_PROVISION_H_

#include <stdint.h>
// #include "intel_pfr_verification.h"
// #include "pfr/pfr_common.h"

extern int g_provision_data;

enum
{
    UFM_STATUS,
    ROOT_KEY_HASH = 0x004,
    PCH_ACTIVE_PFM_OFFSET = 0x024,
    PCH_RECOVERY_REGION_OFFSET = 0x028,
    PCH_STAGING_REGION_OFFSET = 0x02c,
    BMC_ACTIVE_PFM_OFFSET = 0x030,
    BMC_RECOVERY_REGION_OFFSET = 0x034,
    BMC_STAGING_REGION_OFFSET = 0x038,
    PIT_PASSWORD = 0x03c,
    PIT_PCH_FW_HASH = 0x044,
    PIT_BMC_FW_HASH = 0x064,
    SVN_POLICY_FOR_CPLD_UPDATE = 0x084,
    SVN_POLICY_FOR_PCH_FW_UPDATE = 0x08c,
    SVN_POLICY_FOR_BMC_FW_UPDATE = 0x094,
    KEY_CANCELLATION_POLICY_FOR_SIGNING_PCH_PFM = 0x09c,
    KEY_CANCELLATION_POLICY_FOR_SIGNING_PCH_UPDATE_CAPSULE = 0x0ac,
    KEY_CANCELLATION_POLICY_FOR_SIGNING_BMC_PFM = 0x0bc,
    KEY_CANCELLATION_POLICY_FOR_SIGNING_BMC_UPDATE_CAPSULE = 0x0cc,
    KEY_CANCELLATION_POLICY_FOR_SIGNING_CPLD_UPDATE_CAPSULE = 0x0dc
};

// int verify_root_key_hash(struct pfr_manifest *manifest, uint8_t *root_public_key);
// int verify_root_key_data(struct pfr_manifest *manifest, uint8_t *pubkey_x, uint8_t *pubkey_y);
// int verify_root_key_entry(struct pfr_manifest *manifest, PFR_AUTHENTICATION_BLOCK1 *block1_buffer);


#endif /*INTEL_PFR_PROVISION_H*/

