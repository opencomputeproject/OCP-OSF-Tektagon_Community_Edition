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

#include "PfmFlash.h"
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "pfm_format.h"
#include "common/buffer_util.h"

/**
 * Static array indicating the manifest contains no firmware identifiers.
 */
static const char *NO_FW_IDS[] = {NULL};

static int PfmFlashVerify (struct manifest *pfm, struct hash_engine *hash,
	struct signature_verification *verification, uint8_t *hash_out, size_t hash_length)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;
	
	return PfmFlashVerifyWrapper(pfm,hash,verification,hash_length);
	
}

static int PfmFlashGetId (struct manifest *pfm, uint32_t *id)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if (pfm_flash == NULL) {
		return PFM_INVALID_ARGUMENT;
	}

	return PfmFlashGetIdWrapper (&pfm_flash->base_flash, id);
}

static int PfmFlashGetPlatformId (struct manifest *pfm, char **id, size_t length)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if (pfm_flash == NULL) {
		return PFM_INVALID_ARGUMENT;
	}

	return PfmFlashGetPlatformIdWrapper (&pfm_flash->base_flash, id, length);
}

static void PfmFlashFreePlatformId (struct manifest *manifest, char *id)
{
	/* Don't need to do anything.  Manifest allocated buffers use the internal static buffer. */
}

static int PfmFlashGetHash (struct manifest *pfm, struct hash_engine *hash, uint8_t *hash_out,
	size_t hash_length)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if (pfm_flash == NULL) {
		return PFM_INVALID_ARGUMENT;
	}
	return PfmFlashGetHashWrapper();
}

static int PfmFlashGetSignature (struct manifest *pfm, uint8_t *signature, size_t length)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if (pfm_flash == NULL) {
		return PFM_INVALID_ARGUMENT;
	}

	return PfmFlashGetSignatureWrapper (&pfm_flash->base_flash, signature, length);
}

static int PfmFlashIsEmpty (struct manifest *pfm)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;
	int status;

	if (pfm_flash == NULL) {
		return PFM_INVALID_ARGUMENT;
	}

	if (!pfm_flash->base_flash.manifest_valid) {
		return MANIFEST_NO_MANIFEST;
	}

	if (pfm_flash->base_flash.header.magic == PFM_MAGIC_NUM) {
		uint8_t check;

		status = pfm_flash->base.buffer_supported_versions (&pfm_flash->base, NULL, 0, 1, &check);
		if ((status == 0) || (status == 1)) {
			status = !status;
		}
	}
	else {
		status = (pfm_flash->flash_dev_format < 0) || (pfm_flash->flash_dev.fw_count == 0);
	}

	return status;
}

static void PfmFlashFreeFirmware (struct pfm *pfm, struct pfm_firmware *fw)
{
	return PfmFlashFreeFirmwareWrapper();
}

static int PfmFlashGetFirmware (struct pfm *pfm, struct pfm_firmware *fw)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if ((pfm_flash == NULL) || (fw == NULL)) {
		return PFM_INVALID_ARGUMENT;
	}

	if (!pfm_flash->base_flash.manifest_valid) {
		return MANIFEST_NO_MANIFEST;
	}

	if (pfm_flash->base_flash.header.magic == PFM_MAGIC_NUM) {
		return pfm_flash_get_firmware_v1 (pfm_flash, fw);
	}
	else {
		return pfm_flash_get_firmware_v2 (pfm_flash, fw);
	}
}

static void PfmFlashFreeFwVersions (struct pfm *pfm, struct pfm_firmware_versions *ver_list)
{
	size_t i;

	if ((ver_list != NULL) && (ver_list->versions != NULL)) {
		for (i = 0; i < ver_list->count; i++) {
			platform_free ((void*) ver_list->versions[i].fw_version_id);
		}

		platform_free ((void*) ver_list->versions);

		memset (ver_list, 0, sizeof (*ver_list));
	}
}

static int PfmFlashGetSupportedVersions (struct pfm *pfm, const char *fw,
	struct pfm_firmware_versions *ver_list)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if ((pfm_flash == NULL) || (ver_list == NULL)) {
		return PFM_INVALID_ARGUMENT;
	}

	if (!pfm_flash->base_flash.manifest_valid) {
		return MANIFEST_NO_MANIFEST;
	}

	if (pfm_flash->base_flash.header.magic == PFM_MAGIC_NUM) {
		return pfm_flash_get_supported_versions_v1 (pfm_flash, ver_list, 0, 1, NULL, NULL);
	}
	else {
		return pfm_flash_get_supported_versions_v2 (pfm_flash, fw, ver_list, NULL, NULL, NULL,
			NULL);
	}
}

static int PfmFlashBufferSupportedVersions (struct pfm *pfm, const char *fw, size_t offset,
	size_t length, uint8_t *ver_list)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;
	struct pfm_firmware fw_list;
	
	return PfmFlashBufferSupportedVersionsWrapper();
}

