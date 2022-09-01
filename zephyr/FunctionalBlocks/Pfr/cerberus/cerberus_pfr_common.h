#ifndef CERBERUS_PFR_COMMON_H_
#define CERBERUS_PFR_COMMON_H_

#if CONFIG_CERBERUS_PFR_SUPPORT
#include "pfr/pfr_common.h"
int get_rsa_public_key(uint8_t flash_id, uint32_t address, struct rsa_public_key *public_key);
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

int init_pfr_signature(struct verifcation *verification, struct pfr_pubkey *pubkey);

#endif

#endif
