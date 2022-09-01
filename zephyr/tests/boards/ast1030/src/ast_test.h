/*
 * Copyright (c) 2021 Aspeedtech Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ASPEED_TESTSUITE_ZTEST_H_
#define ASPEED_TESTSUITE_ZTEST_H_

#include <ztest.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define AST_TEST_PASS	0
#define AST_TEST_FAIL	-1

enum aspeed_test_type {
	AST_TEST_CI = 0,
	AST_TEST_SLT,
	AST_TEST_FT,
};

struct aspeed_test_param {
	enum aspeed_test_type type;
	const char *name;
	int test_count;
	int timeout;
};

struct aspeed_tests {
	const char *name;
	int (*test)(int count, enum aspeed_test_type);
	int results;
};

static bool ast_test_fail;

const char *ztest_relative_filename(const char *file);

static inline bool aspeed_zassert(bool cond,
			    const char *default_msg,
			    const char *file,
			    int line, const char *func,
			    const char *msg, ...)
{
	if (cond == false) {
		va_list vargs;

		va_start(vargs, msg);
		PRINT("\n    Assertion failed at %s:%d: %s: %s\n",
		      ztest_relative_filename(file), line, func, default_msg);
		vprintk(msg, vargs);
		printk("\n");
		va_end(vargs);
		ast_test_fail = true;
		return false;
	}

	return true;
}

static inline int ast_test_result(void)
{
	return ast_test_fail ? AST_TEST_FAIL : AST_TEST_PASS;
}

#define ast_ztest_result()	ast_test_result()

/**
 * @defgroup ztest_assert Ztest assertion macros
 * @ingroup ztest
 *
 * This module provides assertions when using Ztest.
 *
 * @{
 */

/**
 * @brief Fail the test, if @a cond is false
 *
 * You probably don't need to call this macro directly. You should
 * instead use zassert_{condition} macros below.
 *
 * Note that when CONFIG_MULTITHREADING=n macro returns from the function. It is
 * then expected that in that case ztest asserts will be used only in the
 * context of the test function.
 *
 * @param cond Condition to check
 * @param msg Optional, can be NULL. Message to print if @a cond is false.
 * @param default_msg Message to print if @a cond is false
 */
#define ast_zassert(cond, default_msg, msg, ...) \
	aspeed_zassert(cond, msg ? ("(" default_msg ")") : (default_msg), \
			     __FILE__, __LINE__, __func__, \
			     msg ? msg : "", ##__VA_ARGS__);

/**
 * @brief Assert that this function call won't be reached
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_unreachable(msg, ...) ast_zassert(0, "Reached unreachable code", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is true
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_true(cond, msg, ...) ast_zassert(cond, #cond " is false", \
					     msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is false
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_false(cond, msg, ...) ast_zassert(!(cond), #cond " is true", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a cond is 0 (success)
 * @param cond Condition to check
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_ok(cond, msg, ...) ast_zassert(!(cond), #cond " is non-zero", \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_is_null(ptr, msg, ...) ast_zassert((ptr) == NULL,	    \
					       #ptr " is not NULL", \
					       msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a ptr is not NULL
 * @param ptr Pointer to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_not_null(ptr, msg, ...) ast_zassert((ptr) != NULL,	      \
						#ptr " is NULL", msg, \
						##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_equal(a, b, msg, ...) ast_zassert((a) == (b),	      \
					      #a " not equal to " #b, \
					      msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a does not equal @a b
 *
 * @a a and @a b won't be converted and will be compared directly.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_not_equal(a, b, msg, ...) ast_zassert((a) != (b),	      \
						  #a " equal to " #b, \
						  msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a equals @a b
 *
 * @a a and @a b will be converted to `void *` before comparing.
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_equal_ptr(a, b, msg, ...)			    \
	ast_zassert((void *)(a) == (void *)(b), #a " not equal to " #b, \
		msg, ##__VA_ARGS__)

/**
 * @brief Assert that @a a is within @a b with delta @a d
 *
 * @param a Value to compare
 * @param b Value to compare
 * @param d Delta
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_within(a, b, d, msg, ...)			     \
	ast_zassert(((a) >= ((b) - (d))) && ((a) <= ((b) + (d))),	     \
		#a " not within " #b " +/- " #d,		     \
		msg, ##__VA_ARGS__)

/**
 * @brief Assert that 2 memory buffers have the same contents
 *
 * This macro calls the final memory comparison assertion macro.
 * Using double expansion allows providing some arguments by macros that
 * would expand to more than one values (ANSI-C99 defines that all the macro
 * arguments have to be expanded before macro call).
 *
 * @param ... Arguments, see @ref zassert_mem_equal__
 *            for real arguments accepted.
 */
#define ast_zassert_mem_equal(...) \
	ast_zassert_mem_equal__(__VA_ARGS__)

/**
 * @brief Internal assert that 2 memory buffers have the same contents
 *
 * @note This is internal macro, to be used as a second expansion.
 *       See @ref zassert_mem_equal.
 *
 * @param buf Buffer to compare
 * @param exp Buffer with expected contents
 * @param size Size of buffers
 * @param msg Optional message to print if the assertion fails
 */
#define ast_zassert_mem_equal__(buf, exp, size, msg, ...)                    \
	ast_zassert(memcmp(buf, exp, size) == 0, #buf " not equal to " #exp, \
	msg, ##__VA_ARGS__)

#endif /* ZEPHYR_TESTSUITE_ZTEST_ASSERT_H_ */
