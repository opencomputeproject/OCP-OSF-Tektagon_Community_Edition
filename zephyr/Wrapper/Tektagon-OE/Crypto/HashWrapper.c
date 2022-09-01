//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2020, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************

#include "HashWrapper.h"
#include <crypto/hash.h>
#include <crypto/hash_aspeed.h>

/**
*	Function to Hash Engine Calculate Sha256.
*/
int HashEngineCalculateSha256 (const char *Data, size_t Length, char *Hash, size_t HashLength)
{
	enum hash_algo shaAlgo = HASH_SHA256;
	
	return hash_engine_sha_calculate(shaAlgo, Data, Length, Hash, HashLength);
}

/**
*	Function to Hash Engine Start Sha256.
*/
int HashEngineStartSha256(void)
{
    enum hash_algo shaAlgo = HASH_SHA256;

    return hash_engine_start(shaAlgo);
}

/**
*	Function to Hash Engine Calculate Sha384
*/
int HashEngineCalculateSha384 (const char *Data, size_t Length, char *Hash, size_t HashLength)
{
	enum hash_algo shaAlgo = HASH_SHA384;

    return hash_engine_sha_calculate(shaAlgo, Data, Length, Hash, HashLength);
}
/**
*	Function to Hash Engine Start Sha384
*/
int HashEngineStartSha384(void)
{
	enum hash_algo shaAlgo = HASH_SHA384;

    return hash_engine_start(shaAlgo);
}
/**
*	Function to Hash Engine Update
*/
int HashEngineUpdate (const char *Data, size_t Length)
{
	return hash_engine_update(Data, Length);
}

/**
*	Function to Hash Engine Finish.
*/
int HashEngineFinish (char *Hash, size_t HashLength)
{	
	return hash_engine_finish(Hash, HashLength);
}
/**
*	Function to Hash Engine Cancel.
*/
void HashEngineCancel(void)
{
	hash_engine_cancel();
}
