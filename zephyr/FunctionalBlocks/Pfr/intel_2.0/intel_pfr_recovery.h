//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef INTEL_PFR_RECOVERY_H_
#define INTEL_PFR_RECOVERY_H_

#include <stdint.h>
#include "manifest/pfm/pfm_manager.h"

int intel_pfr_recovery_verify (struct recovery_image *image, struct hash_engine *hash,
		struct signature_verification *verification, uint8_t *hash_out, size_t hash_length,
		struct pfm_manager *pfm);

#endif