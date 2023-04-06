//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include "intel_pfr_pfm_manifest.h"
#include "intel_pfr_definitions.h"
#include "state_machine/common_smc.h"
#include "intel_pfr_provision.h"
#include "pfr/pfr_common.h"

#undef DEBUG_PRINTF
#if INTEL_MANIFEST_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

uint32_t g_pfm_manifest_length = 1;
uint32_t g_fvm_manifest_length = 1;

uint8_t g_active_pfm_svn = 0;

ProtectLevelMask pch_protect_level_mask_count;
ProtectLevelMask bmc_protect_level_mask_count;

int pfm_spi_region_verification(struct pfr_manifest *manifest);
Manifest_Status get_pfm_manifest_data(struct pfr_manifest *manifest, uint32_t *position,void *spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition);

int pfm_version_set(struct pfr_manifest *manifest, uint32_t read_address)
{
    int status = 0;
	uint8_t  active_svn;
    uint16_t active_major_version, active_minor_version;
    uint8_t  ufm_svn = 0;
    uint8_t buffer[sizeof(PFM_STRUCTURE_1)];

	status = pfr_spi_read(manifest->image_type, read_address, sizeof(PFM_STRUCTURE_1), buffer); 
    if(status != Success){
        DEBUG_PRINTF("Pfm Version Set failed...\r\n");
        return Failure;
    }

    if(((PFM_STRUCTURE_1 *)buffer)->PfmTag == PFMTAG)
        DEBUG_PRINTF("PfmTag verification success...\r\n");
    else{
        DEBUG_PRINTF("PfmTag verification failed...\r\n");
        return Failure;
    }

    active_svn = ((PFM_STRUCTURE_1 *)buffer)->SVN;
    active_major_version = ((PFM_STRUCTURE_1 *)buffer)->PfmRevision;
    active_major_version = active_major_version & 0xFF;
    active_minor_version = ((PFM_STRUCTURE_1 *)buffer)->PfmRevision;
    active_minor_version = active_minor_version >> 8;

    if (manifest->image_type == PCH_TYPE){
       SetPchPfmActiveSvn(active_svn);
       SetPchPfmActiveMajorVersion(active_major_version);
       SetPchPfmActiveMinorVersion(active_minor_version);

       ufm_svn = get_ufm_svn(manifest, SVN_POLICY_FOR_PCH_FW_UPDATE);
	    if(ufm_svn < active_svn) {
    	   status = set_ufm_svn(manifest, SVN_POLICY_FOR_PCH_FW_UPDATE, active_svn);
       }

   }else if(manifest->image_type == BMC_TYPE){
       SetBmcPfmActiveSvn(active_svn);
       SetBmcPfmActiveMajorVersion(active_major_version);
       SetBmcPfmActiveMinorVersion(active_minor_version);

       ufm_svn = get_ufm_svn(manifest, SVN_POLICY_FOR_BMC_FW_UPDATE);
       if (ufm_svn < active_svn) {
    	   status = set_ufm_svn(manifest, SVN_POLICY_FOR_BMC_FW_UPDATE,active_svn);
       }
   }
    
	return Success;
}

