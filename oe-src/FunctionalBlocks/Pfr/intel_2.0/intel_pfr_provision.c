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
#include "intel_pfr_verification.h" 

#undef DEBUG_PRINTF
#if PFR_AUTHENTICATION_DEBUG
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int g_provision_data;

//Verify Root Key hash
int verify_root_key_hash(struct pfr_manifest *manifest, uint8_t *root_public_key)
{
	int status = 0;
    uint8_t hash_length = 0;
	uint8_t sha_buffer[SHA384_DIGEST_LENGTH] = {0};
	uint8_t ufm_sha_data[SHA384_DIGEST_LENGTH] = {0};

	if(manifest->hash_curve == secp256r1){
		hash_length = (2 * SHA256_HASH_LENGTH) ;
	}else if (manifest->hash_curve == secp384r1){
		hash_length = (2 * SHA384_HASH_LENGTH) ;
	}else{
		return Failure;
	}

	status = get_buffer_hash(manifest, root_public_key, hash_length, sha_buffer);
	if(status != Success)
		return Failure;

	// Read hash from provisoned UFM 0
	status = ufm_read(PROVISION_UFM,ROOT_KEY_HASH, ufm_sha_data, (hash_length / 2));
	if (status != Success)
		return status;

	status = compare_buffer(sha_buffer, ufm_sha_data, (hash_length / 2));
	if(status != Success){
		DEBUG_PRINTF("Root Key hash not matched\r\n");
		return Failure;
	}

	return Success;
}

//Verify Root Key  
int verify_root_key_data(struct pfr_manifest *manifest, uint8_t *pubkey_x, uint8_t *pubkey_y)
{
    int status;
	uint8_t root_public_key[2 * SHA384_DIGEST_LENGTH] = {0};
	uint8_t i = 0;
    uint8_t temp = 0;
	uint8_t digest_length = 0;
	
    if(manifest->hash_curve == secp256r1){
        digest_length = SHA256_DIGEST_LENGTH;
    }else if(manifest->hash_curve == secp384r1){
        digest_length = SHA384_DIGEST_LENGTH;
    }else {
        return Failure;
    }

    // Root Public Key update
	memcpy(&root_public_key[0], pubkey_x,digest_length);
	memcpy(&root_public_key[digest_length], pubkey_y, digest_length);

	//Changing Endianess
	for (i = digest_length; i > digest_length/2; i--) {
		temp = root_public_key[i - 1];
		root_public_key[i -1] = root_public_key[digest_length - i];
		root_public_key[digest_length - i] = temp;
		temp = root_public_key[digest_length + i - 1];
		root_public_key[digest_length + i - 1] = root_public_key[(digest_length - i) + digest_length];
		root_public_key[(digest_length - i) + digest_length] = temp;
	}

    status = verify_root_key_hash(manifest,root_public_key);
    if(status != Success)
        return Failure;

	return Success;
}

//Root Entry Key
int verify_root_key_entry(struct pfr_manifest *manifest, PFR_AUTHENTICATION_BLOCK1 *block1_buffer)
{	
	int status;
    int root_key_permission = 0xFFFFFFFF;    // -1;
    uint8_t i = 0;
    uint8_t temp = 0;

    if(block1_buffer->RootEntry.Tag != BLOCK1_ROOTENTRY_TAG){
		DEBUG_PRINTF("Root Magic/Tag not matched \r\n");
        return Failure;
    }

    //Update CSK curve type to validate Block 0 entry
    if(block1_buffer->RootEntry.PubCurveMagic == PUBLIC_SECP256_TAG)
        manifest->hash_curve = secp256r1;
    else if(block1_buffer->RootEntry.PubCurveMagic == PUBLIC_SECP384_TAG)
        manifest->hash_curve = secp384r1;

    //Key permission
    if(block1_buffer->RootEntry.KeyPermission != root_key_permission){
        DEBUG_PRINTF("Root key permission not matched \r\n");
        return Failure;
    }

    //Key Cancellation
    if(block1_buffer->RootEntry.KeyId != root_key_permission){
        DEBUG_PRINTF("Root key permission not matched \r\n");
        return Failure;
    }

	status = verify_root_key_data(manifest, block1_buffer->RootEntry.PubKeyX, block1_buffer->RootEntry.PubKeyY);
	if(status != Success)
		return Failure;

	return Success;
}