static void PfmFlashFreeReadWriteRegions (struct pfm *pfm,
	struct pfm_read_write_regions *writable)
{
	if (writable != NULL) {
		platform_free ((void*) writable->regions);
		platform_free ((void*) writable->properties);

		memset (writable, 0, sizeof (*writable));
	}
}


static int PfmFlashGetReadWriteRegions (struct pfm *pfm, const char *fw, const char *version,
	struct pfm_read_write_regions *writable)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	if ((pfm_flash == NULL) || (version == NULL) || (writable == NULL)) {
		return PFM_INVALID_ARGUMENT;
	}

	if (!pfm_flash->base_flash.manifest_valid) {
		return MANIFEST_NO_MANIFEST;
	}

	if (pfm_flash->base_flash.header.magic == PFM_MAGIC_NUM) {
		return pfm_flash_get_read_write_regions_v1 (pfm_flash, version, writable);
	}
	else {
		return pfm_flash_get_read_write_regions_v2 (pfm_flash, fw, version, writable);
	}
}

static void PfmFlashFreeFirmwareImages (struct pfm *pfm, struct pfm_image_list *img_list)
{
	size_t i;

	if (img_list != NULL) {
		if (img_list->images_sig != NULL) {
			for (i = 0; i < img_list->count; i++) {
				platform_free ((void*) img_list->images_sig[i].regions);
			}

			platform_free ((void*) img_list->images_sig);
		}

		if (img_list->images_hash != NULL) {
			for (i = 0; i < img_list->count; i++) {
				platform_free ((void*) img_list->images_hash[i].regions);
			}

			platform_free ((void*) img_list->images_hash);
		}

		memset (img_list, 0, sizeof (*img_list));
	}
}

static int PfmFlashGetFirmwareImages (struct pfm *pfm, const char *fw, const char *version,
	struct pfm_image_list *img_list)
{
	struct pfm_flash *pfm_flash = (struct pfm_flash*) pfm;

	return PfmFlashGetFirmwareImagesWrapper();
}

/**
 * Initialize the interface to a PFM residing in flash memory.
 *
 * @param pfm The PFM instance to initialize.
 * @param flash The flash device that contains the PFM.
 * @param hash A hash engine to use for validating run-time access to PFM information.  If it is
 * possible for any PFM information to be requested concurrently by different threads, this hash
 * engine MUST be thread-safe.  There is no internal synchronization around the hashing operations.
 * @param base_addr The starting address of the PFM storage location.
 * @param signature_cache Buffer to hold the manifest signature.
 * @param max_signature The maximum supported length for a manifest signature.
 * @param platform_id_cache Buffer to hold the manifest platform ID.
 * @param max_platform_id The maximum platform ID length supported, including the NULL terminator.
 *
 * @return 0 if the PFM instance was initialized successfully or an error code.
 */
int PfmFlashInit (struct pfm_flash *pfm, struct flash *flash, struct hash_engine *hash,
	uint32_t base_addr, uint8_t *signature_cache, size_t max_signature, uint8_t *platform_id_cache,
	size_t max_platform_id)
{
	int status;

	if (pfm == NULL) {
		return PFM_INVALID_ARGUMENT;
	}

	memset (pfm, 0, sizeof (struct pfm_flash));

	status = manifest_flash_v2_init (&pfm->base_flash, flash, hash, base_addr, PFM_MAGIC_NUM,
		PFM_V2_MAGIC_NUM, signature_cache, max_signature, platform_id_cache, max_platform_id);
	if (status != 0) {
		return status;
	}

	pfm->base.base.verify = PfmFlashVerify;
	pfm->base.base.get_id = PfmFlashGetId;
	pfm->base.base.get_platform_id = PfmFlashGetPlatformId;
	pfm->base.base.free_platform_id = PfmFlashFreePlatformId;
	pfm->base.base.get_hash = PfmFlashGetHash;
	pfm->base.base.get_signature = PfmFlashGetSignature;
	pfm->base.base.is_empty = PfmFlashIsEmpty;

	pfm->base.get_firmware = PfmFlashGetFirmware;
	pfm->base.free_firmware = PfmFlashFreeFirmware;
	pfm->base.get_supported_versions = PfmFlashGetSupportedVersions;
	pfm->base.free_fw_versions = PfmFlashFreeFwVersions;
	pfm->base.buffer_supported_versions = PfmFlashBufferSupportedVersions;
	pfm->base.get_read_write_regions = PfmFlashGetReadWriteRegions;
	pfm->base.free_read_write_regions = PfmFlashFreeReadWriteRegions;
	pfm->base.get_firmware_images = PfmFlashGetFirmwareImages;
	pfm->base.free_firmware_images = PfmFlashFreeFirmwareImages;

	return 0;
}