int get_recover_pfm_version_details(struct pfr_manifest *manifest, uint32_t address)
{
    int status = 0;
	uint32_t pfm_data_address=0;
    uint16_t recovery_major_version, recovery_minor_version;
	uint8_t  recovery_svn;
    uint8_t  ufm_svn;
    PFM_STRUCTURE_1 *pfm_data;
    uint8_t  buffer[sizeof(PFM_STRUCTURE_1)];

    //PFM data start address after Recovery block and PFM block
    pfm_data_address = address + PFM_SIG_BLOCK_SIZE + PFM_SIG_BLOCK_SIZE;
	
	status = pfr_spi_read(manifest->image_type, pfm_data_address, sizeof(PFM_STRUCTURE_1), buffer);
    if(status != Success){
        DEBUG_PRINTF("Get Recover Pfm Version Details failed...\r\n");
        return Failure;
    }

    pfm_data = (PFM_STRUCTURE_1 *)buffer;
    recovery_svn = pfm_data->SVN;
    recovery_major_version = pfm_data->PfmRevision;
    recovery_major_version = recovery_major_version & 0xFF;
    recovery_minor_version = pfm_data->PfmRevision;
    recovery_minor_version = recovery_minor_version >> 8;

    //MailBox Communication
    if (manifest->image_type == PCH_TYPE){
        SetPchPfmRecoverSvn(recovery_svn);
        SetPchPfmRecoverMajorVersion(recovery_major_version);
        SetPchPfmRecoverMinorVersion(recovery_minor_version);

        ufm_svn = get_ufm_svn(manifest, SVN_POLICY_FOR_PCH_FW_UPDATE);
        if (ufm_svn < recovery_svn) {
     	   status = set_ufm_svn(manifest, SVN_POLICY_FOR_PCH_FW_UPDATE,recovery_svn);
        }
    }else if(manifest->image_type == BMC_TYPE){
        SetBmcPfmRecoverSvn(recovery_svn);
        SetBmcPfmRecoverMajorVersion(recovery_major_version);
        SetBmcPfmRecoverMinorVersion(recovery_minor_version);

        ufm_svn = get_ufm_svn(manifest, SVN_POLICY_FOR_BMC_FW_UPDATE);
		if (ufm_svn < recovery_svn) {
			status = set_ufm_svn(manifest, SVN_POLICY_FOR_BMC_FW_UPDATE,recovery_svn);
	    }
    }

    return status;
}

int read_statging_area_pfm(struct pfr_manifest *manifest, uint8_t *svn_version)
{
    int status = 0;
	uint32_t pfm_start_address = 0;
    uint8_t  buffer[sizeof(PFM_STRUCTURE_1)];

    //PFM data start address after Staging block and PFM block
    pfm_start_address = manifest->address + PFM_SIG_BLOCK_SIZE + PFM_SIG_BLOCK_SIZE;
	
	status = pfr_spi_read(manifest->image_type, pfm_start_address, sizeof(PFM_STRUCTURE_1), buffer);
    if(status != Success){
    	DEBUG_PRINTF("Invalid Staging Area Pfm \r\n");
    	return Failure;
    }
	
    *svn_version = ((PFM_STRUCTURE_1 *)buffer)->SVN;

    return Success;
}

int spi_region_hash_verification(struct pfr_manifest *pfr_manifest, PFM_SPI_DEFINITION *PfmSpiDefinition, uint8_t *pfm_spi_Hash) {

	int status = 0;
	uint32_t region_length;

    DEBUG_PRINTF("RegionStartAddress: %x,RegionEndAddress: %x\r\n",
            PfmSpiDefinition->RegionStartAddress, PfmSpiDefinition->RegionEndAddress);
    region_length =(PfmSpiDefinition->RegionEndAddress) - (PfmSpiDefinition->RegionStartAddress);

    if((PfmSpiDefinition->HashAlgorithmInfo.SHA256HashPresent == 1 ) ||
            (PfmSpiDefinition->HashAlgorithmInfo.SHA384HashPresent == 1)){
    	
		uint8_t sha256_buffer[SHA384_DIGEST_LENGTH] = {0};
		uint32_t hash_length = 0;		

		pfr_manifest->pfr_hash->start_address = PfmSpiDefinition->RegionStartAddress;
		pfr_manifest->pfr_hash->length = region_length;
			
        if( PfmSpiDefinition->HashAlgorithmInfo.SHA256HashPresent == 1){
        	pfr_manifest->pfr_hash->type = HASH_TYPE_SHA256;
			hash_length = SHA256_DIGEST_LENGTH;
		} else if(PfmSpiDefinition->HashAlgorithmInfo.SHA384HashPresent ==1 ){
			pfr_manifest->pfr_hash->type = HASH_TYPE_SHA384;
			hash_length = SHA384_DIGEST_LENGTH;
		}else{
			return Failure;
		}

		pfr_manifest->base->get_hash(pfr_manifest,pfr_manifest->hash,sha256_buffer, SHA256_DIGEST_LENGTH);
			
		status = compare_buffer(pfm_spi_Hash, sha256_buffer,SHA256_DIGEST_LENGTH);
        if(status != Success){
			return Failure;
			
		}		
    }
	
	DEBUG_PRINTF("Digest verification success\r\n");

    return Success;
}

