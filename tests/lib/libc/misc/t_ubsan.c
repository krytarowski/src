/*	$NetBSD$	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2018\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD$");

#include <sys/types.h>
#include <sys/wait.h>

#ifdef __cplusplus
#include <atf-c++.hpp>
#define UBSAN_TC(a)		ATF_TEST_CASE(a)
#define UBSAN_TC_HEAD(a, b)	ATF_TEST_CASE_HEAD(a)
#define UBSAN_TC_BODY(a, b)	ATF_TEST_CASE_BODY(a)
#define UBSAN_CASES(a)		ATF_INIT_TEST_CASES(a)
#define UBSAN_TEST_CASE(a, b)	ATF_ADD_TEST_CASE(a, b)
#define UBSAN_MD_VAR(a, b, c)	set_md_var(b, c)
#else
#include <atf-c.h>
#define UBSAN_TC(a)		ATF_TC(a)
#define UBSAN_TC_HEAD(a, b)	ATF_TC_HEAD(a, b)
#define UBSAN_TC_BODY(a, b)	ATF_TC_BODY(a, b)
#define UBSAN_CASES(a)		ATF_TP_ADD_TCS(a)
#define UBSAN_TEST_CASE(a, b)	ATF_TP_ADD_TC(a, b)
#define UBSAN_MD_VAR(a, b, c)	atf_tc_set_md_var(a, b, c)
#endif

#ifdef ENABLE_TESTS
#include "ubsan.c"

static void
test_case(void (*fun)(void), const char *string, bool exited, bool signaled)
{
	int filedes[2];
	pid_t pid;
	FILE *fp;
	size_t len;
	char *buffer;
	int status;

	/*
	 * Spawn a subprocess that triggers the issue.
	 * A child process either exits or is signaled with a crash signal.
	 */
	ATF_REQUIRE_EQ(pipe(filedes), 0);
	pid = fork();
	ATF_REQUIRE(pid != -1);
	if (pid == 0) {
		ATF_REQUIRE(dup2(filedes[1], STDERR_FILENO) != -1);
		ATF_REQUIRE(close(filedes[0]) == 0);
		ATF_REQUIRE(close(filedes[1]) == 0);

		(*fun)();
	}

	ATF_REQUIRE(close(filedes[1]) == 0);

	fp = fdopen(filedes[0], "r");
	ATF_REQUIRE(fp != NULL);

	buffer = fgetln(fp, &len);
	ATF_REQUIRE(buffer != 0);
	ATF_REQUIRE(!ferror(fp));
	ATF_REQUIRE(strstr(buffer, string) != NULL);
	ATF_REQUIRE(wait(&status) == pid);
	ATF_REQUIRE(!!WIFEXITED(status) == exited);
	ATF_REQUIRE(!!WIFSIGNALED(status) == signaled);
	ATF_REQUIRE(!WIFSTOPPED(status));
	ATF_REQUIRE(!WIFCONTINUED(status));
}

