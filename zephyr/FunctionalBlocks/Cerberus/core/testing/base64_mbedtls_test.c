// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "testing.h"
#include "testing/base64_testing.h"
#include "crypto/base64_mbedtls.h"


static const char *SUITE = "base64_mbedtls";


/*******************
 * Test cases
 *******************/

static void base64_mbedtls_test_init (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;

	TEST_START;

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	CuAssertPtrNotNull (test, engine.base.encode);

	base64_mbedtls_release (&engine);
}

static void base64_mbedtls_test_init_null (CuTest *test)
{
	int status;

	TEST_START;

	status = base64_mbedtls_init (NULL);
	CuAssertIntEquals (test, BASE64_ENGINE_INVALID_ARGUMENT, status);
}

static void base64_mbedtls_test_release_null (CuTest *test)
{
	TEST_START;

	base64_mbedtls_release (NULL);
}

static void base64_mbedtls_test_encode (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;
	uint8_t out[BASE64_ENCODED_BLOCK_LEN * 2];

	TEST_START;

	memset (out, 0xff, sizeof (out));

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.encode (&engine.base, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN, out,
		sizeof (out));
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, BASE64_ENCODED_BLOCK_LEN, BASE64_LENGTH (BASE64_DATA_BLOCK_LEN));

	status = testing_validate_array (BASE64_ENCODED_BLOCK, out, BASE64_ENCODED_BLOCK_LEN);
	CuAssertIntEquals (test, 0, status);

	base64_mbedtls_release (&engine);
}

static void base64_mbedtls_test_encode_pad_one_byte (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;
	uint8_t out[BASE64_ENCODED_BLOCK_LEN * 2];

	TEST_START;

	memset (out, 0xff, sizeof (out));

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.encode (&engine.base, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN - 1, out,
		sizeof (out));
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, BASE64_ENCODED_PAD_ONE_LEN, BASE64_LENGTH (BASE64_DATA_BLOCK_LEN - 1));

	status = testing_validate_array (BASE64_ENCODED_PAD_ONE, out, BASE64_ENCODED_PAD_ONE_LEN);
	CuAssertIntEquals (test, 0, status);

	base64_mbedtls_release (&engine);
}

static void base64_mbedtls_test_encode_pad_two_bytes (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;
	uint8_t out[BASE64_ENCODED_BLOCK_LEN * 2];

	TEST_START;

	memset (out, 0xff, sizeof (out));

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.encode (&engine.base, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN - 2, out,
		sizeof (out));
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, BASE64_ENCODED_PAD_TWO_LEN, BASE64_LENGTH (BASE64_DATA_BLOCK_LEN - 2));

	status = testing_validate_array (BASE64_ENCODED_PAD_TWO, out, BASE64_ENCODED_PAD_TWO_LEN);
	CuAssertIntEquals (test, 0, status);

	base64_mbedtls_release (&engine);
}

static void base64_mbedtls_test_encode_not_multiple_of_4 (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;
	uint8_t out[BASE64_ENCODED_BLOCK_LEN * 2];

	TEST_START;

	memset (out, 0xff, sizeof (out));

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.encode (&engine.base, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN - 3, out,
		sizeof (out));
	CuAssertIntEquals (test, 0, status);
	CuAssertIntEquals (test, BASE64_ENCODED_THREE_LESS_LEN,
		BASE64_LENGTH (BASE64_DATA_BLOCK_LEN - 3));

	status = testing_validate_array (BASE64_ENCODED_THREE_LESS, out, BASE64_ENCODED_THREE_LESS_LEN);
	CuAssertIntEquals (test, 0, status);

	base64_mbedtls_release (&engine);
}

static void base64_mbedtls_test_encode_null (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;
	uint8_t out[BASE64_ENCODED_BLOCK_LEN * 2];

	TEST_START;

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.encode (NULL, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN, out,
		sizeof (out));
	CuAssertIntEquals (test, BASE64_ENGINE_INVALID_ARGUMENT, status);

	status = engine.base.encode (&engine.base, NULL, BASE64_DATA_BLOCK_LEN, out,
		sizeof (out));
	CuAssertIntEquals (test, BASE64_ENGINE_INVALID_ARGUMENT, status);

	status = engine.base.encode (&engine.base, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN, NULL,
		sizeof (out));
	CuAssertIntEquals (test, BASE64_ENGINE_INVALID_ARGUMENT, status);

	base64_mbedtls_release (&engine);
}

static void base64_mbedtls_test_encode_buffer_too_small (CuTest *test)
{
	struct base64_engine_mbedtls engine;
	int status;
	uint8_t out[BASE64_ENCODED_BLOCK_LEN * 2];

	TEST_START;

	status = base64_mbedtls_init (&engine);
	CuAssertIntEquals (test, 0, status);

	status = engine.base.encode (&engine.base, BASE64_DATA_BLOCK, BASE64_DATA_BLOCK_LEN, out,
		BASE64_LENGTH (BASE64_DATA_BLOCK_LEN) - 1);
	CuAssertIntEquals (test, BASE64_ENGINE_ENC_BUFFER_TOO_SMALL, status);

	base64_mbedtls_release (&engine);
}


CuSuite* get_base64_mbedtls_suite ()
{
	CuSuite *suite = CuSuiteNew ();

	SUITE_ADD_TEST (suite, base64_mbedtls_test_init);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_init_null);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_release_null);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_encode);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_encode_pad_one_byte);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_encode_pad_two_bytes);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_encode_not_multiple_of_4);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_encode_null);
	SUITE_ADD_TEST (suite, base64_mbedtls_test_encode_buffer_too_small);

	return suite;
}
