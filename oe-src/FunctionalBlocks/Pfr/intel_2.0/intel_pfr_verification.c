//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#include <stdint.h>
#include "state_machine/common_smc.h"
#include "pfr/pfr_common.h"
#include "intel_pfr_definitions.h"
#include "pfr/pfr_util.h"
#include "intel_pfr_provision.h"
#include "intel_pfr_key_cancellation.h"
#include "intel_pfr_verification.h"

#undef DEBUG_PRINTF
#if PFR_AUTHENTICATION_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int validate_pc_type(struct pfr_manifest *manifest, uint32_t pc_type){
	
	if(pc_type != manifest->pc_type && manifest->pc_type != PFR_PCH_CPU_Seamless_Update_Capsule)
		return Failure;

	return Success;
}

// Block 1 _ Block 0 Entry
int intel_block1_block0_entry_verify(struct pfr_manifest *manifest)
{
	int status = 0;
	uint32_t block0_entry_address = 0;
	BLOCK0ENTRY *block1_buffer;
	uint8_t block0_signature_curve_magic = 0;
	uint8_t verify_status = 0;
	uint8_t buffer[sizeof(BLOCK0ENTRY)] = {0};
	uint32_t block1_address = manifest->address + sizeof(PFR_AUTHENTICATION_BLOCK0);
	CSKENTRY *block1_csk_buffer;
	uint8_t buffer_csk[sizeof(CSKENTRY)] = {0};	

	//Adjusting BlockAddress in case of KeyCancellation
	if(manifest->kc_flag == 0){
		block0_entry_address = manifest->address + sizeof(PFR_AUTHENTICATION_BLOCK0) + CSK_START_ADDRESS + sizeof(CSKENTRY);
	} else {
		block0_entry_address = manifest->address + sizeof(PFR_AUTHENTICATION_BLOCK0) + CSK_START_ADDRESS;
	}

	status = pfr_spi_read(manifest->image_type, block0_entry_address, sizeof(BLOCK0ENTRY), buffer);
	if(status != Success)
		return Failure;

	block1_buffer = (BLOCK0ENTRY *)&buffer;

	status = pfr_spi_read(manifest->image_type, block1_address + CSK_START_ADDRESS, sizeof(CSKENTRY), buffer_csk);
	if(status != Success)
		return Failure;

	block1_csk_buffer = (CSKENTRY *)&buffer_csk;

	if(block1_buffer->TagBlock0Entry != BLOCK1_BLOCK0ENTRYTAG){
		DEBUG_PRINTF("Block 0 entry Magic/Tag not matched \r\n");
		return Failure;
	}

	if(block1_buffer->Block0SignatureMagic == SIGNATURE_SECP256_TAG)
		block0_signature_curve_magic = secp256r1;
	else if(block1_buffer->Block0SignatureMagic == SIGNATURE_SECP384_TAG){
		block0_signature_curve_magic = secp384r1;
	}

	//Key curve and Block 0 signature curve type should match
	if(block0_signature_curve_magic != manifest->hash_curve){
		DEBUG_PRINTF("Key curve magic and Block0 signature curve magic not matched \r\n");
		return Failure;
	}

	uint8_t signature[2 * SHA384_DIGEST_LENGTH] = {0};
	uint32_t hash_length = 0;

	manifest->pfr_hash->start_address = manifest->address;
	manifest->pfr_hash->length = sizeof(PFR_AUTHENTICATION_BLOCK0);

	if(manifest->hash_curve == secp256r1) {
		manifest->pfr_hash->type = HASH_TYPE_SHA256;
		hash_length = SHA256_HASH_LENGTH;
	}else if(manifest->hash_curve == secp384r1) {
		manifest->pfr_hash->type = HASH_TYPE_SHA384;
		hash_length = SHA384_HASH_LENGTH;
	}else{
		return Failure;
	}

	status = manifest->base->get_hash(manifest, manifest->hash, manifest->pfr_hash->hash_out, hash_length);
	if(status != Success){
		return Failure;
	}
	
	if(manifest->kc_flag == 0){
		memcpy(manifest->verification->pubkey->x, block1_csk_buffer->CskEntryInitial.PubKeyX, sizeof(block1_csk_buffer->CskEntryInitial.PubKeyX));
		memcpy(manifest->verification->pubkey->y, block1_csk_buffer->CskEntryInitial.PubKeyY, sizeof(block1_csk_buffer->CskEntryInitial.PubKeyY));
	}

	memcpy(manifest->verification->pubkey->signature_r, block1_buffer->Block0SignatureR, hash_length);
	memcpy(manifest->verification->pubkey->signature_s, block1_buffer->Block0SignatureS, hash_length);

	status = manifest->verification->base->verify_signature(manifest, manifest->pfr_hash->hash_out, hash_length, signature, (2 * hash_length));
	if(status != Success)
		return Failure;

	DEBUG_PRINTF("Block 0 entry Verification status: %d\r\n",status);

	return Success;
}

