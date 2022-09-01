//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * @file ManifestProcess.c
 * 
 */
 
#include "ManifestProcess.h"
#include "platform.h"
/**
 * Initialize the common handling for manifests stored on flash.  Only version 1 style manifests
 * will be supported.
 *
 * @param manifest The manifest to initialize.
 * @param flash The flash device that contains the manifest.
 * @param base_addr The starting address in flash of the manifest.
 * @param magic_num The magic number that identifies the manifest.
 *
 * @return 0 if the manifest was initialized successfully or an error code.
 */
int ManifestFlashInitialize (struct manifest_flash *manifest, struct flash *flash, uint32_t base_addr, uint16_t magic_num)
{	
	uint32_t block;
	int status;
	struct hash_engine *hash = NULL;
	uint8_t *signature_cache = NULL;
	size_t max_signature = 0;
	uint8_t *platform_id_cache = NULL;
	size_t max_platform_id = 0;
	uint16_t magic_num_v1 = magic_num;
	uint16_t magic_num_v2 = MANIFEST_NOT_SUPPORTED;

	if ((manifest == NULL) || (flash == NULL)) {
		return MANIFEST_INVALID_ARGUMENT;
	}

	if ((magic_num_v2 != MANIFEST_NOT_SUPPORTED) &&
		((hash == NULL) || (platform_id_cache == NULL) || (signature_cache == NULL))) {
		return MANIFEST_INVALID_ARGUMENT;
	}

	status = flash->get_block_size (flash, &block);
	if (status != 0) {
		return status;
	}

	if (FLASH_REGION_OFFSET (base_addr, block) != 0) {
		return MANIFEST_STORAGE_NOT_ALIGNED;
	}

	memset (manifest, 0, sizeof (struct manifest_flash));

	if (signature_cache == NULL) {
		/* This can only be true if v2 manifests are not supported. */
		max_signature = RSA_KEY_LENGTH_2K;
		signature_cache = platform_malloc (max_signature);
		if (signature_cache == NULL) {
			return MANIFEST_NO_MEMORY;
		}

		manifest->free_signature = true;
	}

	manifest->flash = flash;
	manifest->hash = hash;
	manifest->addr = base_addr;
	manifest->magic_num_v1 = magic_num_v1;
	manifest->magic_num_v2 = magic_num_v2;
	manifest->signature = signature_cache;
	manifest->max_signature = max_signature;
	manifest->platform_id = (char*) platform_id_cache;
	manifest->max_platform_id = max_platform_id - 1;
	manifest->cache_valid = false;

	return 0;
}

/**
 * Verify if the manifest is valid.
 *
 * @param manifest The manifest that will be verified.
 * @param hash The hash engine to use for validation.
 * @param verification The module to use for signature verification.
 * @param hash_out Optional buffer to hold the manifest hash calculated during verification.  The
 * hash output will be valid even if the signature verification fails.  This can be set to null to
 * not save the hash value.
 * @param hash_length Length of hash output buffer.
 *
 * @return 0 if the manifest is valid or an error code.
 */
int ManifestFlashVerify (struct manifest_flash *manifest, struct hash_engine *hash,
	struct signature_verification *verification, uint8_t *hash_out, size_t hash_length)
{
	
	return ManifestFlashVerifyWrapper(manifest,hash,verification,hash_out,hash_length);
}