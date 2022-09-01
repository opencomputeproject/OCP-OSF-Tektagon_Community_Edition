// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the KeyStore functions
 */

#include "KeystoreManager.h"
#include "Definition.h"
#include <Common.h>
#include "state_machine/common_smc.h"
#include <storage/flash_map.h>
#include "pfr/pfr_ufm.h"
#include "pfr/pfr_util.h"

int keystore_save_key(struct keystore *store, int id, const uint8_t *key, size_t length)
{
    int status = 0;    
   	uint16_t StoreBufIndex;
	uint32_t BaseAddr;	
	uint8_t StoreBuf[KeyStoreKeyMaxLen] = { 0 };
	uint8_t Read_key; 

    if (length > KEY_MAX_LENGTH) {
        status = KEYSTORE_KEY_TOO_LONG;
        return status;
    }
    BaseAddr = id * KeyStoreKeyMaxLen;
    if (BaseAddr > (FLASH_AREA_SIZE(key) - KEY_MAX_LENGTH)) {
        status = KEYSTORE_INSUFFICIENT_STORAGE;
        return status;
    }
   	//store key ID
	StoreBufIndex = 0;
	StoreBuf[StoreBufIndex] = id;

	//store key length low byte
	StoreBufIndex = 1;
	StoreBuf[StoreBufIndex] = (length & 0xFF);

	//store key length high byte
	StoreBufIndex = 2;
	StoreBuf[StoreBufIndex] = ((length >> 8) & 0xFF);

	//store key data to buffer
	for(StoreBufIndex = KeyStoreHdrLen; StoreBufIndex < (length + KeyStoreHdrLen);StoreBufIndex++)
	{
		//for buffer ,the key should be store after header
		//for input key, the index should be start from 0
		StoreBuf[StoreBufIndex] = key[StoreBufIndex- KeyStoreHdrLen];
	}
    struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_KEY; // Internal UFM SPI
	
	status = spi_flash->spi.base.write(&spi_flash->spi, BaseAddr, &StoreBuf, KeyStoreKeyMaxLen);
	
	if(status != KeyStoreKeyMaxLen)
	{
        printk("key write error \n");
		status = KEYSTORE_SAVE_FAILED;
	}
	else
	{		
        printk("key write success \n");
		status = Success;
	}

    return status;
}

int keystore_load_key(struct keystore *store, int id, uint8_t **key, size_t *length)
{
    uint32_t BaseAddr;
	uint8_t  StoreBuf[KeyStoreHdrLen] ={0};
	uint16_t StoreBufIndex;
	uint16_t StoreBufLen;
	struct Keystore_Header *KeyStorePkgHdr;
	int status;

	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_KEY; // Internal UFM SPI
	BaseAddr = id * KeyStoreKeyMaxLen;

	status = spi_flash->spi.base.read(&spi_flash->spi, BaseAddr, &StoreBuf[0],KeyStoreHdrLen);
	
	if (status != Success) {
		printk("KeyStore_Load_key load header fail ;Flash read status= %x\n",status);
		return status;	// failed to load key header from SPI
	}

	KeyStorePkgHdr = (struct Keystore_Header *)&StoreBuf[0];

	if ((KeyStorePkgHdr->key_length == 0xFF) && (KeyStorePkgHdr->key_id == 0xFF)) {
		status = KEYSTORE_NO_KEY;
		return status;
	}

	*length = KeyStorePkgHdr->key_length;
	StoreBufLen = KeyStorePkgHdr->key_length;

	//store key from flash part
	status = spi_flash->spi.base.read(&spi_flash->spi, BaseAddr + KeyStoreHdrLen, key,StoreBufLen);
	if (status != Success) {
		printk("KeyStore_Load_key load key fail ;Flash read status= %x\n",status);
		status = KEYSTORE_LOAD_FAILED;
	}

	return status;
}

