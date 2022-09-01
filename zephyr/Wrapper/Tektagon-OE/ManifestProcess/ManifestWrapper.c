//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * @file ManifestWrapper.c
 * Wrapper functionality defined 
 */
 
#include "ManifestWrapper.h"
#include "platform.h"

/**
 * Release the common manifest components.
 *
 * @param manifest The manifest to release.
 */
void ManifestFlashReleaseWrapper(struct manifest_flash *manifest)
{
	if (manifest && manifest->free_signature) {
		platform_free (manifest->signature);
	}
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
int ManifestFlashVerifyWrapper (struct manifest_flash *manifest, struct hash_engine *hash,
	struct signature_verification *verification, uint8_t *hash_out, size_t hash_length)
{
	enum hash_type sig_hash;
	int status;

	if ((manifest == NULL) || (hash == NULL) || (verification == NULL)) {
		return MANIFEST_INVALID_ARGUMENT;
	}

	if ((hash_out != NULL) && (hash_length < SHA256_HASH_LENGTH)) {
		return MANIFEST_HASH_BUFFER_TOO_SMALL;
	}

	manifest->manifest_valid = false;
	manifest->cache_valid = false;
	if (hash_out != NULL) {
		/* Clear the output hash buffer to indicate no hash was calculated. */
		memset (hash_out, 0, hash_length);
	}

	status = manifest_flash_read_header (manifest, &manifest->header);
	if (status != 0) {
		return status;
	}

	if (manifest->header.sig_length > manifest->max_signature) {
		return MANIFEST_SIG_BUFFER_TOO_SMALL;
	}

	switch (manifest_get_hash_type (manifest->header.sig_type)) {
		case MANIFEST_HASH_SHA256:
			sig_hash = HASH_TYPE_SHA256;
			manifest->hash_length = SHA256_HASH_LENGTH;
			break;

		case MANIFEST_HASH_SHA384:
			sig_hash = HASH_TYPE_SHA384;
			manifest->hash_length = SHA384_HASH_LENGTH;
			break;

		case MANIFEST_HASH_SHA512:
			sig_hash = HASH_TYPE_SHA512;
			manifest->hash_length = SHA512_HASH_LENGTH;
			break;

		default:
			return MANIFEST_SIG_UNKNOWN_HASH_TYPE;
	}

	if (hash_out != NULL) {
		if (hash_length < manifest->hash_length) {
			return MANIFEST_HASH_BUFFER_TOO_SMALL;
		}
	}

	status = manifest->flash->read (manifest->flash,
		manifest->addr + manifest->header.length - manifest->header.sig_length, manifest->signature,
		manifest->header.sig_length);
	if (status != 0) {
		return status;
	}

	if (manifest->header.magic == manifest->magic_num_v1) {
		status = manifest_flash_verify_v1 (manifest, hash, verification, sig_hash, hash_out);
	}
	else {
		status = manifest_flash_verify_v2 (manifest, hash, verification, sig_hash, hash_out);
	}

	if (status == 0) {
		manifest->manifest_valid = true;
	}
	return status;
}
