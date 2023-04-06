#if CONFIG_CERBERUS_PFR_SUPPORT

#include "pfr/pfr_common.h"
#include "pfr/pfr_util.h"
#include "cerberus_pfr_authentication.h"
#include "cerberus_pfr_verification.h"
#include "cerberus_pfr_recovery.h"
#include "cerberus_pfr_update.h"
#include "cerberus_pfr_definitions.h"
#include "cerberus_pfr_provision.h"
#include "cerberus_pfr_key_cancellation.h"
#include "state_machine/common_smc.h"
#include "crypto/rsa.h"

#if PF_STATUS_DEBUG
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF printk
#endif
#else
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(...)
#endif
#endif

extern struct pfr_keystore *get_pfr_keystore();
extern struct pfr_signature_verification *get_pfr_signature_verification();
/**
 * Verify if the manifest is valid.
 *
 * @param manifest The manifest to validate.
 * @param hash The hash engine to use for validation.
 * @param verification Verification instance to use to verify the manifest signature.
 * @param hash_out Optional output buffer for manifest hash calculated during verification.  A
 * validation error does not necessarily mean the hash output is not valid.  If the manifest
 * hash was not calculated, this buffer will be cleared.  Set this to null to not return the
 * manifest hash.
 * @param hash_length Length of the hash output buffer.
 *
 * @return 0 if the manifest is valid or an error code.
 */
int manifest_verify (struct manifest *manifest, struct hash_engine *hash,
    struct signature_verification *verification, uint8_t *hash_out, size_t hash_length){

    return cerberus_pfr_manifest_verify(manifest, hash, verification, hash_out, hash_length);
}

/**
 * Get the ID of the manifest.
 *
 * @param manifest The manifest to query.
 * @param id The buffer to hold the manifest ID.
 *
 * @return 0 if the ID was successfully retrieved or an error code.
 */
int manifest_get_id (struct manifest *manifest, uint32_t *id){
    
    ARG_UNUSED(manifest);
	ARG_UNUSED(id);

    return Success;
}

/**
 * Get the string identifier of the platform for the manifest.
 *
 * @param manifest The manifest to query.
 * @param id Pointer to the output buffer for the platform identifier.  The buffer pointer
 * cannot be null, but if the buffer itself is null, the manifest instance will allocate an
 * output buffer for the platform identifier.  When using a manifest-allocated buffer, the
 * output must be treated as const (i.e. do not modify the contents) and must be freed by
 * calling free_platform_id on the same instance that allocated it.
 * @param length Length of the output buffer if the buffer is static (i.e. not null).  This
 * argument is ignored when using manifest allocation.
 *
 * @return 0 if the platform ID was retrieved successfully or an error code.
 */
int manifest_get_platform_id (struct manifest *manifest, char **id, size_t length){
    
    ARG_UNUSED(manifest);
	ARG_UNUSED(id);
    ARG_UNUSED(length);

    return Success;
}

/**
 * Free a platform identifier allocated by a manifest instance.  Do not call this function for
 * static buffers owned by the caller.
 *
 * @param manifest The manifest that allocated the platform identifier.
 * @param id The platform identifier to free.
 */
void manifest_free_platform_id(struct manifest *manifest, char *id){
    
    ARG_UNUSED(manifest);
	ARG_UNUSED(id);

    return Success;
}

/**
 * Get the hash of the manifest data, not including the signature.  The hash returned will be
 * calculated using the same algorithm as was used to generate the manifest signature.
 *
 * @param manifest The manifest to query.
 * @param hash The hash engine to use for generating the hash.
 * @param hash_out Output buffer for the manifest hash.
 * @param hash_length Length of the hash output buffer.
 *
 * @return Length of the hash if it was calculated successfully or an error code.  Use
 * ROT_IS_ERROR to check the return value.
 */
int manifest_get_hash (struct manifest *manifest, struct hash_engine *hash, uint8_t *hash_out,
    size_t hash_length){
    
    return get_hash(manifest, hash, hash_out, hash_length);
}

/**
 * Get the signature of the manifest.
 *
 * @param manifest The manifest to query.
 * @param signature Output buffer for the manifest signature.
 * @param length Length of the signature buffer.
 *
 * @return The length of the signature or an error code.  Use ROT_IS_ERROR to check the return
 * value.
 */