int keystore_erase_key(struct keystore *store, int id)
{
    uint8_t  StoreBuf[KeySectionSize] ={0};
	uint32_t BaseAddr;
	uint32_t WipeOutIndex;
	int status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_KEY; // Internal UFM SPI

	BaseAddr = KeyStoreOffset_0;	

	status = spi_flash->spi.base.read(&spi_flash->spi, BaseAddr, &StoreBuf,KeySectionSize);
	if(status)
	{
		printk("KeyStore_Erase_key load key section fail ;Flash read status= %x\n",status);
		return status;	// failed to load key header from SPI
	}
	status = spi_flash->spi.base.sector_erase(&spi_flash->spi, BaseAddr);

	if(status)
	{
		printk("KeyStore_Erase_key key section erase fail ;Flash erase status= %x\n",status);
		return status;	// failed to load key header from SPI
	}

	WipeOutIndex = id * KeyStoreKeyMaxLen;

	memset(&StoreBuf[WipeOutIndex], 0xff, KeyStoreKeyMaxLen);

	status = spi_flash->spi.base.write(&spi_flash->spi, BaseAddr, &StoreBuf[0],KeySectionSize);
	if (status != Success) {
		//Spi write suppose to return write Length
		printk("KeyStore_Erase_key buffer store fail; write status= %x\n",status);
		status = KEYSTORE_SAVE_FAILED;
	} else {
		status = 0;
	}

    return status;
}

int keystore_erase_all_keys(struct keystore *store)
{
    uint32_t BaseAddr;
	int status;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = ROT_INTERNAL_KEY; // Internal UFM SPI
	BaseAddr = KeyStoreOffset_0;
	
	status = spi_flash->spi.base.sector_erase(&spi_flash->spi, BaseAddr);

	if (status != Success) {
		printk("KeyStore_Erase_All_Keys key section erase fail ;Flash erase status= %x\n",status);
	}

	return status;
}


int keystoreManager_init (struct Keystore_Manager *key_store)
{    
	if (key_store == NULL){
		return KEYSTORE_INVALID_ARGUMENT;
	}

	memset (key_store, 0, sizeof (struct keystore));

    key_store->base.save_key = keystore_save_key;
	key_store->base.load_key = keystore_load_key;
	key_store->base.erase_key = keystore_erase_key;
	key_store->base.erase_all_keys = keystore_erase_all_keys;

	return 0;
}

int keystore_get_key_id(struct keystore *store, uint8_t *key, int *key_id, int *last_key_id)
{
    uint32_t BaseAddr;
	uint8_t  StoreBuf[KeyStoreHdrLen] ={0};
	uint16_t StoreBufIndex;
	uint16_t StoreBufLen;
	struct Keystore_Header *KeyStorePkgHdr;
	int status;
	int id = 0;
	uint8_t key_buffer[KEY_MAX_LENGTH];
	int key_match = 0;

	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_KEY; // Internal UFM SPI
	
	while (key_match == 0 && id < KEY_MAX_NUMBER) {
		BaseAddr = id * KeyStoreKeyMaxLen;

		status = spi_flash->spi.base.read(&spi_flash->spi, BaseAddr, &StoreBuf[0],KeyStoreHdrLen);
		
		if (status != Success) {
			printk("KeyStore_Load_key load header fail ;Flash read status= %x\n",status);
			return status;	// failed to load key header from SPI
		}

		KeyStorePkgHdr = (struct Keystore_Header *)&StoreBuf[0];
		if ((KeyStorePkgHdr->key_length == 0xFFFF) && (KeyStorePkgHdr->key_id == 0xFF)) {
			status = KEYSTORE_NO_KEY;
			return status;
		}
		
		StoreBufLen = KeyStorePkgHdr->key_length;
		//store key from flash part
		status = spi_flash->spi.base.read(&spi_flash->spi, BaseAddr + KeyStoreHdrLen, &key_buffer, StoreBufLen);
		if (status != Success) {
			printk("KeyStore_Load_key load key fail ;Flash read status= %x\n",status);
			status = KEYSTORE_LOAD_FAILED;
		} else {
			// Compare key and key_buffer
			status = compare_buffer(key, &key_buffer, KEY_MAX_LENGTH);
			if (status == Success) {
				key_match = 1;
				*key_id = KeyStorePkgHdr->key_id;
			} else {
				*last_key_id = id;
			}
		}
		id++;
	}	
	return status;
}