// Block 1 CSK 
int intel_block1_csk_block0_entry_verify(struct pfr_manifest *manifest)
{
	int status = 0;
	uint32_t sign_bit_verify = 0;
	uint32_t block1_address = manifest->address + sizeof(PFR_AUTHENTICATION_BLOCK0);
	CSKENTRY *block1_buffer;
	uint8_t csk_sign_curve_magic = 0;
	uint8_t buffer[sizeof(CSKENTRY)] = {0};
	uint8_t csk_key_curve_type = 0;
	uint8_t verify_status = 0;

	status = pfr_spi_read(manifest->image_type, block1_address + CSK_START_ADDRESS, sizeof(CSKENTRY), buffer);
	if(status != Success)
		return Failure;

	block1_buffer = (CSKENTRY *)&buffer;

	//validate CSK entry magic tag
	if(block1_buffer->CskEntryInitial.Tag != BLOCK1CSKTAG){
		DEBUG_PRINTF("CSK Magic/Tag not matched \r\n");
		return Failure;
	}

	if(block1_buffer->CskSignatureMagic == SIGNATURE_SECP256_TAG)
		csk_sign_curve_magic = secp256r1;
	else if(block1_buffer->CskSignatureMagic == SIGNATURE_SECP384_TAG)
		csk_sign_curve_magic = secp384r1;

	// Root key curve and CSK signature curve type should match
	if(csk_sign_curve_magic != manifest->hash_curve){
		DEBUG_PRINTF("Root Key curve magic and CSK key signature curve magic not matched \r\n");
		return Failure;
	}

	//Update CSK curve type to validate Block 0 entry
	if(block1_buffer->CskEntryInitial.PubCurveMagic == PUBLIC_SECP256_TAG)
		csk_key_curve_type = secp256r1;
	else if(block1_buffer->CskEntryInitial.PubCurveMagic == PUBLIC_SECP384_TAG)
		csk_key_curve_type = secp384r1;

	//Key permission
	if(manifest->pc_type == PFR_BMC_UPDATE_CAPSULE)// Bmc update
		sign_bit_verify = SIGN_BMC_UPDATE_BIT3;
	else if(manifest->pc_type == PFR_PCH_UPDATE_CAPSULE)// PCH update
		sign_bit_verify = SIGN_PCH_UPDATE_BIT1;
    else if(manifest->pc_type == PFR_BMC_PFM) // BMC PFM
		sign_bit_verify = SIGN_BMC_PFM_BIT2;
    else if(manifest->pc_type == PFR_PCH_PFM) // PCH PFM
		sign_bit_verify = SIGN_PCH_PFM_BIT0;
    else if(manifest->pc_type == PFR_CPLD_UPDATE_CAPSULE
				|| manifest->pc_type == PFR_CPLD_UPDATE_CAPSULE_DECOMMISSON){
		sign_bit_verify = SIGN_CPLD_UPDATE_BIT4;
	}

	if (!(block1_buffer->CskEntryInitial.KeyPermission & sign_bit_verify)){
	   DEBUG_PRINTF("CSK key permission denied..\r\n");
	   return Failure;
	}
	
	uint8_t signature[2 * SHA384_DIGEST_LENGTH] = {0};
	uint32_t hash_length = 0;
	manifest->pfr_hash->start_address = block1_address+ CSK_START_ADDRESS + sizeof(block1_buffer->CskEntryInitial.Tag);
	manifest->pfr_hash->length = CSK_ENTRY_PC_SIZE;

	if(csk_key_curve_type == secp256r1) {
		manifest->pfr_hash->type = HASH_TYPE_SHA256;
		hash_length = SHA256_DIGEST_LENGTH;
	}else if(csk_key_curve_type == secp384r1) {
		manifest->pfr_hash->type = HASH_TYPE_SHA384;
		hash_length = SHA384_DIGEST_LENGTH;
	}else{
		return Failure;
	}
	
	status = manifest->base->get_hash(manifest, manifest->hash, manifest->pfr_hash->hash_out, hash_length);
	if(status != Success)
		return Failure;

	memcpy(manifest->verification->pubkey->signature_r, block1_buffer->CskSignatureR, hash_length);
	memcpy(manifest->verification->pubkey->signature_s, block1_buffer->CskSignatureS, hash_length);

	// memcpy(&signature[0],&block1_buffer->CskSignatureR[0],hash_length);
	// memcpy(&signature[hash_length],&block1_buffer->CskSignatureS[0],hash_length);

	status = manifest->verification->base->verify_signature(manifest, manifest->pfr_hash->hash_out, hash_length, signature, (2 * hash_length));
	if(status != Success)
		return Failure;

	status = manifest->pfr_authentication->block1_block0_entry_verify(manifest);
	if(status != Success){
		return Failure;
	}
	
	DEBUG_PRINTF("Block0 Entry validation success\r\n");
	
    return Success;
}

