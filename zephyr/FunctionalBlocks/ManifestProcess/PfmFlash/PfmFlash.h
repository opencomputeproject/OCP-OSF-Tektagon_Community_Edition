//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Pfm Handling functions
 */

#ifndef PFM_FLASH_H_
#define PFM_FLASH_H_

#include "pfm.h"
#include "pfm_flash.h"
#include "flash/flash_util.h"
#include "manifest/manifest_flash.h"

int PfmFlashInit (struct pfm_flash *pfm, struct flash *flash, struct hash_engine *hash,
	uint32_t base_addr, uint8_t *signature_cache, size_t max_signature, uint8_t *platform_id_cache,
	size_t max_platform_id);

#endif /* PFM_FLASH_H_ */
