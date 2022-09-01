//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

#ifndef CERBERUS_PFR_UPDATE_H_
#define CERBERUS_PFR_UPDATE_H_

#include <stdint.h>

int cerberus_pfr_update_verify(struct firmware_image *fw, struct hash_engine *hash, struct rsa_engine *rsa);

#endif /*INTEL_PFR_UPDATE_H_*/
