//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * manifestProcessor.h
 *
 *  Created on: Dec 15, 2021
 *      Author: presannar
 */

#ifndef ZEPHYR_TEKTAGON_SRC_MANIFESTPROCESSOR_MANIFESTPROCESSOR_H_
#define ZEPHYR_TEKTAGON_SRC_MANIFESTPROCESSOR_MANIFESTPROCESSOR_H_

#include <zephyr.h>

int initializeManifestProcessor(void);
int processPfmFlashManifest(void);

#endif /* ZEPHYR_TEKTAGON_SRC_MANIFESTPROCESSOR_MANIFESTPROCESSOR_H_ */