int get_fvm_start_address (struct pfr_manifest *manifest, uint32_t *fvm_address) {

	int status = 0;
	int region_count;
	uint32_t position;
	PFM_FVM_ADDRESS_DEFINITION pfm_definition = {0};
	uint8_t pfm_definition_type = PCH_PFM_FVM_ADDRESS_DEFINITION;
	uint8_t *pfm_hash;
	region_count = 0;

	for (position = 0; position <= g_pfm_manifest_length - 1; region_count++){
		status = get_pfm_manifest_data(&position,manifest->image_type, (void *)&pfm_definition, (uint8_t *)&pfm_hash, pfm_definition_type);
		if(status == manifest_failure){
			return manifest_failure;
		} else if (status == manifest_success){
			*fvm_address = pfm_definition.FVMAddress;
			return manifest_success;
		}

	}

	return manifest_failure;
}

int get_fvm_address(struct pfr_manifest *manifest, uint16_t fv_type) {
	
	int status = 0;
	int region_count = 0;
	int position;
	PFM_FVM_ADDRESS_DEFINITION pfm_definition = {0};
	uint8_t pfm_definition_type = PCH_PFM_FVM_ADDRESS_DEFINITION;
	uint8_t *pfm_hash;
	region_count = 0;

	for (position = 0; position <= g_pfm_manifest_length - 1; region_count++){
		status = get_pfm_manifest_data(manifest, &position, (void *)&pfm_definition, (uint8_t *)&pfm_hash, pfm_definition_type);
		if(status == manifest_failure){
			return Failure;
		} else if (status == manifest_unsupported){
			continue;
		}

		if(pfm_definition.FVType == fv_type) {
			return pfm_definition.FVMAddress;
		}

	}

	return 0;
}

int get_spi_region_hash(struct pfr_manifest *manifest, uint32_t address, PFM_SPI_DEFINITION *p_spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition) {
	int status = 0;

	if (p_spi_definition->HashAlgorithmInfo.SHA256HashPresent == 1) {
		if(p_spi_definition->PFMDefinitionType == pfm_definition)
			status = pfr_spi_read(manifest->image_type, address, SHA256_SIZE, pfm_spi_hash);
		
		return SHA256_SIZE;
	} else if (p_spi_definition->HashAlgorithmInfo.SHA384HashPresent == 1) {
		if(p_spi_definition->PFMDefinitionType == pfm_definition)
			status = pfr_spi_read(manifest->image_type, address, SHA384_SIZE, pfm_spi_hash);
		return SHA384_SIZE;
	}

	return 0;
}

void set_protect_level_mask_count(struct pfr_manifest *manifest, PFM_SPI_DEFINITION *spi_definition)
{	
	int status = 0;
	uint8_t local_count = 0;
	if (manifest->image_type == PCH_TYPE){
		if (pch_protect_level_mask_count.Calculated == 1){
			return;
		}else{
			if (spi_definition->ProtectLevelMask.RecoverOnFirstRecovery == 1){
				local_count = 1;
			}else if (spi_definition->ProtectLevelMask.RecoverOnSecondRecovery == 1){
				local_count = 2;
			}else if(spi_definition->ProtectLevelMask.RecoverOnThirdRecovery == 1){
				local_count = 3;
			}
			if (pch_protect_level_mask_count.Count != 0 && pch_protect_level_mask_count.Count > local_count){
				pch_protect_level_mask_count.Count = local_count;
			}else if(pch_protect_level_mask_count.Count == 0){
				pch_protect_level_mask_count.Count = 1;
			}
		}

	}else{
		if (bmc_protect_level_mask_count.Calculated == 1){
			return;
		}else{
			if (spi_definition->ProtectLevelMask.RecoverOnFirstRecovery == 1){
				local_count = 1;
			}else if (spi_definition->ProtectLevelMask.RecoverOnSecondRecovery == 1){
				local_count = 2;
			}else if(spi_definition->ProtectLevelMask.RecoverOnThirdRecovery == 1){
				local_count = 3;
			}
			if (bmc_protect_level_mask_count.Count != 0 && bmc_protect_level_mask_count.Count > local_count){
				bmc_protect_level_mask_count.Count = local_count;
			}else if(bmc_protect_level_mask_count.Count == 0){
				bmc_protect_level_mask_count.Count = 1;
			}
		}

	}
}