int keystore_save_root_key(struct rsa_public_key *pub_key)
{
    int status = 0;
	uint16_t StoreBufIndex;
	uint16_t BaseAddr = KeyStoreOffset_200;	
	int rootkey_contain_size = 2 + pub_key->mod_length + sizeof(pub_key->exponent);
	uint8_t StoreBuf[KEY_MAX_LENGTH + 2 + sizeof(pub_key->exponent) ] = { 0 };


    if (pub_key->mod_length > KEY_MAX_LENGTH) {
        status = KEYSTORE_KEY_TOO_LONG;
        return status;
    }
    
	//store key length low byte
	StoreBufIndex = 0;
	StoreBuf[StoreBufIndex] = (pub_key->mod_length & 0xFF);

	//store key length high byte
	StoreBufIndex = 1;
	StoreBuf[StoreBufIndex] = ((pub_key->mod_length >> 8) & 0xFF);

	//store key data to buffer
	for(StoreBufIndex = 2; StoreBufIndex < (pub_key->mod_length + 2);StoreBufIndex++)
	{
		//for buffer ,the key should be store after header
		//for input key, the index should be start from 0
		StoreBuf[StoreBufIndex] = pub_key->modulus[StoreBufIndex- 2];
	}
	// store exponent
	StoreBufIndex = pub_key->mod_length + 2;
	StoreBuf[StoreBufIndex] = (pub_key->exponent & 0xFF);

	StoreBufIndex = pub_key->mod_length + 3;
	StoreBuf[StoreBufIndex] = ((pub_key->exponent >> 8) & 0xFF);

	StoreBufIndex = pub_key->mod_length + 4;
	StoreBuf[StoreBufIndex] = ((pub_key->exponent >> 16) & 0xFF);

	StoreBufIndex = pub_key->mod_length + 5;
	StoreBuf[StoreBufIndex] = ((pub_key->exponent >> 24) & 0xFF);

    struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE; // Root Key save to Intel State
	
	status = spi_flash->spi.base.write(&spi_flash->spi, BaseAddr, StoreBuf, rootkey_contain_size);
	
	if(status != rootkey_contain_size)
	{
        printk("key write error \n");
		status = KEYSTORE_SAVE_FAILED;
	}
	else
	{		
        printk("key write success \n");
		status = Success;
	}

    return status;
}

int keystore_get_root_key(struct rsa_public_key *pub_key)
{
    int status = Success;
	uint16_t BaseAddr = KeyStoreOffset_200;

    uint16_t key_length;
    uint8_t exponent_length;
    uint32_t modules_address,exponent_address;

	struct SpiEngine *spi_flash = getSpiEngineWrapper();

	spi_flash->spi.device_id[0] = ROT_INTERNAL_INTEL_STATE; // Root Key save to Intel State

    //Key Length
    status = spi_flash->spi.base.read(&spi_flash->spi, BaseAddr, &key_length, sizeof(key_length));
    if (status != Success){
        return Failure;
    }	
	pub_key->mod_length = key_length;
    modules_address = BaseAddr + sizeof(key_length);
    //rsa_key_module
    status = spi_flash->spi.base.read(&spi_flash->spi, modules_address, pub_key->modulus, key_length);

    pub_key->mod_length = key_length;
    exponent_address = BaseAddr + sizeof(key_length) + key_length;
    
    //rsa_key_exponent
    status = spi_flash->spi.base.read(&spi_flash->spi, exponent_address, &pub_key->exponent, sizeof(pub_key->exponent));

    return status;
}