//Block 1 
int intel_block1_verify(struct pfr_manifest *manifest)
{
	int status = 0;
	PFR_AUTHENTICATION_BLOCK1 *block1_buffer;
	uint8_t buffer[LENGTH]={0};

	status = pfr_spi_read(manifest->image_type,manifest->address + sizeof(PFR_AUTHENTICATION_BLOCK0), sizeof(block1_buffer->TagBlock1) + sizeof(block1_buffer->ReservedBlock1) + sizeof(block1_buffer->RootEntry), buffer);
	if(status != Success){
		DEBUG_PRINTF("Block1 Verification failed\r\n");
		return Failure;
	}

	block1_buffer = (PFR_AUTHENTICATION_BLOCK1 *)buffer;

	if(block1_buffer->TagBlock1 != BLOCK1TAG){
		DEBUG_PRINTF("Block1 Tag Not Found\r\n");
		return Failure;
	}

	status = verify_root_key_entry(manifest, block1_buffer);
	if(status != Success){
		DEBUG_PRINTF("Root Entry validation failed\r\n");
		return Failure;
	}

	DEBUG_PRINTF("Root Entry validation success\r\n");
	memcpy(manifest->verification->pubkey->x, block1_buffer->RootEntry.PubKeyX, sizeof(block1_buffer->RootEntry.PubKeyX));
	memcpy(manifest->verification->pubkey->y, block1_buffer->RootEntry.PubKeyY, sizeof(block1_buffer->RootEntry.PubKeyY));
	
	if(block1_buffer->RootEntry.PubCurveMagic == PUBLIC_SECP256_TAG)
		manifest->verification->pubkey->length = SHA256_DIGEST_LENGTH;
	else if(block1_buffer->RootEntry.PubCurveMagic == PUBLIC_SECP384_TAG)
		manifest->verification->pubkey->length = SHA384_DIGEST_LENGTH;

	if (manifest->kc_flag == 0){
		//CSK and Block 0 entry verification
		status = manifest->pfr_authentication->block1_csk_block0_entry_verify(manifest);
		if(status != Success){
			DEBUG_PRINTF("CSK and Block0 Entry validation failed\r\n");
			return Failure;
		}
	}else{
		status = manifest->pfr_authentication->block1_block0_entry_verify(manifest);
		if(status != Success){
			DEBUG_PRINTF("Block0 Entry validation failed\r\n");
			return Failure;
		}
	}

	return Success;
}

