//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef PFR_UTIL_H
#define PFR_UTIL_H

#include <stdint.h>

int pfr_spi_read(unsigned int device_id,unsigned int address,
						unsigned int data_length, unsigned char *data);

int pfr_spi_write(unsigned int device_id,unsigned int address,
						unsigned int data_length, unsigned char *data);

int pfr_spi_page_read_write(unsigned int device_id, uint32_t *source_address,uint32_t *target_address);

int pfr_spi_erase_4k(unsigned int device_id,unsigned int address);

int esb_ecdsa_verify(struct pfr_manifest *manifest, unsigned int digest[], unsigned char pub_key[], 
							unsigned char signature[], unsigned char *auth_pass);

int get_buffer_hash(struct pfr_manifest *manifest,uint8_t *data_buffer, uint8_t length, unsigned char *hash_out);

int get_hash(struct manifest *manifest, struct hash_engine *hash_engine, uint8_t *hash_out,
	size_t hash_length);

void print_buffer(uint8_t *string,uint8_t *buffer, uint32_t length);

int compare_buffer(uint8_t *buffer1, uint8_t *buffer2, uint32_t length);

void reverse_byte_array(uint8_t *data_buffer, uint32_t length);

int verify_signature(struct signature_verification *verification, const uint8_t *digest,
		size_t length, const uint8_t *signature, size_t sig_length);

int pfr_cpld_update_reboot (void);

#endif /*PFR_UTIL_H*/