Manifest_Status get_pfm_manifest_data(struct pfr_manifest *manifest, uint32_t *position,void *spi_definition, uint8_t *pfm_spi_hash, uint8_t pfm_definition){

	int status = 0;
	static uint32_t manifest_start_address;
	uint32_t read_address = manifest->address;
	uint8_t pfm_definition_type;

	if (*position == 0){
		PFM_STRUCTURE_1 pfm_data;
	       if (manifest->image_type == PCH_TYPE){
	           manifest_start_address = manifest->address + PFM_SIG_BLOCK_SIZE;
	       }else{
	           manifest_start_address = read_address + PFM_SIG_BLOCK_SIZE;
	       }

	       status = pfr_spi_read(manifest->image_type, manifest_start_address, sizeof(PFM_STRUCTURE_1), (uint8_t *)&pfm_data);
	    	if(status != Success)
	    	   	return manifest_failure;

	       g_pfm_manifest_length = pfm_data.Length;
	       g_active_pfm_svn = pfm_data.SVN;
	       manifest_start_address += sizeof(PFM_STRUCTURE_1);
	   }

	   status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(uint8_t), &pfm_definition_type);
		if(status != Success)
			return manifest_failure;
	   
       // DEBUG_PRINTF("PFMDefinitionType %d\r\n", pfm_definition_type);

	   if(pfm_definition_type == ACTIVE_PFM_SMBUS_RULE)
	   {
		   if(pfm_definition_type == pfm_definition){
			   status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(PFM_SMBUS_RULE),(uint8_t *)spi_definition);
		    	if(status != Success)
		    	   	return manifest_failure;
		   }
			*position += sizeof(PFM_SMBUS_RULE);
	   } else if (pfm_definition_type == PCH_PFM_SPI_REGION) {
		   
           DEBUG_PRINTF("PFM Spi Region Detected\n");

		   status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(PFM_SPI_DEFINITION), (uint8_t*) spi_definition);
		   if(status != Success)
			   return manifest_failure;

		   *position += sizeof(PFM_SPI_DEFINITION);

		   *position += get_spi_region_hash(manifest, (uint32_t)(manifest_start_address + *position), (PFM_SPI_DEFINITION *)spi_definition, pfm_spi_hash, pfm_definition);
		   set_protect_level_mask_count(manifest->image_type,spi_definition);
		} else if (pfm_definition_type == PCH_PFM_FVM_ADDRESS_DEFINITION) {

			if (pfm_definition_type == pfm_definition){
				status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(PFM_FVM_ADDRESS_DEFINITION), (uint8_t*) spi_definition);
				if(status != Success)
					return manifest_failure;
			}
			*position += sizeof(PFM_FVM_ADDRESS_DEFINITION);

		} else{
	       *position = g_pfm_manifest_length;
	   }

		if (pfm_definition_type != pfm_definition)
			return manifest_unsupported;

	   return manifest_success;

}

