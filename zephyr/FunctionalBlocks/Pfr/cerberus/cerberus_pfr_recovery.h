//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef CERBERUS_PFR_RECOVERY_H_
#define CERBERUS_PFR_RECOVERY_H_
#define DUAL_SPI 0

#include <stdint.h>
#include "manifest/pfm/pfm_manager.h"

struct recovery_header{
	uint16_t header_length;
	uint16_t format;
	uint32_t magic_number;
	uint8_t version_id[32];
	uint32_t image_length;
	uint32_t sign_length;
};

struct recovery_section{
	uint16_t header_length;
	uint16_t format;
	uint32_t magic_number;
	uint32_t start_addr;
	uint32_t section_length;
};

int pfr_recovery_verify(struct recovery_image *image, struct hash_engine *hash,
			      struct signature_verification *verification, uint8_t *hash_out, size_t hash_length,
			      struct pfm_manager *pfm);

#endif