int manifest_get_signature(struct manifest *manifest, uint8_t *signature, size_t length){
    
    ARG_UNUSED(manifest);
    ARG_UNUSED(signature);
    ARG_UNUSED(length);

    return Success;
}

/**
 * Determine if the manifest is considered to be empty.  What indicates an empty manifest will
 * depend on the specific implementation, and it doesn't necessarily mean there is no data in
 * the manifest.
 *
 * @param manifest The manifest to query.
 *
 * @return 1 if the manifest is empty, 0 if it is not, or an error code.
 */
int is_manifest_empty (struct manifest *manifest){
    ARG_UNUSED(manifest);
    return Success;
}

void init_manifest(struct manifest *manifest){
    manifest->verify = manifest_verify;
    manifest->get_id = manifest_get_id;
    manifest->get_platform_id = manifest_get_platform_id;
    manifest->free_platform_id = manifest_free_platform_id;
    manifest->get_hash = manifest_get_hash;
    manifest->get_signature = manifest_get_signature;
    manifest->is_empty = is_manifest_empty;
}

/**
 * Verify if the recovery image is valid.
 *
 * @param image The recovery image to validate.
 * @param hash The hash engine to use for validation.
 * @param verification Verification instance to use to verify the recovery image signature.
 * @param hash_out Optional output buffer for the recovery image hash calculated during
 * verification.  Set to null to not return the hash.
 * @param hash_length Length of the hash output buffer.
 * @param pfm_manager The PFM manager to use for validation.
 *
 * @return 0 if the recovery image is valid or an error code.
 */
int recovery_verify (struct recovery_image *image, struct hash_engine *hash,
    struct signature_verification *verification, uint8_t *hash_out, size_t hash_length,
    struct pfm_manager *pfm){

    return cerberus_pfr_recovery_verify (image,hash,verification, hash_out, hash_length, pfm);
}

/**
 * Get the SHA-256 hash of the recovery image data, not including the signature.
 *
 * @param image The recovery image to query.
 * @param hash The hash engine to use for generating the hash.
 * @param hash_out Output buffer for the manifest hash.
 * @param hash_length Length of the hash output buffer.
 *
 * @return 0 if the hash was calculated successfully or an error code.
 */
int recovery_get_hash(struct recovery_image *image, struct hash_engine *hash, uint8_t *hash_out,
    size_t hash_length){

    ARG_UNUSED(image);
	ARG_UNUSED(hash);
    ARG_UNUSED(hash_out);
    ARG_UNUSED(hash_length);

    return Success;
}

/**
 * Get the version of the recovery image.
 *
 * @param image The recovery image to query.
 * @param version The buffer to hold the version ID.
 * @param len The output buffer length.
 *
 * @return 0 if the ID was successfully retrieved or an error code.
 */
int recovery_get_version (struct recovery_image *image, char *version, size_t len){

    ARG_UNUSED(image);
    ARG_UNUSED(version);
    ARG_UNUSED(len);

    return Success;
}

/**
 * Apply the recovery image to host flash.  It is assumed that the host flash region is already
 * blank.
 *
 * @param image The recovery image to query.
 * @param flash The flash device to write the recovery image to.
 *
 * @return 0 if applying the recovery image to host flash was successful or an error code.
 */
int recovery_apply_to_flash (struct recovery_image *image, struct spi_flash *flash){
    
    int status  = 0;        
    struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) image;

    return pfr_recover_update_action(pfr_manifest);

}

void init_recovery_manifest(struct recovery_image *image){
    image->verify = recovery_verify;
    image->get_hash = recovery_get_hash;
    image->get_version = recovery_get_version;
    image->apply_to_flash = recovery_apply_to_flash;

}

/**
 * Get the total size of the firmware image.
 *
 * @param fw The firmware image to query.
 *
 * @return The size of the firmware image or an error code.  Use ROT_IS_ERROR to check the
 * return value.
 */
int firmware_image_get_image_size(struct firmware_image *fw){
	ARG_UNUSED(fw);
	return Success;
}

/**
 * Get the key manifest for the current firmware image.
 *
 * @param fw The firmware image to query.
 *
 * @return The image key manifest or null if there is an error.  The memory for the key
 * manifest is managed by the firmware image instance and is only guaranteed to be valid until
 * the next call to 'load'.
 */
