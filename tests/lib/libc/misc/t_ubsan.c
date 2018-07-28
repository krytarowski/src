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
	volatile int8_t a = INT8_MAX;
	volatile int8_t b = atoi("1");

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
	volatile uint8_t a = UINT8_MAX;
	volatile uint8_t b = atoi("1");

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
	volatile int8_t a = 1;
	volatile int8_t b = atoi("1");

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
	volatile int8_t a = INT8_MIN;
	volatile int8_t b = atoi("-1");

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
	volatile int8_t a = INT8_MIN;
	volatile int8_t b = atoi("-1");

	usleep((a % b) ? 1 : 2);
}

#ifdef __cplusplus
ATF_TC(function_type_mismatch);
ATF_TC_HEAD(function_type_mismatch, tc)
{
        atf_tc_set_md_var(tc, "descr",
	    "Checks -fsanitize=function");
}

static void
fun1(void)
{
}

ATF_TC_BODY(function_type_mismatch, tc)
{

	((void(*)(int)) ((uintptr_t)(fun1))) (1);
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
	volatile int8_t a = atoi("0");			\
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
	volatile int8_t a = atoi("10");
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
	volatile int8_t a = atoi("10");
	volatile enum e E = a;

	usleep((E == e1) ? 1 : 2);
}

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

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, add_overflow_signed);
	ATF_TP_ADD_TC(tp, add_overflow_unsigned);
	ATF_TP_ADD_TC(tp, builtin_unreachable);
//	ATF_TP_ADD_TC(tp, cfi_bad_type);	// TODO
//	ATF_TP_ADD_TC(tp, cfi_check_fail);	// TODO
	ATF_TP_ADD_TC(tp, divrem_overflow_signed_div);
	ATF_TP_ADD_TC(tp, divrem_overflow_signed_mod);
//	ATF_TP_ADD_TC(tp, dynamic_type_cache_miss); // Not supported in uBSan
//	ATF_TP_ADD_TC(tp, float_cast_overflow);	// TODO
#ifdef __cplusplus
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
	ATF_TP_ADD_TC(tp, missing_return); // C++
	ATF_TP_ADD_TC(tp, mul_overflow);
	ATF_TP_ADD_TC(tp, negate_overflow);
	ATF_TP_ADD_TC(tp, nonnull_arg);
	ATF_TP_ADD_TC(tp, nonnull_return);
	ATF_TP_ADD_TC(tp, nullability_arg);
	ATF_TP_ADD_TC(tp, nullability_return);
	ATF_TP_ADD_TC(tp, out_of_bounds);
	ATF_TP_ADD_TC(tp, pointer_overflow);
	ATF_TP_ADD_TC(tp, shift_out_of_bounds);
	ATF_TP_ADD_TC(tp, sub_overflow);
	ATF_TP_ADD_TC(tp, type_mismatch);
	ATF_TP_ADD_TC(tp, vla_bound_not_positive);

	return atf_no_error();
}
