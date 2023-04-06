//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2020, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************

#include <stdlib.h>
#include <string.h>
#include <crypto/rsa.h>
#include "platform.h"
#include "RsaWrapper.h"
#include <crypto/rsa_aspeed.h>

int RsaWrapperDecrypt(const struct rsa_private_key *Key,
		const uint8_t *Encrypted, size_t InLength, const uint8_t *Label, size_t LabelLength,
		enum hash_type PadHash, uint8_t *Decrypted, size_t OutLength)
{
    return decrypt_aspeed(Key,Encrypted,InLength, Decrypted, OutLength);
}

int RsaWrapperSigVerify(const struct rsa_public_key *Key,
		const uint8_t *Signature, size_t SigLength, const uint8_t *Match, size_t MatchLength)
{
	struct rsa_key DriverKey;
	DriverKey.m = Key->modulus;//&test;
	DriverKey.m_bits = Key->mod_length * 8;
	DriverKey.e = &Key->exponent;
	DriverKey.e_bits = 24;
	DriverKey.d = NULL;
	DriverKey.d_bits =  0;
	
    return sig_verify_aspeed(&DriverKey, Signature, SigLength, Match, MatchLength);
}

int RsaWrapperGenerateKey(struct rsa_private_key *Key, int Bits)
{
	return 0;
}

int RsaWrapperInitPrivateKey(struct rsa_private_key *Key,
		const uint8_t *Der, size_t Length)
{
	return 0;
}

int RsaWrapperInitPublicKey(struct rsa_private_key *Key,
		const uint8_t *Der, size_t Length)
{
	return 0;
}

int RsaWrapperReleaseKey(struct rsa_private_key *Key)
{
	return 0;
}

int RsaWrapperGetPrivateKeyDer( const struct rsa_private_key *Key,
		uint8_t **Der, size_t *Length)
{
	return 0;
}

int RsaWrapperGetPublicKeyDer( const struct rsa_private_key *Key,
		uint8_t **Der, size_t *Length)
{
	return 0;
}
