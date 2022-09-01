//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef CERBERUS_PFR_PROVISION_H_
#define CERBERUS_PFR_PROVISION_H_

#include <stdint.h>
#include "cerberus_pfr_definitions.h"
#include "pfr/pfr_common.h"

extern int g_provision_data;

#define PROVISIONING_IMAGE_TYPE			0x02
#define PROVISION_ROOT_KEY_FLAG			0x01
#define PROVISION_OTP_KEY_FLAG			0x0f
#define CERBERUS_ROOT_KEY_ADDRESS		0x200
enum {
	UFM_STATUS,
	ROOT_KEY_HASH                                           = 0x004,
	PCH_ACTIVE_PFM_OFFSET                                   = 0x024,
	PCH_RECOVERY_REGION_OFFSET                              = 0x028,
	PCH_STAGING_REGION_OFFSET                               = 0x02c,
	BMC_ACTIVE_PFM_OFFSET                                   = 0x030,
	BMC_RECOVERY_REGION_OFFSET                              = 0x034,
	BMC_STAGING_REGION_OFFSET                               = 0x038,
	PIT_PASSWORD                                            = 0x03c,
	PIT_PCH_FW_HASH                                         = 0x044,
	PIT_BMC_FW_HASH                                         = 0x064,
	SVN_POLICY_FOR_CPLD_UPDATE                              = 0x084,
	SVN_POLICY_FOR_PCH_FW_UPDATE                            = 0x08c,
	SVN_POLICY_FOR_BMC_FW_UPDATE                            = 0x094,
	KEY_CANCELLATION_POLICY_FOR_SIGNING_PCH_PFM             = 0x09c,
	KEY_CANCELLATION_POLICY_FOR_SIGNING_PCH_UPDATE_CAPSULE  = 0x0ac,
	KEY_CANCELLATION_POLICY_FOR_SIGNING_BMC_PFM             = 0x0bc,
	KEY_CANCELLATION_POLICY_FOR_SIGNING_BMC_UPDATE_CAPSULE  = 0x0cc,
	KEY_CANCELLATION_POLICY_FOR_SIGNING_CPLD_UPDATE_CAPSULE = 0x0dc
};

enum CERBERUS_PROVISION_STRUCT{
	CERBERUS_PROVISION_IMAGE_LENGTH							= 0x000,
	CERBERUS_IMAGE_TYPE										= 0x002,
	CERBERUS_MAGIC_NUM										= 0x004,
	CERBERUS_MANIFEST_LENGTH								= 0x008,
	CERBERUS_MANIFEST_FLAG									= 0x00A,
	CERBERUS_PROVISION_RESERVED								= 0x00E,
	CERBERUS_BMC_ACTIVE_OFFSET								= 0x010,
	CERBERUS_BMC_ACTIVE_SIZE								= 0x014,
	CERBERUS_BMC_RECOVERY_OFFSET							= 0x018,
	CERBERUS_BMC_RECOVERY_SIZE								= 0x01C,
	CERBERUS_BMC_STAGE_OFFSET								= 0x020,
	CERBERUS_BMC_STAGE_SIZE									= 0x024,
	CERBERUS_PCH_ACTIVE_OFFSET								= 0x028,
	CERBERUS_PCH_ACTIVE_SIZE								= 0x02C,
	CERBERUS_PCH_RECOVERY_OFFSET							= 0x030,
	CERBERUS_PCH_RECOVERY_SIZE								= 0x034,
	CERBERUS_PCH_STAGE_OFFSET								= 0x038,
	CERBERUS_PCH_STAGE_SIZE									= 0x03C,
	CERBERUS_ROOT_KEY_LENGTH								= 0x040,
	CERBERUS_ROOT_KEY										= 0x042
};

struct PROVISIONING_IMAGE_HEADER{
	uint16_t image_length;
	uint16_t image_type;
	uint32_t magic_num;
	uint16_t manifest_length;
	uint8_t provisioning_flag[4];
	uint8_t reserved[2];
};

struct PROVISIONING_MANIFEST_DATA{
	
	uint32_t bmc_active_manifest_offset;
	uint32_t bmc_active_size;
	uint32_t bmc_recovery_offset;
	uint32_t bmc_recovery_size;
	uint32_t bmc_stage_offset;
	uint32_t bmc_stage_size;
	uint32_t pch_active_manifest_offset;
	uint32_t pch_active_size;
	uint32_t pch_recovery_offset;
	uint32_t pch_recovery_size;
	uint32_t pch_stage_offset;
	uint32_t pch_stage_size;

};

int cerberus_provisioning_root_key_action(struct pfr_manifest *manifest);
int getCerberusProvisionData(int offset, uint8_t *data, uint32_t length);
#endif /*INTEL_PFR_PROVISION_H*/