//BLOCK 0
uint8_t intel_block0_verify(struct pfr_manifest *manifest)
{
	int status = 0;
	uint32_t pc_type_status = 0;
		PFR_AUTHENTICATION_BLOCK0 *block0_buffer;

	uint8_t block0_hash_match = 0;
	uint8_t buffer[sizeof(PFR_AUTHENTICATION_BLOCK0)] = {0};
	uint8_t sha_buffer[SHA384_DIGEST_LENGTH] = {0};

	status = pfr_spi_read(manifest->image_type,manifest->address, sizeof(PFR_AUTHENTICATION_BLOCK0), buffer);
	if(status != Success){
		DEBUG_PRINTF("Block0 Verification failed\r\n");
		return Failure;
	}

	status = pfr_spi_read(manifest->image_type,manifest->address + (2*sizeof(pc_type_status)), sizeof(pc_type_status), (uint8_t *)&pc_type_status);
	if(status != Success){
		DEBUG_PRINTF("Block0 Verification failed\r\n");
		return Failure;
	}

	block0_buffer = (PFR_AUTHENTICATION_BLOCK0 *)buffer;

	// Block0 Hash verify 
	status = get_buffer_hash(manifest, buffer, sizeof(PFR_AUTHENTICATION_BLOCK0), sha_buffer);
	if(status != Success)
		return Success;

	if(manifest->hash_curve == secp256r1)
		status = compare_buffer(manifest->pfr_hash->hash_out, sha_buffer, SHA256_DIGEST_LENGTH);
	else if(manifest->hash_curve == secp384r1)
		status = compare_buffer(manifest->pfr_hash->hash_out, sha_buffer, SHA384_DIGEST_LENGTH);
	else
		return Failure;

	if(status != Success){
		DEBUG_PRINTF("Block0 Hash Not Matched..\r\n");
		return Failure;
	}

	if(block0_buffer->Block0Tag != BLOCK0TAG){
		DEBUG_PRINTF("Block0 tag not found\r\n");
		return Failure;
	}

	if (pc_type_status == DECOMMISSION_CAPSULE) {
		manifest->pc_length = block0_buffer->PcLength;
		return Success;
	}

	uint32_t hash_length = 0;
	uint8_t *ptr_sha;
	memset(sha_buffer, 0x00, sizeof(sha_buffer));

	//Protected content length
	manifest->pc_length = block0_buffer->PcLength;
	manifest->pfr_hash->start_address = manifest->address + PFM_SIG_BLOCK_SIZE;
	manifest->pfr_hash->length = block0_buffer->PcLength;

	if(manifest->hash_curve == secp256r1) {
		manifest->pfr_hash->type = HASH_TYPE_SHA256;
		hash_length = SHA256_DIGEST_LENGTH;
		ptr_sha = block0_buffer->Sha256Pc;
	}else if(manifest->hash_curve == secp384r1) {
		manifest->pfr_hash->type = HASH_TYPE_SHA384;
		hash_length = SHA384_DIGEST_LENGTH;
		ptr_sha = block0_buffer->Sha384Pc;
	}else{
		return Failure;
	}
	
	status = manifest->base->get_hash(manifest, manifest->hash, sha_buffer, hash_length);
	if(status != Success)
		return Failure;

	status = compare_buffer(ptr_sha, sha_buffer, hash_length);
	if(status != Success){
		DEBUG_PRINTF(" Block0 Verification failed..\r\n");
		return Failure;
	}
		
	if (pc_type_status == PFR_CPLD_UPDATE_CAPSULE) {
		SetCpldFpgaRotHash(&sha_buffer[0]);
	}

	DEBUG_PRINTF("Block0 Hash Matched..\r\n");
	return Success;
}

void init_pfr_authentication(struct pfr_authentication *pfr_authentication)
{
	pfr_authentication->validate_pctye = validate_pc_type;
	pfr_authentication->validate_kc = validate_key_cancellation_flag;
	pfr_authentication->block1_verify = intel_block1_verify;
	pfr_authentication->block1_csk_block0_entry_verify = intel_block1_csk_block0_entry_verify;	
	pfr_authentication->block1_block0_entry_verify = intel_block1_block0_entry_verify;
	pfr_authentication->block0_verify = intel_block0_verify;
}

int intel_pfr_manifest_verify(struct manifest *manifest, struct hash_engine *hash,
		struct signature_verification *verification, uint8_t *hash_out, uint32_t hash_length){

	int status = 0;
	uint32_t pc_type = 0;

	struct pfr_manifest *pfr_manifest = (struct pfr_manifest *) manifest;
	init_pfr_authentication(pfr_manifest->pfr_authentication);
	
	status = pfr_spi_read(pfr_manifest->image_type,pfr_manifest->address + BLOCK0_PCTYPE_ADDRESS, sizeof(pc_type), &pc_type);
	if(status != Success)
		return Failure;
	
	//Validate PC type
	status =  pfr_manifest->pfr_authentication->validate_pctye(pfr_manifest, pc_type);
	if(status != Success)
		return Failure;
	
	//Validate Key cancellation
	status =  pfr_manifest->pfr_authentication->validate_kc(pfr_manifest);
	if(status != Success)
		return Failure;

	//Block1verifcation
	status = pfr_manifest->pfr_authentication->block1_verify(pfr_manifest);
	if (status != Success)
		return status;
	
	//Block0Verification
	status = pfr_manifest->pfr_authentication->block0_verify(pfr_manifest);
	if(status != Success)
		return Failure;

	return status;
}
