//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file
 * This file contains the Crypto Wrapper Handling functions
 */

#ifndef HASH_WRAPPER_H_
#define HASH_WRAPPER_H_

#include <stdio.h>
#include <stdlib.h>
int HashEngineCalculateSha256 (const char *Data, size_t Length, char *Hash, size_t HashLength);
int HashEngineStartSha256(void);
int HashEngineCalculateSha384 (const char *Data, size_t Length, char *Hash, size_t HashLength);
int HashEngineStartSha384(void);
int HashEngineUpdate (const char *Data, size_t Length);
int HashEngineFinish (char *Hash, size_t HashLength);
void HashEngineCancel(void);

#endif /* HASH_WRAPPER_H_ */
