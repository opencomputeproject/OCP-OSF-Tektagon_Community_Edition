//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2020, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************

#include "CommonRsa.h"
#include <stdio.h>
#include <string.h>
#include "platform.h"

int RsaGenerateKey (struct rsa_engine *Engine, struct rsa_private_key *Key, int Bits)
{
	printf("RsaGenerateKey\r\n");
	
    return RsaWrapperGenerateKey(Key,Bits);
}

int RsaInitPrivateKey (struct rsa_engine *Engine, struct rsa_private_key *Key,
		const uint8_t *Der, size_t Length)
{
	printf("RsaInitPrivateKey\r\n");
	
    return RsaWrapperInitPrivateKey(Key, Der, Length);
}

int RsaInitPublicKey(struct rsa_engine *Engine, struct rsa_public_key *Key,
    const uint8_t *Der, size_t Length)
{
	printf("RsaInitPublicKey\r\n");
	
    return RsaWrapperInitPublicKey(Key, Der, Length);
}

int RsaReleaseKey(struct rsa_engine *Engine, struct rsa_private_key *Key)
{
	printf("RsaInitPublicKey\r\n");
	
	return RsaWrapperReleaseKey(Key);
}

int RsaGetPrivateKeyDer(struct rsa_engine *Engine, const struct rsa_private_key *Key,
		uint8_t **Der, size_t *Length)
{
	printf("RsaWrapperGetPrivateKeyDer\r\n");
	
    return RsaWrapperGetPrivateKeyDer(Key,Der,Length);
}

int RsaGetPublicKeyDer(struct rsa_engine *Engine, const struct rsa_private_key *Key,
		uint8_t **Der, size_t *Length)
{
	printf("RsaGetPublicKeyDer\r\n");
	
    return RsaWrapperGetPublicKeyDer(Key,Der,Length);
}

int RsaDecrypt(struct rsa_engine *Engine, const struct rsa_private_key *Key,
		const uint8_t *Encrypted, size_t InLength, const uint8_t *Label, size_t LabelLength,
		enum hash_type PadHash, uint8_t *Decrypted, size_t OutLength)
{
	printf("RsaDecrypt\r\n");
	
    return RsaWrapperDecrypt(Key,Encrypted,InLength, Decrypted, OutLength);
}


int RsaSigVerify(struct rsa_engine *Engine, const struct rsa_public_key *Key,
		const uint8_t *Signature, size_t SigLength, const uint8_t *Match, size_t MatchLength)
{
	printf("RsaWrapperSigVerify\r\n");

    return RsaWrapperSigVerify(Key, Signature, SigLength, Match, MatchLength);
}

/**
 * Initialize an aspeed RSA Engine.
 *
 * @param Engine The RSA Engine to initialize.
 *
 * @return 0 if the RSA Engine was successfully initialize or an error code.
 */
int RsaInit (struct rsa_engine *Engine)
{
	if (Engine == NULL) {
		return RSA_ENGINE_INVALID_ARGUMENT;
	}

	memset (Engine, 0, sizeof (struct rsa_engine));
#ifdef ECC_ENABLE_GENERATE_KEY_PAIR
	Engine->generate_key = RsaGenerateKey;
	Engine->init_private_key = RsaInitPrivateKey;
	Engine->init_public_key = RsaInitPublicKey;
	Engine->release_key = RsaReleaseKey;
	Engine->get_private_key_der = RsaGetPrivateKeyDer;
	Engine->get_public_key_der = RsaGetPublicKeyDer;
	Engine->decrypt = RsaDecrypt;
	Engine->sig_verify = RsaSigVerify;
#endif
	return 0;
}
