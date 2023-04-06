//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include "pfr_common.h"
#include "CommonFlash/CommonFlash.h"
#include "recovery/recovery_image.h"
#include "pfr_util.h"

#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_common.h"
#include "intel_2.0/intel_pfr_verification.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_common.h"
#include "cerberus/cerberus_pfr_verification.h"
#endif
struct pfr_manifest pfr_manifest;

//Block0-Block1 verifcation
struct active_image pfr_active_image;

// PFR_SIGNATURE
struct signature_verification pfr_verification;
struct pfr_pubkey  pubkey;
struct pfr_signature_verification verification;

//PFR_MANIFEST
struct manifest manifest_base;

//PFR RECOVERY 
struct recovery_image recovery_base;
struct pfm_manager recovery_pfm;

//PFR UPDATE
struct firmware_image update_base;
struct pfr_firmware_image update_fw;

//PFR_KEYSTORE
struct keystore keystore;
struct key_cancellation_flag kc_flag;
struct pfr_keystore pfr_keystore;

struct pfr_authentication pfr_authentication;
struct pfr_hash pfr_hash;

struct pfr_manifest *get_pfr_manifest(){
    return &pfr_manifest;
} 

struct active_image *get_active_image(){
    return &pfr_active_image;
}

static struct manifest *get_manifest(){
    return &manifest_base;
} 

static struct verifcation *get_signature_verification(){
    return &pfr_verification;
}

static struct pfr_pubkey *get_pubkey(){
    return &pubkey;
}

struct pfr_signature_verification *get_pfr_signature_verification(){
    return &verification;
}

struct keystore *get_keystore(){
    return &keystore;
}

static struct key_cancellation_flag *get_kc_flag(){
    return &kc_flag;
}

struct pfr_keystore *get_pfr_keystore(){
    return &pfr_keystore;
}

static struct pfr_authentication *get_pfr_authentication(){
    return &pfr_authentication;
}

static struct pfr_hash *get_pfr_hash(){
    return &pfr_hash;
}

static struct hash_engine *get_pfr_hash_engine()
{
	return get_hash_engine_instance();
}

static struct spi_flash *get_pfr_spi_flash(){
    struct SpiEngine *spi_flash = getSpiEngineWrapper();
    return &spi_flash->spi;
}

static struct recovery_image *get_recovery_base(){
    return &recovery_base;
}


static struct pfm_manager *get_recovery_pfm(){
    return &recovery_pfm;
}

static struct pfr_firmware_image *get_update_fw_base(){
    return &update_fw;
}

static struct firmware_image *get_update_base(){
    return &update_base;
}

void init_pfr_manifest(){

    init_pfr_keystore(get_keystore(), get_kc_flag());

    init_pfr_signature(get_signature_verification(), get_pubkey());

    init_pfr_firmware_image(get_update_fw_base(), get_update_base());

    init_lib_pfr_manifest(get_pfr_manifest(), 
							get_manifest(), 
							get_pfr_hash_engine(),
							get_pfr_signature_verification(),
							get_pfr_spi_flash(),
							get_pfr_keystore(),
							get_pfr_authentication(),
							get_pfr_hash(),
                            get_recovery_base(),
                            get_recovery_pfm(),
                            get_update_fw_base(),
                            get_active_image());

}

