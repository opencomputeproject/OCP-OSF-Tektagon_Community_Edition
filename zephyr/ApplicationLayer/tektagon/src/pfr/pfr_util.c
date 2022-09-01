//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

//#include "pfr_util.h"
#include "CommonFlash/CommonFlash.h"
#include "flash/flash_util.h"
#include "state_machine/common_smc.h"
#include "pfr_common.h"
#include <sys/reboot.h>
#include <crypto/ecdsa_structs.h>
#include <crypto/ecdsa.h>
#include "mbedtls/ecdsa.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_INTEL_PFR_SUPPORT
#include "intel_2.0/intel_pfr_definitions.h"
#endif
#ifdef CONFIG_CERBERUS_PFR_SUPPORT
#include "cerberus/cerberus_pfr_definitions.h"
#endif

#undef DEBUG_PRINTF
#if 1
#define DEBUG_PRINTF printk
#else
#define DEBUG_PRINTF(...)
#endif

int pfr_spi_read(unsigned int device_id, unsigned int address, unsigned int data_length, unsigned char *data)
{
	int status = 0;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = device_id; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1
	spi_flash->spi.base.read(&spi_flash->spi,address,data,data_length);
	return Success;
}

int pfr_spi_write(unsigned int device_id, unsigned int address, unsigned int data_length, unsigned char *data)
{
	int status = 0;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = device_id; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1
	spi_flash->spi.base.write(&spi_flash->spi,address,data,data_length);
	return Success;
}

int pfr_spi_erase_4k(unsigned int device_id, unsigned int address){
	int status = 0;
	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = device_id; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1
	spi_flash->spi.base.sector_erase(&spi_flash->spi,address);
	return Success;
}

int pfr_spi_page_read_write(unsigned int device_id, uint32_t *source_address,uint32_t *target_address)
{
	int status = 0;
	int index1,index2;
	uint8_t buffer[MAX_READ_SIZE] = {0};

	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	spi_flash->spi.device_id[0] = device_id; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1

	for (index1 = 0; index1 < (PAGE_SIZE / MAX_READ_SIZE); index1 ++){
		spi_flash->spi.base.read(&spi_flash->spi,*source_address, buffer, MAX_READ_SIZE);
		for (index2 = 0; index2 < (MAX_READ_SIZE / MAX_WRITE_SIZE);index2 ++){
			spi_flash->spi.base.write(&spi_flash->spi,*target_address,&buffer[index2 * MAX_WRITE_SIZE], MAX_WRITE_SIZE);
			*target_address += MAX_WRITE_SIZE;
		}

		*source_address += MAX_READ_SIZE;
	}

	return Success;
}

int pfr_spi_page_read_write_between_spi(int source_flash, uint32_t *source_address, int target_flash, uint32_t *target_address)
{	
	int status = 0;
	uint32_t index1, index2;
	uint8_t buffer[MAX_READ_SIZE];
	
	struct SpiEngine *spi_flash = getSpiEngineWrapper();


	for (index1 = 0; index1 < (PAGE_SIZE / MAX_READ_SIZE); index1 ++){
		spi_flash->spi.device_id[0] = source_flash; // assign the flash device id,  0:spi1_cs0, 1:spi2_cs0 , 2:spi2_cs1, 3:spi2_cs2, 4:fmc_cs0, 5:fmc_cs1
		spi_flash->spi.base.read(&spi_flash->spi,*source_address, buffer, MAX_READ_SIZE);
	
		for (index2 = 0; index2 < (MAX_READ_SIZE / MAX_WRITE_SIZE);index2 ++){
			spi_flash->spi.device_id[0] = target_flash; 
			spi_flash->spi.base.write(&spi_flash->spi,*target_address,&buffer[index2 * MAX_WRITE_SIZE], MAX_WRITE_SIZE);
	
			*target_address += MAX_WRITE_SIZE;
		}

		*source_address += MAX_READ_SIZE;
	}
	
	return Success;
}

// calculates sha for dataBuffer
int get_buffer_hash(struct pfr_manifest *manifest, uint8_t *data_buffer, uint8_t length, unsigned char *hash_out) {

	struct SpiEngine *spi_flash = getSpiEngineWrapper();
	
	if(manifest->hash_curve == secp256r1) {
		manifest->hash->start_sha256(manifest->hash); 
		manifest->hash->calculate_sha256 (manifest->hash,data_buffer, length, hash_out, SHA256_HASH_LENGTH);
	}else if(manifest->hash_curve == secp384r1) {
	}else{
		return Failure;
	}

	return Success;
}

int esb_ecdsa_verify(struct pfr_manifest *manifest, unsigned int digest[], unsigned char pub_key[], 
							unsigned char signature[], unsigned char *auth_pass){
	*auth_pass = true;
	
	return Success;
}

