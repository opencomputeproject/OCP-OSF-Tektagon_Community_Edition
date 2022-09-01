// ***********************************************************************
// *                                                                     *
// *                  Copyright (c) 1985-2022, AMI.                      *
// *                                                                     *
// *      All rights reserved. Subject to AMI licensing agreement.       *
// *                                                                     *
// ***********************************************************************
/**@file
 * This file contains the Logging Handling functions
 */
#ifndef LOGGINGWRAPPER_H_
#define LOGGINGWRAPPER_H_

#include <logging/logging_flash.h>
#include <flash/flash_aspeed.h>

#define TE_LOGGING_FLASH_SIZE 0x1000000
#define TE_LOGGING_BASE_ADD 0
#define TE_ROT_INTERNAL_LOG ROT_INTERNAL_LOG

extern int debug_log_test(void);

#endif /* LOGGING_WRAPPER_H_ */