#ifndef INTEL_PFR_COMMON_H_
#define INTEL_PFR_COMMON_H_

#if CONFIG_INTEL_PFR_SUPPORT
#include "pfr/pfr_common.h"
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
    struct active_image *active_image);

int init_pfr_keystore(struct keystore *keystore, struct key_cancellation_flag *kc_flag);
int init_pfr_signature(struct signature_verification *verification, struct pfr_pubkey *pubkey);
void init_pfr_firmware_image(struct pfr_firmware_image *update_fw, struct firmware_image *update_base);
#endif

#endif