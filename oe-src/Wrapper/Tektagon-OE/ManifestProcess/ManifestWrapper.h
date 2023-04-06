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

#ifndef MANIFEST_PROCESS_WRAPPER_H_
#define MANIFEST_PROCESS_WRAPPER_H_

#include "manifest.h"
#include "manifest_flash.h"
#include "firmware/app_image.h"

int ManifestFlashVerifyWrapper (struct manifest_flash *manifest, struct hash_engine *hash,
	struct signature_verification *verification, uint8_t *hash_out, size_t hash_length);
	

#endif /* MANIFEST_PROCESS_WRAPPER_H_ */
