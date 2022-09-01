//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * manifestProcessor.c
 *
 *  Created on: Dec 15, 2021
 *      Author: presannar
 */
#include <assert.h>

#include "include/definitions.h"
#include "manifestProcessor.h"
#include <Common.h>
#include "firmware/app_image.h"

int initializeManifestProcessor(void)
{
	int status = 0;

	status = ManifestFlashInitialize(getManifestFlashInstance(), getFlashDeviceInstance(), PFM_FLASH_MANIFEST_ADDRESS, PFM_V2_MAGIC_NUM);
	if (status)
		return status;

	init_pfr_manifest();
	// status = pfm_manager_flash_init(getPfmManagerFlashInstance(), getPfmFlashInstance(), getPfmFlashInstance(),
	// getHostStateManagerInstance(), get_hash_engine_instance(), getSignatureVerificationInstance());

	return status;
}

void uninitializeManifestProcessor(void)
{
	manifest_flash_release(getManifestFlashInstance());
}

int processPfmFlashManifest(void)
{
	int status = 0;
	uint8_t *hashStorage = getNewHashStorage();
	struct manifest_flash *manifest_flash = getManifestFlashInstance();

	// printk("Manifest Verification\n");
	status = ManifestFlashVerify(manifest_flash, get_hash_engine_instance(),
				       getSignatureVerificationInstance(), hashStorage, hashStorageLength);

	if (true == manifest_flash->manifest_valid) {
		printk("Manifest Verificaation Successful\n");

		status = perform_image_verification();
	}

	return status;
}
