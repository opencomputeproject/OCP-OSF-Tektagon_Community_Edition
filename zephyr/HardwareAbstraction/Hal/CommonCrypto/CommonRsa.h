//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file Rsa.h
 * This file contains the Crypto Handling functions
 */
#include <crypto/rsa.h>

int RsaInit (struct rsa_engine *Engine);
int RsaSigVerify(struct rsa_engine *Engine, const struct rsa_public_key *Key,
		const uint8_t *Signature, size_t SigLength, const uint8_t *Match, size_t MatchLength);