//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * manifestProcessor.h
 *
 */

#ifndef MANIFEST_PROCESS_H_
#define MANIFEST_PROCESS_H_

#include "manifest.h"
#include "manifest_flash.h"
#include "firmware/app_image.h"

int ManifestFlashVerify (struct manifest_flash *manifest, struct hash_engine *hash,
	struct signature_verification *verification, uint8_t *hash_out, size_t hash_length);
	
int ManifestFlashInitialize (struct manifest_flash *manifest, struct flash *flash, uint32_t base_addr, uint16_t magic_num);

#endif /* MANIFEST_PROCESS_H_ */