// Calculate hash digest
int get_hash(struct manifest *manifest, struct hash_engine *hash_engine, uint8_t *hash_out, size_t hash_length){
	int status = 0;

	struct pfr_manifest *pfr_manifest = (struct pfr_manifest *)manifest;
	
	if(pfr_manifest == NULL || hash_engine == NULL ||
		hash_out == NULL || hash_length < SHA256_HASH_LENGTH ||
		(hash_length > SHA256_HASH_LENGTH && hash_length < SHA384_HASH_LENGTH))
		return Failure;
	flash_hash_contents(pfr_manifest->flash, pfr_manifest->pfr_hash->start_address, pfr_manifest->pfr_hash->length, pfr_manifest->hash, pfr_manifest->pfr_hash->type, hash_out, hash_length);
	
	return Success;
}

//print buffer
void print_buffer(uint8_t *string, uint8_t *buffer, uint32_t length){
	
	DEBUG_PRINTF("\r\n%s ", string);
	
	for(int i = 0; i < length; i++){
		DEBUG_PRINTF(" %x", buffer[i]);
	}
	
	DEBUG_PRINTF("\r\n");	
}

//compare buffer
int compare_buffer(uint8_t *buffer1, uint8_t *buffer2, uint32_t length){
	int status = Success;
	for(int i = 0; i < length; i++){
		if((buffer1[i] != buffer2[i]) && (status != Failure))
			status = Failure;
	}
	return status;
}

// reverse byte array
void reverse_byte_array(uint8_t *data_buffer, uint32_t length){
	uint8_t temp = 0; 
	for(int i = 0, j = length; i < length / 2 ; i++, j--){
		temp = data_buffer[i];
		data_buffer[i] = data_buffer[j];
		data_buffer[j] = temp;
	}
}

static int mbedtls_ecdsa_verify_middlelayer(struct pfr_pubkey *pubkey,
				const uint8_t *digest, uint8_t *signature_r,
				uint8_t *signature_s)
{
	mbedtls_ecdsa_context ctx_verify;
	mbedtls_mpi r, s;
	unsigned char hash[SHA256_HASH_LENGTH];
	int ret = 0;
	char z = 1;

	mbedtls_ecdsa_init(&ctx_verify);
	mbedtls_mpi_init(&r);
	mbedtls_mpi_init(&s);

	// print_buffer("ECDSA X: ", pubkey->x, pubkey->length);
	// print_buffer("ECDSA Y: ", pubkey->y, pubkey->length);
	// print_buffer("ECDSA R: ", signature_r, pubkey->length);
	// print_buffer("ECDSA S: ", signature_s, pubkey->length);
	// print_buffer("ECDSA D: ", digest, pubkey->length);

	mbedtls_mpi_read_binary(&ctx_verify.Q.X, pubkey->x, pubkey->length/*SHA256_HASH_LENGTH*/);

	mbedtls_mpi_read_binary(&ctx_verify.Q.Y, pubkey->y, pubkey->length/*SHA256_HASH_LENGTH*/);

	mbedtls_mpi_read_binary(&ctx_verify.Q.Z, &z, 1);

	mbedtls_mpi_read_binary(&r, signature_r, pubkey->length /*SHA256_HASH_LENGTH*/);

	mbedtls_mpi_read_binary(&s, signature_s, pubkey->length /*SHA256_HASH_LENGTH*/);

	mbedtls_ecp_group_load(&ctx_verify.grp, MBEDTLS_ECP_DP_SECP256R1);
	memcpy(hash, digest, pubkey->length /*SHA256_HASH_LENGTH*/);
	ret = mbedtls_ecdsa_verify(&ctx_verify.grp, hash, SHA256_HASH_LENGTH,
							   &ctx_verify.Q, &r, &s);

	mbedtls_ecdsa_free(&ctx_verify);
	mbedtls_mpi_free(&r);
	mbedtls_mpi_free(&s);
	
	// printk("ECDSA:%d\r\n", ret);

	return ret;

}

/**
 * Verify that a calculated digest matches a signature.
 *
 * @param verification The verification context to use for checking the signature.
 * @param digest The digest to verify.
 * @param digest_length The length of the digest.
 * @param signature The signature to compare against the digest.
 * @param sig_length The length of the signature.
 *
 * @return 0 if the digest matches the signature or an error code.
 */
int verify_signature(struct signature_verification *verification, const uint8_t *digest,
		size_t length, const uint8_t *signature, size_t sig_length){
	int status = Success;

	struct pfr_manifest *manifest = (struct pfr_manifest *)verification;
	// uint8_t signature_r[SHA256_HASH_LENGTH];
	// uint8_t signature_s[SHA256_HASH_LENGTH];
	// memcpy(&signature_r[0],&signature[0],length);
	// memcpy(&signature_s[0],&signature[length],length);

	status =  mbedtls_ecdsa_verify_middlelayer(manifest->verification->pubkey,
														digest,
														manifest->verification->pubkey->signature_r,
														manifest->verification->pubkey->signature_s);

	return status;
}


int pfr_cpld_update_reboot (void)
{
	DEBUG_PRINTF("system going reboot ...\n");

#if (CONFIG_KERNEL_SHELL_REBOOT_DELAY > 0)
	k_sleep(K_MSEC(CONFIG_KERNEL_SHELL_REBOOT_DELAY));
#endif

	sys_reboot(SYS_REBOOT_COLD);

	CODE_UNREACHABLE;

	return -1;
}