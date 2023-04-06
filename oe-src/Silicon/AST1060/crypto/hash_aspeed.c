/*
 * Copyright (c) 2021 AMI
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#include <sys/printk.h>
#include <crypto/hash_structs.h>
#include <crypto/hash.h>
#include "hash_aspeed.h"

static struct hash_params hashParams;   // hash internal parameters

/**
 * @brief Calculate a hash on a complete set of data.
 *
 * @param algo hash algorithm as SHA1, SHA256, SHA384, SHA512
 * @param data plain text
 * @param length size of plain text
 * @param hash hash digest
 * @param hash_length hash digest length
 *
 * @return 0 if the hash calculated successfully or an error code.
 */
int hash_engine_sha_calculate(enum hash_algo algo, const uint8_t *data, size_t length, uint8_t *hash, size_t hash_length)
{
	const struct device *dev;       // hash engine driver info
	int ret;

	dev = device_get_binding(HASH_DRV_NAME);        // retrieves hash driver device info

	hashParams.pkt.in_buf = (uint8_t *)data;        // plaint text info
	hashParams.pkt.in_len = length;                 // plaint text size
	hashParams.pkt.out_buf = hash;                  // hash value and this will updated by hash engine
	hashParams.pkt.out_buf_max = hash_length;       // hash length

	if (hashParams.sessionReady) {
		ret = 0;                                                // hash engine session is ready
	} else   {
		ret = hash_begin_session(dev, &hashParams.ctx, algo);   // initializes hash engine session
	}

	if (!ret) {                                                             // success to initializes hash engine
		ret = hash_update(&hashParams.ctx, &hashParams.pkt);            // update plaint text into hash engine

		if (!ret) {                                                     // success to update hash engine
			ret = hash_final(&hashParams.ctx, &hashParams.pkt);     // final setup hash engine
		}
	}

	hash_free_session(dev, &hashParams.ctx);        // free hash engine

	return ret;
}

/**
 * @brief Configure the hash engine to process independent blocks of data to calculate a hash
 * the aggregated data.
 *
 * Calling this function will reset any active hashing operation.
 *
 * Every call to start MUST be followed by either a call to finish or cancel.
 *
 * @param algo hash algorithm as SHA1, SHA256, SHA384, SHA512
 *
 * @return 0 if the hash engine was configured successfully or an error code.
 */
int hash_engine_start(enum hash_algo algo)
{
	const struct device *dev;       // hash engine driver
	int ret;

	memset(&hashParams, 0, sizeof(hashParams));             // clear all the hash internal parameters

	dev = device_get_binding(HASH_DRV_NAME);                // retrieves hash driver device info

	ret = hash_begin_session(dev, &hashParams.ctx, algo);   // initializes hash engine

	if (!ret)
		hashParams.sessionReady = 1;    // hash engine session is ready

	return ret;
}

/**
 * @brief Update the current hash operation with a block of data.
 *
 * @param data The data that should be added to generate the final hash.
 * @param length The length of the data.
 *
 * @return 0 if the hash operation was updated successfully or an error code.
 */
int hash_engine_update(const uint8_t *data, size_t length)
{
	int status;                                             // hash operation status

	hashParams.pkt.in_buf = (uint8_t *)data;                // plaint text info
	hashParams.pkt.in_len = length;                         // plaint text size
	status = hash_update(&hashParams.ctx, &hashParams.pkt); // update plaint text into hash engine

	return status;
}

/**
 * @brief Complete the current hash operation and get the calculated digest.
 *
 * If a call to finish fails, finish MUST be called until it succeeds or the operation can be
 * terminated with a call to cancel.
 *
 * @param hash The buffer to hold the completed hash.
 * @param hash_length The length of the hash buffer.
 *
 * @return 0 if the hash was completed successfully or an error code.
 */
int hash_engine_finish(uint8_t *hash, size_t hash_length)
{
	const struct device *dev = device_get_binding(HASH_DRV_NAME);   // retrieves hash driver device info
	int ret;

	hashParams.pkt.out_buf = hash,                          // hash value and this will updated by hash engine
	hashParams.pkt.out_buf_max = hash_length,               // hash size
	hashParams.sessionReady = 0;                            // clear as hash engine session as expired, this should initialize again

	ret = hash_final(&hashParams.ctx, &hashParams.pkt);     // final setup hash engine

	hash_free_session(dev, &hashParams.ctx);                // free hash engine

	return ret;
}

/**
 * @brief Cancel an in-progress hash operation without getting the hash values.  After canceling, a new
 * hash operation needs to be started.
 *
 * @param engine The hash engine to cancel.
 */
void hash_engine_cancel(void)
{
	const struct device *dev = device_get_binding(HASH_DRV_NAME);   // retrieves hash driver device info

	hashParams.sessionReady = 0;                                    // clear as hash engine session as expired, this should initialize again

	hash_free_session(dev, &hashParams.ctx);                        // free hash engine
}

#if ZEPHYR_HASH_API_MIDLEYER_TEST_SUPPORT

const uint8_t *SHA_DISPLAY_MSG[] = {
	"SHA1",
	"SHA256",
	"SHA384",
	"SHA512",
};

struct hash_test_hmac_info {
	enum hash_algo shaAlgo;
	uint32_t hmacLength;
	const uint8_t *message;
	uint32_t messageSize;
	const uint8_t *key;
	uint32_t keySize;
	uint8_t *expected;
};