struct key_manifest* firmware_imag_eget_key_manifest (struct firmware_image *fw){
	ARG_UNUSED(fw);
	return NULL;
}

/**
 * Get the main image header for the current firmware image.
 *
 * @param fw The firmware image to query.
 *
 * @return The image firmware header or null if there is an error.  The memory for the header
 * is managed by the firmware image instance and is only guaranteed to be valid until the next
 * call to 'load'.
 */
struct firmware_header* firmware_image_get_firmware_header (struct firmware_image *fw){
	ARG_UNUSED(fw);
	return NULL;
}

/**
 * Verify the complete firmware image.  All components in the image will be fully validated.
 * This includes checking image signatures and key revocation.
 *
 * @param fw The firmware image to validate.
 * @param hash The hash engine to use for validation.
 * @param rsa The RSA engine to use for signature checking.
 *
 * @return 0 if the firmware image is valid or an error code.
 */
int firmware_image_verify (struct firmware_image *fw, struct hash_engine *hash, struct rsa_engine *rsa)
{
    return cerberus_pfr_update_verify(fw, hash, rsa);	
}

void init_update_fw_manifest(struct firmware_image *fw)
{
	// fw->load = firmware_image_load;
	fw->verify = firmware_image_verify;
	// fw->get_image_size = firmware_image_get_image_size;
}

void init_signature_verifcation(struct signature_verification *signature_verification)
{
    signature_verification->verify_signature = verify_signature;
}

void init_active_image(struct active_image *active_image)
{
    active_image->verify = cerberus_auth_pfr_active_verify;
}

int init_pfr_keystore(struct keystore *keystore, struct key_cancellation_flag *kc_flag)
{
    
    int status = 0;

    struct pfr_keystore *pfr_keystore = get_pfr_keystore();
    pfr_keystore->base = keystore;
    pfr_keystore->kc_flag = kc_flag;
    pfr_keystore->kc_flag->verify_kc_flag = cerberus_verify_csk_key_id;
    pfr_keystore->kc_flag->cancel_kc_flag = cerberus_cancel_csk_key_id;

    return status;
}

int init_pfr_signature(struct verifcation *verification, struct pfr_pubkey *pubkey)
{
    int status = 0;

    struct pfr_signature_verification *pfr_verification = get_pfr_signature_verification();
    pfr_verification->base = verification;
    pfr_verification->pubkey = pubkey;
    
    return status; 
}

void init_pfr_firmware_image(struct pfr_firmware_image *update_fw, struct firmware_image *update_base)
{
    update_fw->base = update_base;
}

void init_lib_pfr_manifest(struct pfr_manifest *pfr_manifest, 
    struct manifest *manifest, 
    struct hash_engine *hash,
    struct pfr_signature_verification *verification,
    struct spi_flash *flash,
    struct pfr_keystore *keystore,
    struct pfr_authentication *pfr_authentication,
    struct pfr_hash *pfr_hash,
    struct recovery_image *recovery_base,
    struct pfm_manager *recovery_pfm,
    struct pfr_firmware_image *update_fw,
    struct active_image *active_image)
{
    int status = 0;

    pfr_manifest->base = manifest;
    pfr_manifest->hash = hash;
    pfr_manifest->verification = verification;
    pfr_manifest->flash = flash;
    pfr_manifest->keystore = keystore;  
    pfr_manifest->pfr_authentication = pfr_authentication;
    pfr_manifest->pfr_hash = pfr_hash;
    pfr_manifest->recovery_base = recovery_base;
    pfr_manifest->recovery_pfm = recovery_pfm;
    pfr_manifest->update_fw = update_fw;
    pfr_manifest->active_image_base = active_image;

    init_manifest(manifest);
    init_recovery_manifest(recovery_base);
    init_update_fw_manifest(update_fw->base);
    init_signature_verifcation(pfr_manifest->verification->base);
    init_active_image(pfr_manifest->active_image_base);
}

int get_rsa_public_key(uint8_t flash_id, uint32_t address, struct rsa_public_key *public_key)
{
	int status;
    status = pfr_spi_read(flash_id, address, sizeof(struct rsa_public_key)-1, public_key);
   	if(status != Success)
    {
    DEBUG_PRINTF("SPI Read Unsuccessful\r\n");
		return Failure;
    }
    

    return status;
}

#endif
