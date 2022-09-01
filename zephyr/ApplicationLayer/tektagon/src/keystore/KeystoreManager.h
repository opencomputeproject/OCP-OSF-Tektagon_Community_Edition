// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Keystore Handling functions
 */

#ifndef KEYSTOREMANAGER_H_
#define KEYSTOREMANAGER_H_

#include "keystore/keystore.h"
#include "crypto/rsa.h"

#define KeyStoreHdrLen				sizeof(struct Keystore_Header)
#define KeyStoreKeyMaxLen			(KEY_MAX_LENGTH + KeyStoreHdrLen)
#define KeySectionSize				0x1000		// key secton size ,4K
#define FLASH_DEVICE_SIZE			0x1000000	// device size supported
#define KeyStoreOffset_0			0x00		// KeyStore memory offset
#define KEY_MAX_LENGTH  256
#define KEY_MAX_NUMBER 128
#define KeyStoreOffset_200			0x200

struct Keystore_Manager {
    struct keystore base;
};

struct Keystore_Header
{
	uint8_t key_id;
	uint16_t key_length;
}__attribute__((packed));

struct Keystore_Package
{
	struct Keystore_Header keysotre_hdr;
	uint8_t key_buffer[KEY_MAX_LENGTH];
};



int keystoreManager_init (struct Keystore_Manager *key_store);
int keystore_get_key_id(struct keystore *store, uint8_t *key, int *key_id, int *last_key_id);
int keystore_get_root_key(struct rsa_public_key *pub_key);
int keystore_save_root_key(struct rsa_public_key *pub_key);
#endif
