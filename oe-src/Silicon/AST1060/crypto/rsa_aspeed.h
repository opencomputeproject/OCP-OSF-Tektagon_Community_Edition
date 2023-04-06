/*
 * Copyright (c) 2021 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_RSA_API_MIDLEYER_H_
#define ZEPHYR_INCLUDE_RSA_API_MIDLEYER_H_

#include <crypto/rsa_structs.h>

#define ZEPHYR_RSA_API_MIDLEYER_TEST_SUPPORT 1  // non-zero for support rsa functions testing

#ifdef CONFIG_CRYPTO_ASPEED
#ifndef RSA_DRV_NAME
#define RSA_DRV_NAME DT_LABEL(DT_INST(0, aspeed_rsa))     // define rsa driver for Aspeed
#endif
#endif

#if ZEPHYR_RSA_API_MIDLEYER_TEST_SUPPORT
void rsa_engine_function_test(void);    // rsa functions testing
#endif

int decrypt_aspeed(const struct rsa_key *key, const uint8_t *encrypted, size_t in_length, uint8_t *decrypted, size_t out_length);
int sig_verify_aspeed(const struct rsa_key *key, const uint8_t *signature, int sig_length, const uint8_t *match, size_t match_length);
int rsa_sig_verify_test(void);
#endif  /* ZEPHYR_INCLUDE_RSA_API_MIDLEYER_H_ */