const struct hash_test_hmac_info HASH_TEST_CAL_SHA_INFO[] = {
	{
		.shaAlgo = HASH_SHA256,
		.hmacLength = (256 / 8),
		.message = "Test",
		.messageSize = sizeof("Test") - 1,
		.expected = "\x53\x2e\xaa\xbd\x95\x74\x88\x0d\xbf\x76\xb9\xb8\xcc\x00\x83\x2c"
			    "\x20\xa6\xec\x11\x3d\x68\x22\x99\x55\x0d\x7a\x6e\x0f\x34\x5e\x25",
	},
#ifdef HASH_ENABLE_SHA384
	{
		.shaAlgo = HASH_SHA384,
		.hmacLength = (384 / 8),
		.message = "Test",
		.messageSize = sizeof("Test") - 1,
		.expected = "\x7b\x8f\x46\x54\x07\x6b\x80\xeb\x96\x39\x11\xf1\x9c\xfa\xd1\xaa"
			    "\xf4\x28\x5e\xd4\x8e\x82\x6f\x6c\xde\x1b\x01\xa7\x9a\xa7\x3f\xad"
			    "\xb5\x44\x6e\x66\x7f\xc4\xf9\x04\x17\x78\x2c\x91\x27\x05\x40\xf3"
	},
#endif
#ifdef HASH_ENABLE_SHA512
	{
		.shaAlgo = HASH_SHA512,
		.hmacLength = (512 / 8),
		.message = "Test",
		.messageSize = sizeof("Test") - 1,
		.expected = "\xc6\xee\x9e\x33\xcf\x5c\x67\x15\xa1\xd1\x48\xfd\x73\xf7\x31\x88"
			    "\x84\xb4\x1a\xdc\xb9\x16\x02\x1e\x2b\xc0\xe8\x00\xa5\xc5\xdd\x97"
			    "\xf5\x14\x21\x78\xf6\xae\x88\xc8\xfd\xd9\x8e\x1a\xfb\x0c\xe4\xc8"
			    "\xd2\xc5\x4b\x5f\x37\xb3\x0b\x7d\xa1\x99\x7b\xb3\x3b\x0b\x8a\x31",
	},
#endif
};

static int hash_test_start_new_hash_sha(void)
{
	int status, failure;
	uint8_t hash[512 / 8];

	printk("\n%s :\n", __func__);

	failure = 0;

	for (size_t i = 0; i < (sizeof(HASH_TEST_CAL_SHA_INFO) / sizeof(HASH_TEST_CAL_SHA_INFO[0])); i++) {
		status = hash_engine_start(HASH_TEST_CAL_SHA_INFO[i].shaAlgo);

		if (status) {
			printk(" X hash_engine_start failed !\n");
			failure = 1;
			continue;
		}

		status = hash_engine_update(HASH_TEST_CAL_SHA_INFO[i].message, HASH_TEST_CAL_SHA_INFO[i].messageSize);
		if (status) {
			printk(" X engine.base.update failed ! index : %d status : %x\n", i, status);
			failure = 1;
			continue;
		}
		// hmacLength = hash length
		status = hash_engine_finish(hash, HASH_TEST_CAL_SHA_INFO[i].hmacLength);
		if (status) {
			printk(" X engine.base.finish failed ! index : %d status : %x\n", i, status);
			failure = 1;
			continue;
		}

		// hmacLength = hash length
		status = memcmp(HASH_TEST_CAL_SHA_INFO[i].expected, hash, HASH_TEST_CAL_SHA_INFO[i].hmacLength);
		if (status) {
			printk(" X not match ! index : %d status : %x\n", i, status);
			failure = 1;
			continue;
		} else   {
			printk("index : %d %s PASS\n", i, SHA_DISPLAY_MSG[HASH_TEST_CAL_SHA_INFO[i].shaAlgo]);
		}
	}

	return failure;
}

static int hash_test_calculate_sha(void)
{
	int status, failure;
	uint8_t hash[(512 / 8)];

	failure = 0;

	printk("\n%s :\n", __func__);

	for (size_t i = 0; i < (sizeof(HASH_TEST_CAL_SHA_INFO) / sizeof(HASH_TEST_CAL_SHA_INFO[0])); i++) { // hmacLength = hash length

		status = hash_engine_start(HASH_TEST_CAL_SHA_INFO[i].shaAlgo);

		if (status) {
			printk(" X hash_engine_start failed !\n");
			failure = 1;
			continue;
		}

		status = hash_engine_sha_calculate(HASH_TEST_CAL_SHA_INFO[i].shaAlgo,
						   HASH_TEST_CAL_SHA_INFO[i].message, HASH_TEST_CAL_SHA_INFO[i].messageSize,
						   hash, HASH_TEST_CAL_SHA_INFO[i].hmacLength);

		if (status) {
			printk(" X hash_calculate failed ! index : %d status : %x\n", i, status);
			failure = 1;
			continue;
		}
		// hmacLength = hash length
		status = memcmp(HASH_TEST_CAL_SHA_INFO[i].expected, hash, HASH_TEST_CAL_SHA_INFO[i].hmacLength);
		if (status) {
			printk(" X not match ! index : %d status : %x\n", i, status);
			failure = 1;
			continue;
		} else   {
			printk("index : %d %s PASS\n", i, SHA_DISPLAY_MSG[HASH_TEST_CAL_SHA_INFO[i].shaAlgo]);
		}
	}

	return failure;
}

/**
 * @brief test hash engine features
 */
void hash_engine_function_test(void)
{
	uint8_t *status_string;

	status_string = (hash_test_calculate_sha()) ? ("FAIL") : ("PASS");

	printk("hash_test_calculate_sha : %s\n", status_string);

	status_string = (hash_test_start_new_hash_sha()) ? ("FAIL") : ("PASS");

	printk("hash_test_start_new_hash_sha : %s\n", status_string);
}

#endif  /* #if ZEPHYR_HASH_API_MIDLEYER_TEST_SUPPORT */