int pfm_spi_region_verification(struct pfr_manifest *manifest)
{	
	int status = 0;
    int region_count,verify_status;
    uint32_t position;
    PFM_SPI_DEFINITION pfm_spi_definition = {0};
    uint8_t pfm_definition_type = PCH_PFM_SPI_REGION;
    uint8_t pfm_spi_hash[SHA384_SIZE] = {0};
	uint8_t fvm_region_count = 0;
    
	region_count = 0;
    
    for (position = 0; position <= g_pfm_manifest_length - 1; region_count++){
    	verify_status = get_pfm_manifest_data(manifest, &position, (void *)&pfm_spi_definition, (uint8_t *)&pfm_spi_hash, pfm_definition_type);
        if(verify_status == manifest_failure){
            DEBUG_PRINTF("Invalid Manifest Data..System Halted !!\r\n");
            return Failure;
        }
		
        if (verify_status == manifest_unsupported){
        	if (pfm_definition_type == PCH_PFM_SPI_REGION)
        		pfm_definition_type = PCH_PFM_FVM_ADDRESS_DEFINITION;
            continue;
        }

        if (pfm_definition_type == PCH_PFM_SPI_REGION) {
        	status = spi_region_hash_verification(manifest, &pfm_spi_definition, pfm_spi_hash);
        	if(status != Success){
        		DEBUG_PRINTF("SPI region hash verification fail...\r\n");
        		return Failure;
        	}

        	memset(&pfm_spi_definition, 0, sizeof(PFM_SPI_DEFINITION));
        }
        memset(pfm_spi_hash, 0, SHA384_SIZE);
    }
    if (manifest->image_type == PCH_TYPE){
        pch_protect_level_mask_count.Calculated = 1;
    }else{
        bmc_protect_level_mask_count.Calculated = 1;
    }

    return Success;
}

Manifest_Status get_fvm_manifest_data(struct pfr_manifest *manifest, uint32_t *position, PFM_FVM_ADDRESS_DEFINITION *p_fvm_address_definition, void *spi_definition, uint8_t *fvm_spi_hash, uint8_t fvm_def_type) {
	
	int status = 0;
	static uint32_t manifest_start_address;
	uint8_t fvm_definition_type;

	if (*position == 0) {

		FVM_STRUCTURE fvm_data;
		manifest_start_address = p_fvm_address_definition->FVMAddress + PFM_SIG_BLOCK_SIZE;

		status = pfr_spi_read(manifest->image_type, manifest_start_address, sizeof(FVM_STRUCTURE), (uint8_t *)&fvm_data);
    	if(status != Success)
    	   	return manifest_failure;

		if(fvm_data.FvmTag == FVMTAG) {
			 DEBUG_PRINTF("FvmTag verification success...\r\n");
		} else{
			DEBUG_PRINTF("FvmTag verification failed...\r\n");
			return manifest_failure;
		}

	   if (fvm_data.FvType != p_fvm_address_definition->FVType) {
		   return manifest_failure;
	   }

		g_fvm_manifest_length = fvm_data.Length;
		*position += sizeof(FVM_STRUCTURE);
	}

	status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(uint8_t), &fvm_definition_type);
	if(status != Success)
	   	return manifest_failure;

	if (fvm_definition_type == PCH_FVM_SPI_REGION) {

		status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(PFM_SPI_DEFINITION), (uint8_t*)spi_definition);
    	if(status != Success)
    	   	return manifest_failure;

		*position += sizeof(PFM_SPI_DEFINITION);

		*position += get_spi_region_Hhash((uint32_t)(manifest_start_address + *position), (PFM_SPI_DEFINITION *)spi_definition, fvm_spi_hash, fvm_def_type);

	} else if (fvm_definition_type == PCH_FVM_Capabilities) {

		if (fvm_definition_type == fvm_def_type){
			status = pfr_spi_read(manifest->image_type, (manifest_start_address + *position), sizeof(FVM_CAPABLITIES), (uint8_t*)spi_definition);
	    	if(status != Success)
	    	   	return manifest_failure;
		}
		*position += sizeof(FVM_CAPABLITIES);

	} else {
		*position = g_fvm_manifest_length;
	}

	if (fvm_def_type != fvm_definition_type)
		return manifest_unsupported;

	return manifest_success;
}
