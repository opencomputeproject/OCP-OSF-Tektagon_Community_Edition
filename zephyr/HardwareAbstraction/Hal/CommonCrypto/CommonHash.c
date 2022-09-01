//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file Hash.c
 * This file contains the Crypto Handling functions
 */

#include "CommonHash.h"
#include <crypto/hash.h>

static int HashCalculateSha256 (struct hash_engine *Engine, const uint8_t *Data,
	size_t Length, uint8_t *Hash, size_t HashLength)
{
    return HashEngineCalculateSha256(Data, Length, Hash, HashLength);
}

static int HashStartSha256 (struct hash_engine *Engine)
{
    return HashEngineStartSha256();
}

static int HashCalculateSha384 (struct hash_engine *Engine, const uint8_t *Data,
	size_t Length, uint8_t *Hash, size_t HashLength)
{
    return HashEngineCalculateSha384(Data, Length, Hash, HashLength);;
}

static int HashStartSha384 (struct hash_engine *Engine){

    return HashEngineStartSha384();
}

static int HashUpdate (struct hash_engine *Engine, const uint8_t *Data, size_t Length)
{
    return HashEngineUpdate(Data, Length);
}

static int HashFinish (struct hash_engine *Engine, uint8_t *Hash, size_t HashLength)
{
    return HashEngineFinish(Hash, HashLength);
}

static void HashCancel (struct hash_engine *Engine)
{
    HashEngineCancel();
}
/**
 * Initialize an mbed TLS hash engine.
 *
 * @param engine The hash engine to initialize.
 *
 * @return 0 if the hash engine was successfully initialized or an error code.
 */
int HashInitialize (struct hash_engine *Engine)
{
	if (Engine == NULL) {
		return HASH_ENGINE_INVALID_ARGUMENT;
	}

	memset (Engine, 0, sizeof (struct hash_engine));

	Engine->calculate_sha256 = HashCalculateSha256;
	Engine->start_sha256 = HashStartSha256;
#ifdef HASH_ENABLE_SHA384
	Engine->calculate_sha384 = HashCalculateSha384;
	Engine->start_sha384 = HashStartSha384;
#endif
	Engine->update = HashUpdate;
	Engine->finish = HashFinish;
	Engine->cancel = HashCancel;

	return 0;
}
