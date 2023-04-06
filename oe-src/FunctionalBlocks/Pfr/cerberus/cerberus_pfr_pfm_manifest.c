//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include "cerberus_pfr_pfm_manifest.h"
#include "cerberus_pfr_definitions.h"
#include "state_machine/common_smc.h"
#include "cerberus_pfr_provision.h"
#include "pfr/pfr_common.h"

#undef DEBUG_PRINTF
#if CERBERUS_MANIFEST_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

uint32_t cerberus_g_pfm_manifest_length = 1;
uint32_t cerberus_g_fvm_manifest_length = 1;

uint8_t cerberus_g_active_pfm_svn;

ProtectLevelMask cerberus_pch_protect_level_mask_count;
ProtectLevelMask cerberus_bmc_protect_level_mask_count;

int cerberus_pfm_spi_region_verification(struct pfr_manifest *manifest);
Manifest_Status cerberus_get_pfm_manifest_data(struct pfr_manifest *manifest, uint32_t *position, void *spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition);

int cerberus_pfm_version_set(struct pfr_manifest *manifest, uint32_t read_address)
{
	return Success;
}

int cerberus_get_recover_pfm_version_details(struct pfr_manifest *manifest, uint32_t address)
{
	return Success;
}

int cerberus_read_statging_area_pfm(struct pfr_manifest *manifest, uint8_t *svn_version)
{
	return Success;
}

int cerberus_spi_region_hash_verification(struct pfr_manifest *pfr_manifest, PFM_SPI_DEFINITION *PfmSpiDefinition, uint8_t *pfm_spi_Hash)
{
	return Success;
}

int cerberus_get_fvm_start_address (struct pfr_manifest *manifest, uint32_t *fvm_address)
{
	return manifest_success;
}

int cerberus_get_fvm_address(struct pfr_manifest *manifest, uint16_t fv_type)
{
	return 0;
}

int cerberus_get_spi_region_hash(struct pfr_manifest *manifest, uint32_t address, PFM_SPI_DEFINITION *p_spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition)
{
	return SHA256_SIZE;
}

void cerberus_set_protect_level_mask_count(struct pfr_manifest *manifest, PFM_SPI_DEFINITION *spi_definition)
{
	int status = 0;
	uint8_t local_count = 0;

	if (manifest->image_type == PCH_TYPE) {
		if (cerberus_pch_protect_level_mask_count.Calculated == 1) {
			return;
		} else {
			if (spi_definition->ProtectLevelMask.RecoverOnFirstRecovery == 1) {
				local_count = 1;
			} else if (spi_definition->ProtectLevelMask.RecoverOnSecondRecovery == 1) {
				local_count = 2;
			} else if (spi_definition->ProtectLevelMask.RecoverOnThirdRecovery == 1) {
				local_count = 3;
			}
			if (cerberus_pch_protect_level_mask_count.Count != 0 && cerberus_pch_protect_level_mask_count.Count > local_count) {
				cerberus_pch_protect_level_mask_count.Count = local_count;
			} else if (cerberus_pch_protect_level_mask_count.Count == 0) {
				cerberus_pch_protect_level_mask_count.Count = 1;
			}
		}

	} else {
		if (cerberus_bmc_protect_level_mask_count.Calculated == 1) {
			return;
		} else {
			if (spi_definition->ProtectLevelMask.RecoverOnFirstRecovery == 1) {
				local_count = 1;
			} else if (spi_definition->ProtectLevelMask.RecoverOnSecondRecovery == 1) {
				local_count = 2;
			} else if (spi_definition->ProtectLevelMask.RecoverOnThirdRecovery == 1) {
				local_count = 3;
			}
			if (cerberus_bmc_protect_level_mask_count.Count != 0 && cerberus_bmc_protect_level_mask_count.Count > local_count) {
				cerberus_bmc_protect_level_mask_count.Count = local_count;
			} else if (bmc_protect_level_mask_count.Count == 0) {
				cerberus_bmc_protect_level_mask_count.Count = 1;
			}
		}

	}
}

Manifest_Status cerberus_get_pfm_manifest_data(struct pfr_manifest *manifest, uint32_t *position, void *spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition)
{

	return manifest_success;

}

int cerberus_pfm_spi_region_verification(struct pfr_manifest *manifest)
{
	return Success;
}

Manifest_Status cerberus_get_fvm_manifest_data(struct pfr_manifest *manifest, uint32_t *position, PFM_FVM_ADDRESS_DEFINITION *p_fvm_address_definition, void *spi_definition, uint8_t *fvm_spi_hash, uint8_t fvm_def_type)
{
	return manifest_success;
}
