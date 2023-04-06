//***********************************************************************
//*                                                                     *
//*                  Copyright (c) 1985-2022, AMI.                      *
//*                                                                     *
//*      All rights reserved. Subject to AMI licensing agreement.       *
//*                                                                     *
//***********************************************************************
/**@file Hash.h
 * This file contains the Crypto Handling functions
 */
#include <stdlib.h>
#include <string.h>
#include <crypto/hash.h>

int HashInitialize (struct hash_engine *Engine);