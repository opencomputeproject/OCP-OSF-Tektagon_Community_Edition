//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Signature Wrapper Handling functions
 */

#ifndef SIGNATURE_VERIFICATION_RSA_H_
#define SIGNATURE_VERIFICATION_RSA_H_

#include "common/signature_verification.h"
#include "crypto/rsa.h"


/**
 * Verification implementation to verify RSA signatures.
 */
struct signature_verification_rsa_wrapper {
	struct signature_verification base;		/**< Base verification instance. */
	struct rsa_engine *rsa;					/**< RSA engine to use for verification. */
	const struct rsa_public_key *key;		/**< Public key for signature verification. */
};


int signature_verification_rsa_wrapper_init (struct signature_verification_rsa_wrapper *verification,
	struct rsa_engine *rsa, const struct rsa_public_key *key);
void signature_verification_rsa_wrapper_release (struct signature_verification_rsa_wrapper *verification);


#endif /* SIGNATURE_VERIFICATION_RSA_H_ */
