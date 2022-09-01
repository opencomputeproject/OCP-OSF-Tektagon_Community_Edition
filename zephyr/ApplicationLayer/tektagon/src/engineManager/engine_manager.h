//***********************************************************************//
//*                                                                     *//
//*                      Copyright © 2022 AMI                           *//
//*                                                                     *//
//*        All rights reserved. Subject to AMI licensing agreement.     *//
//*                                                                     *//
//***********************************************************************//

/*
 * engines.h
 *
 *  Created on: Dec 10, 2021
 *      Author: presannar
 */

#ifndef ZEPHYR_TEKTAGON_SRC_INCLUDE_ENGINES_H_
#define ZEPHYR_TEKTAGON_SRC_INCLUDE_ENGINES_H_

#include <zephyr.h>


struct engine_instances {
	struct aes_engine *aesEngine;
	struct base64_engine *base64Engine;
	struct ecc_engine *eccEngine;
	struct flash *flashDevice;
	struct flash_master *flashMaster;                               /**< Flash master for the PFM flash. */
	struct flash_engine_wrapper *flashEngineWrapper;
	struct hash_engine *hashEngine;
	struct keystore *keyStore;
	struct manifest *manifestHandler;
	struct pfm_flash *pfmFlash;
	struct rng_engine *rngEngine;
	struct rsa_engine *rsaEngine;
	struct signature_verification *verification;    /**< PFM signature verification. */
	struct spi_flash *spiFlash;                     /**< Flash where the PFM is stored. */
	struct spi_flash_wrapper *spiEngineWrapper;
	struct x509_engine *x509Engine;
};
int signature_verification_init(struct signature_verification *verification);
int initializeEngines(void);

#endif /* ZEPHYR_TEKTAGON_SRC_INCLUDE_ENGINES_H_ */