UBSAN_TC(add_overflow_signed);
UBSAN_TC_HEAD(add_overflow_signed, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

static void
test_add_overflow_signed(void)
{
	volatile int a = INT_MAX;
	volatile int b = atoi("1");

	_exit((a + b) ? 1 : 2);
}

UBSAN_TC_BODY(add_overflow_signed, tc)
{

	test_case(test_add_overflow_signed, " signed integer overflow: ", true, false);
}

UBSAN_TC(add_overflow_unsigned);
UBSAN_TC_HEAD(add_overflow_unsigned, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

static void
test_add_overflow_unsigned(void)
{
	volatile unsigned int a = UINT_MAX;
	volatile unsigned int b = atoi("1");

	_exit((a + b) ? 1 : 2);
}

UBSAN_TC_BODY(add_overflow_unsigned, tc)
{

	test_case(test_add_overflow_unsigned, " unsigned integer overflow: ", true, false);
}

UBSAN_TC(builtin_unreachable);
UBSAN_TC_HEAD(builtin_unreachable, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=unreachable");
}

static void
test_builtin_unreachable(void)
{
	volatile int a = atoi("1");
	volatile int b = atoi("1");

	if (a == b) {
		__builtin_unreachable();
	}
}

UBSAN_TC_BODY(builtin_unreachable, tc)
{

	test_case(test_builtin_unreachable, " calling __builtin_unreachable()", false, true);
}

UBSAN_TC(divrem_overflow_signed_div);
UBSAN_TC_HEAD(divrem_overflow_signed_div, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

static void
test_divrem_overflow_signed_div(void)
{
	volatile int a = INT_MIN;
	volatile int b = atoi("-1");

	_exit((a / b)  ? 1 : 2); // SIGFPE will be triggered before exiting
}

UBSAN_TC_BODY(divrem_overflow_signed_div, tc)
{

	test_case(test_divrem_overflow_signed_div, " signed integer overflow: ", false, true);
}

UBSAN_TC(divrem_overflow_signed_mod);
UBSAN_TC_HEAD(divrem_overflow_signed_mod, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

static void
test_divrem_overflow_signed_mod(void)
{
	volatile int a = INT_MIN;
	volatile int b = atoi("-1");

	_exit((a % b) ? 1 : 2);
}

UBSAN_TC_BODY(divrem_overflow_signed_mod, tc)
{

	test_case(test_divrem_overflow_signed_mod, " signed integer overflow: ", false, true);
}

#if defined(__cplusplus) && (defined(__x86_64__) || defined(__i386__))
UBSAN_TC(function_type_mismatch);
UBSAN_TC_HEAD(function_type_mismatch, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=function");
}

static int
fun_type_mismatch(void)
{
}

static void
test_function_type_mismatch(void)
{

	_exit(reinterpret_cast<void(*)(int)>
	    (reinterpret_cast<uintptr_t>(fun_type_mismatch))(1));
}

UBSAN_TC_BODY(function_type_mismatch, tc)
{

	test_case(test_function_type_mismatch, " signed integer overflow: ",
	          true, false);
}
#endif

#define INVALID_BUILTIN(type)				\
UBSAN_TC(invalid_builtin_##type);			\
UBSAN_TC_HEAD(invalid_builtin_##type, tc)		\
{							\
        UBSAN_MD_VAR(tc, "descr",			\
	    "Checks -fsanitize=builtin");		\
}							\
							\
static void						\
test_invalid_builtin_##type(void)			\
{							\
							\
	volatile int a = atoi("0");			\
	_exit(__builtin_##type(a));			\
}							\
							\
UBSAN_TC_BODY(invalid_builtin_##type, tc)		\
{							\
							\
	test_case(test_invalid_builtin_##type,		\
	          " XXX  ", false, true);		\
}

INVALID_BUILTIN(ctz)
INVALID_BUILTIN(ctzl)
INVALID_BUILTIN(ctzll)
INVALID_BUILTIN(clz)
INVALID_BUILTIN(clzl)
INVALID_BUILTIN(clzll)

UBSAN_TC(load_invalid_value_bool);
UBSAN_TC_HEAD(load_invalid_value_bool, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=bool");
}

static void
test_load_invalid_value_bool(void)
{
	volatile int a = atoi("10");
	volatile bool b = a;

	_exit(b ? 1 : 2);
}

UBSAN_TC_BODY(load_invalid_value_bool, tc)
{
	test_case(test_load_invalid_value_bool, " XXX ", true, false);
}

UBSAN_TC(load_invalid_value_enum);
UBSAN_TC_HEAD(load_invalid_value_enum, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=enum");
}

static void
test_load_invalid_value_enum(void)
{
	enum e { e1, e2, e3, e4 };
	volatile int a = atoi("10");
	volatile enum e E = STATIC_CAST(enum e, a);

	_exit((E == e1) ? 1 : 2);
}

UBSAN_TC_BODY(load_invalid_value_enum, tc)
{

	test_case(test_load_invalid_value_enum, " signed integer overflow: ", true, false);
}

#ifdef __cplusplus
UBSAN_TC(missing_return);
UBSAN_TC_HEAD(missing_return, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=return");
}

static int
fun_missing_return(void)
{
}

static void
test_missing_return(void)
{
	volatile int a = fun_missing_return();

	_exit(a ? 1 : 2);
}

UBSAN_TC_BODY(missing_return, tc)
{

	test_case(test_missing_return, " XXX ", true, false);
}
#endif

UBSAN_TC(mul_overflow_signed);
UBSAN_TC_HEAD(mul_overflow_signed, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

static void
test_mul_overflow_signed(void)
{
	volatile int a = INT_MAX;
	volatile int b = atoi("2");

	_exit((a * b) ? 1 : 2);
}

UBSAN_TC_BODY(mul_overflow_signed, tc)
{

	test_case(test_mul_overflow_signed, " signed integer overflow: ", true, false);
}

UBSAN_TC(mul_overflow_unsigned);
UBSAN_TC_HEAD(mul_overflow_unsigned, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

static void
test_mul_overflow_unsigned(void)
{
	volatile unsigned int a = UINT_MAX;
	volatile unsigned int b = atoi("2");

	_exit((a * b) ? 1 : 2);
}

UBSAN_TC_BODY(mul_overflow_unsigned, tc)
{

	test_case(test_mul_overflow_signed, " signed integer overflow: ", true, false);
}

UBSAN_TC(negate_overflow_signed);
UBSAN_TC_HEAD(negate_overflow_signed, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

static void
test_negate_overflow_signed(void)
{
	volatile int a = INT_MIN;

	_exit(-a ? 1 : 2);
}

UBSAN_TC_BODY(negate_overflow_signed, tc)
{

	test_case(test_negate_overflow_signed, " signed integer overflow: ", false, true);
}

UBSAN_TC(negate_overflow_unsigned);
UBSAN_TC_HEAD(negate_overflow_unsigned, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

void
test_negate_overflow_unsigned(void)
{
	volatile unsigned int a = UINT_MAX;

	_exit(-a ? 1 : 2);
}

UBSAN_TC_BODY(negate_overflow_unsigned, tc)
{

	test_case(test_negate_overflow_unsigned, " signed integer overflow: ", true, false);
}

#ifdef __clang
UBSAN_TC(nonnull_arg);
UBSAN_TC_HEAD(nonnull_arg, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=nullability-arg");
}

static void *
fun_nonnull_arg(void * _Nonnull ptr)
{

	return ptr;
}

void
test_nonnull_arg(void)
{
	volatile intptr_t a = atoi("0");

	_exit(fun_nonnull_arg((void *)a) ? 1 : 2);
}

UBSAN_TC_BODY(nonnull_arg, tc)
{

	test_case(test_nonnull_arg, " signed integer overflow: ", true, false);
}

UBSAN_TC(nonnull_assign);
UBSAN_TC_HEAD(nonnull_assign, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=nullability-assign");
}

static void *
fun_nonnull_assign(intptr_t a)
{
	volatile void *_Nonnull ptr;

	ptr = (void *)a;

	return ptr;
}

static void
test_nonnull_assign(void)
{
	volatile intptr_t a = atoi("0");

	_exit(fun_nonnull_assign(a) ? 1 : 2);
}

UBSAN_TC_BODY(nonnull_assign, tc)
{

	test_case(test_nonnull_assign, " signed integer overflow: ", true, false);
}

UBSAN_TC(nonnull_return);
UBSAN_TC_HEAD(nonnull_return, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=nullability-return");
}

static void *
fun_nonnull_return(void)
{
	volatile intptr_t a = atoi("0");

	return (void *)ptr;
}

static void
test_nonnull_return(void)
{

	_exit(fun_nonnull_return() ? 1 : 2);
}

UBSAN_TC_BODY(nonnull_return, tc)
{

	test_case(test_nonnull_return, " XXX ", true, false);
}
#endif

UBSAN_TC(out_of_bounds);
UBSAN_TC_HEAD(out_of_bounds, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=bounds");
}

static void
test_out_of_bounds(void)
{
	int A[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	volatile int a = atoi("10");

	_exit(A[a] ? 1 : 2);
}

UBSAN_TC_BODY(out_of_bounds, tc)
{

	test_case(test_out_of_bounds, " XXX ", true, false);
}

UBSAN_TC(pointer_overflow);
UBSAN_TC_HEAD(pointer_overflow, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=pointer-overflow");
}

static void
test_pointer_overflow(void)
{
	volatile uintptr_t a = UINTPTR_MAX;
	volatile uintptr_t b = atoi("1");
	volatile int *ptr = REINTERPRET_CAST(int *, a);

	_exit((ptr + b) ? 1 : 2);
}

UBSAN_TC_BODY(pointer_overflow, tc)
{

	test_case(test_pointer_overflow, " XXX ", true, false);
}

#ifndef __cplusplus
UBSAN_TC(shift_out_of_bounds_signednessbit);
UBSAN_TC_HEAD(shift_out_of_bounds_signednessbit, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=shift");
}

static void
test_shift_out_of_bounds_signednessbit(void)
{
	volatile int32_t a = atoi("1");

	_exit((a << 31) ? 1 : 2);
}

UBSAN_TC_BODY(shift_out_of_bounds_signednessbit, tc)
{

	test_case(test_shift_out_of_bounds_signednessbit, " XXX ", true, false);
}
#endif

UBSAN_TC(shift_out_of_bounds_signedoverflow);
UBSAN_TC_HEAD(shift_out_of_bounds_signedoverflow, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=shift");
}

static void
test_shift_out_of_bounds_signedoverflow(void)
{
	volatile int32_t a = atoi("1");
	volatile int32_t b = atoi("30");
	a <<= b;

	_exit((a << 10) ? 1 : 2);
}

UBSAN_TC_BODY(shift_out_of_bounds_signedoverflow, tc)
{

	test_case(test_shift_out_of_bounds_signedoverflow, " XXX ", true, false);
}

UBSAN_TC(shift_out_of_bounds_negativeexponent);
UBSAN_TC_HEAD(shift_out_of_bounds_negativeexponent, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=shift");
}

static void
test_load_invalid_value_enum(void)
{
	volatile int32_t a = atoi("1");
	volatile int32_t b = atoi("-10");

	_exit((a << b) ? 1 : 2);
}

UBSAN_TC_BODY(shift_out_of_bounds_negativeexponent, tc)
{

	test_case(test_load_invalid_value_enum, " XXX ", true, false);
}

UBSAN_TC(shift_out_of_bounds_toolargeexponent);
UBSAN_TC_HEAD(shift_out_of_bounds_toolargeexponent, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=shift");
}

static void
test_shift_out_of_bounds_toolargeexponent(void)
{
	volatile int32_t a = atoi("1");
	volatile int32_t b = atoi("40");

	_exit((a << b) ? 1 : 2);
}

UBSAN_TC_BODY(shift_out_of_bounds_toolargeexponent, tc)
{

	test_case(test_shift_out_of_bounds_toolargeexponent, " XXX ", true, false);
}

UBSAN_TC(sub_overflow_signed);
UBSAN_TC_HEAD(sub_overflow_signed, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

static void
test_sub_overflow_signed(void)
{
	volatile int a = INT_MIN;
	volatile int b = atoi("1");

	_exit((a - b) ? 1 : 2);
}

UBSAN_TC_BODY(sub_overflow_signed, tc)
{

	test_case(test_sub_overflow_signed, " XXX ", true, false);
}

UBSAN_TC(sub_overflow_unsigned);
UBSAN_TC_HEAD(sub_overflow_unsigned, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

static void
test_sub_overflow_unsigned(void)
{
	volatile unsigned int a = atoi("0");
	volatile unsigned int b = atoi("1");

	_exit((a - b) ? 1 : 2);
}

UBSAN_TC_BODY(sub_overflow_unsigned, tc)
{

	test_case(test_sub_overflow_unsigned, " XXX ", true, false);
}

UBSAN_TC(type_mismatch_nullptrderef);
UBSAN_TC_HEAD(type_mismatch_nullptrderef, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=null");
}

static void
test_type_mismatch_nullptrderef(void)
{
	volatile intptr_t a = atoi("0");
	volatile int *b = REINTERPRET_CAST(int *, a);

	_exit((*b) ? 1 : 2);
}

UBSAN_TC_BODY(type_mismatch_nullptrderef, tc)
{

	test_case(test_type_mismatch_nullptrderef, " XXX ", true, false);
}

UBSAN_TC(type_mismatch_misaligned);
UBSAN_TC_HEAD(type_mismatch_misaligned, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=alignment");
}

UBSAN_TC_BODY(type_mismatch_misaligned, tc)
{
	volatile int8_t A[10] __aligned(4);
	volatile int *b;

	memset(__UNVOLATILE(A), 0, sizeof(A));

	b = REINTERPRET_CAST(volatile int *, &A[1]);

	usleep((*b) ? 1 : 2);
}

UBSAN_TC(vla_bound_not_positive);
UBSAN_TC_HEAD(vla_bound_not_positive, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=vla-bound");
}

UBSAN_TC_BODY(vla_bound_not_positive, tc)
{
	volatile int a = atoi("-1");
	int A[a];

	usleep(A[0] ? 1 : 2);
}

UBSAN_TC(integer_divide_by_zero);
UBSAN_TC_HEAD(integer_divide_by_zero, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=integer-divide-by-zero");
}

static void
test_integer_divide_by_zero(void)
{
	volatile int a = atoi("-1");
	volatile int b = atoi("0");

	_exit((a / b) ? 1 : 2);
}

UBSAN_TC_BODY(integer_divide_by_zero, tc)
{

	test_case(test_integer_divide_by_zero, " XXX ", true, false);
}

UBSAN_TC(float_divide_by_zero);
UBSAN_TC_HEAD(float_divide_by_zero, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "Checks -fsanitize=float-divide-by-zero");
}

static void
test_float_divide_by_zero(void)
{
	volatile float a = strtof("1.0", NULL);
	volatile float b = strtof("0.0", NULL);

	_exit((a / b) > 0 ? 1 : 2);
}

UBSAN_TC_BODY(float_divide_by_zero, tc)
{

	test_case(test_float_divide_by_zero, " XXX ", true, false);
}
#else
UBSAN_TC(dummy);
UBSAN_TC_HEAD(dummy, tc)
{
        UBSAN_MD_VAR(tc, "descr",
	    "A dummy test");
}

UBSAN_TC_BODY(dummy, tc)
{

	// Dummy, skipped
	// The framework requires at least a single defined test.
}
#endif

UBSAN_CASES(tp)
{
#ifdef ENABLE_TESTS
	UBSAN_TEST_CASE(tp, add_overflow_signed);
	UBSAN_TEST_CASE(tp, add_overflow_unsigned);
	UBSAN_TEST_CASE(tp, builtin_unreachable);
//	UBSAN_TEST_CASE(tp, cfi_bad_type);	// TODO
//	UBSAN_TEST_CASE(tp, cfi_check_fail);	// TODO
	UBSAN_TEST_CASE(tp, divrem_overflow_signed_div);
	UBSAN_TEST_CASE(tp, divrem_overflow_signed_mod);
//	UBSAN_TEST_CASE(tp, dynamic_type_cache_miss); // Not supported in uUBSan
//	UBSAN_TEST_CASE(tp, float_cast_overflow);	// TODO
#if defined(__cplusplus) && (defined(__x86_64__) || defined(__i386__))
	UBSAN_TEST_CASE(tp, function_type_mismatch);
#endif
	UBSAN_TEST_CASE(tp, invalid_builtin_ctz);
	UBSAN_TEST_CASE(tp, invalid_builtin_ctzl);
	UBSAN_TEST_CASE(tp, invalid_builtin_ctzll);
	UBSAN_TEST_CASE(tp, invalid_builtin_clz);
	UBSAN_TEST_CASE(tp, invalid_builtin_clzl);
	UBSAN_TEST_CASE(tp, invalid_builtin_clzll);
	UBSAN_TEST_CASE(tp, load_invalid_value_bool);
	UBSAN_TEST_CASE(tp, load_invalid_value_enum);
#ifdef __cplusplus
	UBSAN_TEST_CASE(tp, missing_return);
#endif
	UBSAN_TEST_CASE(tp, mul_overflow_signed);
	UBSAN_TEST_CASE(tp, mul_overflow_unsigned);
	UBSAN_TEST_CASE(tp, negate_overflow_signed);
	UBSAN_TEST_CASE(tp, negate_overflow_unsigned);
#ifdef __clang
	// Clang/LLVM specific extension
	// http://clang.llvm.org/docs/AttributeReference.html#nullability-attributes
	UBSAN_TEST_CASE(tp, nonnull_arg);
	UBSAN_TEST_CASE(tp, nonnull_assign);
	UBSAN_TEST_CASE(tp, nonnull_return);
#endif
	UBSAN_TEST_CASE(tp, out_of_bounds);
	UBSAN_TEST_CASE(tp, pointer_overflow);
#ifndef __cplusplus
	// Acceptable in C++11
	UBSAN_TEST_CASE(tp, shift_out_of_bounds_signednessbit);
#endif
	UBSAN_TEST_CASE(tp, shift_out_of_bounds_signedoverflow);
	UBSAN_TEST_CASE(tp, shift_out_of_bounds_negativeexponent);
	UBSAN_TEST_CASE(tp, shift_out_of_bounds_toolargeexponent);
	UBSAN_TEST_CASE(tp, sub_overflow_signed);
	UBSAN_TEST_CASE(tp, sub_overflow_unsigned);
	UBSAN_TEST_CASE(tp, type_mismatch_nullptrderef);
	UBSAN_TEST_CASE(tp, type_mismatch_misaligned);
	UBSAN_TEST_CASE(tp, vla_bound_not_positive);
	UBSAN_TEST_CASE(tp, integer_divide_by_zero);
	UBSAN_TEST_CASE(tp, float_divide_by_zero);
#else
	UBSAN_TEST_CASE(tp, dummy);
#endif

#ifndef __cplusplus
	return atf_no_error();
#endif
}
