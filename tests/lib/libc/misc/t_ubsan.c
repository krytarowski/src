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

#include <atf-c.h>


ATF_TC(add_overflow_signed);
ATF_TC_HEAD(add_overflow_signed, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

ATF_TC_BODY(add_overflow_signed, tc)
{
	volatile int a = INT_MAX;
	volatile int b = atoi("1");

	usleep((a + b) ? 1 : 2);
}

ATF_TC(add_overflow_unsigned);
ATF_TC_HEAD(add_overflow_unsigned, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

ATF_TC_BODY(add_overflow_unsigned, tc)
{
	volatile unsigned int a = UINT_MAX;
	volatile unsigned int b = atoi("1");

	usleep((a + b) ? 1 : 2);
}

ATF_TC(builtin_unreachable);
ATF_TC_HEAD(builtin_unreachable, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=unreachable");
}

ATF_TC_BODY(builtin_unreachable, tc)
{
	volatile int a = atoi("1");
	volatile int b = atoi("1");

	if (a == b) {
		__builtin_unreachable();
	}
}

ATF_TC(divrem_overflow_signed_div);
ATF_TC_HEAD(divrem_overflow_signed_div, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

ATF_TC_BODY(divrem_overflow_signed_div, tc)
{
	volatile int a = INT_MIN;
	volatile int b = atoi("-1");

	usleep((a / b)  ? 1 : 2);
}

ATF_TC(divrem_overflow_signed_mod);
ATF_TC_HEAD(divrem_overflow_signed_mod, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

ATF_TC_BODY(divrem_overflow_signed_mod, tc)
{
	volatile int a = INT_MIN;
	volatile int b = atoi("-1");

	usleep((a % b) ? 1 : 2);
}

#if defined(__cplusplus) && (defined(__x86_64__) || defined(__i386__))
ATF_TC(function_type_mismatch);
ATF_TC_HEAD(function_type_mismatch, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=function");
}

static void
fun_type_mismatch(void)
{
}

ATF_TC_BODY(function_type_mismatch, tc)
{

	((void(*)(int)) ((uintptr_t)(fun_type_mismatch))) (1);
}
#endif

#define INVALID_BUILTIN(type)				\
ATF_TC(invalid_builtin_##type);				\
ATF_TC_HEAD(invalid_builtin_##type, tc)			\
{							\
        atf_tc_set_md_var(tc, "descr",			\
	    "Checks -fsanitize=builtin");		\
}							\
							\
ATF_TC_BODY(invalid_builtin_##type, tc)			\
{							\
							\
	volatile int a = atoi("0");			\
	__builtin_##type(a);				\
}

INVALID_BUILTIN(ctz)
INVALID_BUILTIN(ctzl)
INVALID_BUILTIN(ctzll)
INVALID_BUILTIN(clz)
INVALID_BUILTIN(clzl)
INVALID_BUILTIN(clzll)

ATF_TC(load_invalid_value_bool);
ATF_TC_HEAD(load_invalid_value_bool, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=bool");
}

ATF_TC_BODY(load_invalid_value_bool, tc)
{
	volatile int a = atoi("10");
	volatile bool b = a;

	usleep(b ? 1 : 2);
}

ATF_TC(load_invalid_value_enum);
ATF_TC_HEAD(load_invalid_value_enum, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=enum");
}

ATF_TC_BODY(load_invalid_value_enum, tc)
{
	enum e { e1, e2, e3, e4 };
	volatile int a = atoi("10");
	volatile enum e E = a;

	usleep((E == e1) ? 1 : 2);
}

#ifdef __cplusplus
ATF_TC(missing_return);
ATF_TC_HEAD(missing_return, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=return");
}

static int
fun_missing_return(void)
{
}

ATF_TC_BODY(missing_return, tc)
{
	volatile int a = fun_missing_return();

	usleep(a ? 1 : 2);
}
#endif

ATF_TC(mul_overflow_signed);
ATF_TC_HEAD(mul_overflow_signed, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

ATF_TC_BODY(mul_overflow_signed, tc)
{
	volatile int a = INT_MAX;
	volatile int b = atoi("2");

	usleep((a * b) ? 1 : 2);
}

ATF_TC(mul_overflow_unsigned);
ATF_TC_HEAD(mul_overflow_unsigned, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

ATF_TC_BODY(mul_overflow_unsigned, tc)
{
	volatile unsigned int a = UINT_MAX;
	volatile unsigned int b = atoi("2");

	usleep((a * b) ? 1 : 2);
}

ATF_TC(negate_overflow_signed);
ATF_TC_HEAD(negate_overflow_signed, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

ATF_TC_BODY(negate_overflow_signed, tc)
{
	volatile int a = INT_MIN;

	usleep(-a ? 1 : 2);
}

ATF_TC(negate_overflow_unsigned);
ATF_TC_HEAD(negate_overflow_unsigned, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

ATF_TC_BODY(negate_overflow_unsigned, tc)
{
	volatile unsigned int a = UINT_MAX;

	usleep(-a ? 1 : 2);
}

#ifdef __clang
ATF_TC(nonnull_arg);
ATF_TC_HEAD(nonnull_arg, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=nullability-arg");
}

static void *
fun_nonnull_arg(void * _Nonnull ptr)
{

	return ptr;
}

ATF_TC_BODY(nonnull_arg, tc)
{
	volatile intptr_t a = atoi("0");

	usleep(fun_nonnull_arg((void *)a) ? 1 : 2);
}

ATF_TC(nonnull_assign);
ATF_TC_HEAD(nonnull_assign, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=nullability-assign");
}

static void *
fun_nonnull_assign(intptr_t a)
{
	volatile void *_Nonnull ptr;

	ptr = (void *)a;

	return ptr;
}

ATF_TC_BODY(nonnull_assign, tc)
{
	volatile intptr_t a = atoi("0");

	usleep(fun_nonnull_assign(a) ? 1 : 2);
}

ATF_TC(nonnull_return);
ATF_TC_HEAD(nonnull_return, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=nullability-return");
}

static void *
fun_nonnull_return(void)
{
	volatile intptr_t a = atoi("0");

	return (void *)ptr;
}

ATF_TC_BODY(nonnull_return, tc)
{

	usleep(fun_nonnull_return() ? 1 : 2);
}
#endif

ATF_TC(out_of_bounds);
ATF_TC_HEAD(out_of_bounds, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=bounds");
}

ATF_TC_BODY(out_of_bounds, tc)
{
	int A[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	volatile int a = atoi("10");

	usleep(A[a] ? 1 : 2);
}

ATF_TC(pointer_overflow);
ATF_TC_HEAD(pointer_overflow, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=pointer-overflow");
}

ATF_TC_BODY(pointer_overflow, tc)
{
	volatile uintptr_t a = UINTPTR_MAX;
	volatile uintptr_t b = atoi("1");
	volatile int *ptr = (int *)a;

	usleep((ptr + b) ? 1 : 2);
}

#ifndef __cplusplus
ATF_TC(shift_out_of_bounds_signednessbit);
ATF_TC_HEAD(shift_out_of_bounds_signednessbit, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=shift");
}

ATF_TC_BODY(shift_out_of_bounds_signednessbit, tc)
{
	volatile int32_t a = atoi("1");

	usleep((a << 31) ? 1 : 2);
}
#endif

ATF_TC(shift_out_of_bounds_signedoverflow);
ATF_TC_HEAD(shift_out_of_bounds_signedoverflow, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=shift");
}

ATF_TC_BODY(shift_out_of_bounds_signedoverflow, tc)
{
	volatile int32_t a = atoi("1");
	volatile int32_t b = atoi("30");
	a <<= b;

	usleep((a << 10) ? 1 : 2);
}

ATF_TC(shift_out_of_bounds_negativeexponent);
ATF_TC_HEAD(shift_out_of_bounds_negativeexponent, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=shift");
}

ATF_TC_BODY(shift_out_of_bounds_negativeexponent, tc)
{
	volatile int32_t a = atoi("1");
	volatile int32_t b = atoi("-10");

	usleep((a << b) ? 1 : 2);
}

ATF_TC(shift_out_of_bounds_toolargeexponent);
ATF_TC_HEAD(shift_out_of_bounds_toolargeexponent, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=shift");
}

ATF_TC_BODY(shift_out_of_bounds_toolargeexponent, tc)
{
	volatile int32_t a = atoi("1");
	volatile int32_t b = atoi("40");

	usleep((a << b) ? 1 : 2);
}

ATF_TC(sub_overflow_signed);
ATF_TC_HEAD(sub_overflow_signed, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=signed-integer-overflow");
}

ATF_TC_BODY(sub_overflow_signed, tc)
{
	volatile int a = INT_MIN;
	volatile int b = atoi("1");

	usleep((a - b) ? 1 : 2);
}

ATF_TC(sub_overflow_unsigned);
ATF_TC_HEAD(sub_overflow_unsigned, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=unsigned-integer-overflow");
}

ATF_TC_BODY(sub_overflow_unsigned, tc)
{
	volatile unsigned int a = atoi("0");
	volatile unsigned int b = atoi("1");

	usleep((a - b) ? 1 : 2);
}

ATF_TC(type_mismatch_nullptrderef);
ATF_TC_HEAD(type_mismatch_nullptrderef, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=null");
}

ATF_TC_BODY(type_mismatch_nullptrderef, tc)
{
	volatile intptr_t a = atoi("0");
	volatile int *b = (int *)a;

	usleep((*b) ? 1 : 2);
}

ATF_TC(type_mismatch_misaligned);
ATF_TC_HEAD(type_mismatch_misaligned, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=alignment");
}

ATF_TC_BODY(type_mismatch_misaligned, tc)
{
	volatile int8_t A[10] __aligned(4);
	volatile int *b;

	memset(A, 0, sizeof(A));

	b = &A[1];

	usleep((*b) ? 1 : 2);
}

ATF_TC(vla_bound_not_positive);
ATF_TC_HEAD(vla_bound_not_positive, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=vla-bound");
}

ATF_TC_BODY(vla_bound_not_positive, tc)
{
	volatile int a = atoi("-1");
	int A[a];

	usleep(A[0] ? 1 : 2);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, add_overflow_signed);
	ATF_TP_ADD_TC(tp, add_overflow_unsigned);
	ATF_TP_ADD_TC(tp, builtin_unreachable);
//	ATF_TP_ADD_TC(tp, cfi_bad_type);	// TODO
//	ATF_TP_ADD_TC(tp, cfi_check_fail);	// TODO
	ATF_TP_ADD_TC(tp, divrem_overflow_signed_div);
	ATF_TP_ADD_TC(tp, divrem_overflow_signed_mod);
//	ATF_TP_ADD_TC(tp, dynamic_type_cache_miss); // Not supported in uUBSan
//	ATF_TP_ADD_TC(tp, float_cast_overflow);	// TODO
#if defined(__cplusplus) && (defined(__x86_64__) || defined(__i386__))
	ATF_TP_ADD_TC(tp, function_type_mismatch);
#endif
	ATF_TP_ADD_TC(tp, invalid_builtin_ctz);
	ATF_TP_ADD_TC(tp, invalid_builtin_ctzl);
	ATF_TP_ADD_TC(tp, invalid_builtin_ctzll);
	ATF_TP_ADD_TC(tp, invalid_builtin_clz);
	ATF_TP_ADD_TC(tp, invalid_builtin_clzl);
	ATF_TP_ADD_TC(tp, invalid_builtin_clzll);
	ATF_TP_ADD_TC(tp, load_invalid_value_bool);
	ATF_TP_ADD_TC(tp, load_invalid_value_enum);
#ifdef __cplusplus
	ATF_TP_ADD_TC(tp, missing_return);
#endif
	ATF_TP_ADD_TC(tp, mul_overflow_signed);
	ATF_TP_ADD_TC(tp, mul_overflow_unsigned);
	ATF_TP_ADD_TC(tp, negate_overflow_signed);
	ATF_TP_ADD_TC(tp, negate_overflow_unsigned);
#ifdef __clang
	// Clang/LLVM specific extension
	// http://clang.llvm.org/docs/AttributeReference.html#nullability-attributes
	ATF_TP_ADD_TC(tp, nonnull_arg);
	ATF_TP_ADD_TC(tp, nonnull_assign);
	ATF_TP_ADD_TC(tp, nonnull_return);
#endif
	ATF_TP_ADD_TC(tp, out_of_bounds);
	ATF_TP_ADD_TC(tp, pointer_overflow);
#ifndef __cplusplus
	// Acceptable in C++11
	ATF_TP_ADD_TC(tp, shift_out_of_bounds_signednessbit);
#endif
	ATF_TP_ADD_TC(tp, shift_out_of_bounds_signedoverflow);
	ATF_TP_ADD_TC(tp, shift_out_of_bounds_negativeexponent);
	ATF_TP_ADD_TC(tp, shift_out_of_bounds_toolargeexponent);
	ATF_TP_ADD_TC(tp, sub_overflow_signed);
	ATF_TP_ADD_TC(tp, sub_overflow_unsigned);
	ATF_TP_ADD_TC(tp, type_mismatch_nullptrderef);
	ATF_TP_ADD_TC(tp, type_mismatch_misaligned);
	ATF_TP_ADD_TC(tp, vla_bound_not_positive);

	return atf_no_error();
}
