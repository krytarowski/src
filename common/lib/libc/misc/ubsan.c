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


/*
 * The micro UBSan implementation for the userland (uUBSan) and kernel (kUBSan).
 */

#include <sys/cdefs.h>
#if defined(_KERNEL)
__KERNEL_RCSID(0, "$NetBSD$");
#elif defined(_LIBC)
__RCSID("$NetBSD$");
#endif

#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/stdarg.h>
#elif defined(_LIBC)
#include "namespace.h"
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include "extern.h"
#endif

#ifdef _LIBC
static int ubsan_flags = -1;

#define UBSAN_ABORT	__BIT(0)
#define UBSAN_STDOUT	__BIT(1)
#define UBSAN_STDERR	__BIT(2)
#define UBSAN_SYSLOG	__BIT(3)
#endif

/* Local utility functions */
static void report(const char *);

/* Undefined Behavior specific defines and structures */

#define KIND_INTEGER	0
#define KIND_FLOAT	1
#define KIND_UNKNOWN	UCHAR_MAX

struct CSourceLocation {
	const char *mFilename;
	uint32_t mLine;
	uint32_t mColumn;
};

struct CTypeDescriptor {
	uint16_t mTypeKind;
	uint16_t mTypeInfo;
	uint8_t mTypeName[1];
};

struct COverflowData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
};

struct CUnreachableData {
	struct CSourceLocation mLocation;
};

struct CCFICheckFailData {
	uint8_t mCheckKind;
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
};

struct CDynamicTypeCacheMissData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
	void *mTypeInfo;
	uint8_t mTypeCheckKind;
};

struct CFunctionTypeMismatchData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
};

struct CInvalidBuiltinData {
	struct CSourceLocation mLocation;
	uint8_t mKind;
};

struct CInvalidValueData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
};

struct CNonNullArgData {
	struct CSourceLocation mLocation;
	struct CSourceLocation mAttributeLocation;
	int mArgIndex;
};

struct COutOfBoundsData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mArrayType;
	struct CTypeDescriptor *mIndexType;
};

struct CPointerOverflowData {
	struct CSourceLocation mLocation;
};

struct CShiftOutOfBoundsData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mLHSType;
	struct CTypeDescriptor *mRHSType;
};

struct TypeMismatchData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
	uint8_t mLogAlignment;
	uint8_t mTypeCheckKind;
};

struct CVLABoundData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
};

/* Public symbols used in the instrumentation of the code generation part */
void __ubsan_handle_add_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_add_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_builtin_unreachable(struct CUnreachableData *pData);
void __ubsan_handle_cfi_bad_type(struct CCFICheckFailData *pData, unsigned long ulVtable, bool bValidVtable, bool FromUnrecoverableHandler, unsigned long ProgramCounter, unsigned long FramePointer);
void __ubsan_handle_cfi_check_fail(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable);
void __ubsan_handle_cfi_check_fail_abort(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable);
void __ubsan_handle_divrem_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_divrem_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_dynamic_type_cache_miss(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash);
void __ubsan_handle_dynamic_type_cache_miss_abort(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash);
void __ubsan_handle_float_cast_overflow(void *pData, unsigned long ulFrom);
void __ubsan_handle_float_cast_overflow_abort(void *pData, unsigned long ulFrom);
void __ubsan_handle_function_type_mismatch(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction);
void __ubsan_handle_function_type_mismatch_abort(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction);
void __ubsan_handle_invalid_builtin(struct CInvalidBuiltinData *pData);
void __ubsan_handle_invalid_builtin_abort(struct CInvalidBuiltinData *pData);
void __ubsan_handle_load_invalid_value(struct CInvalidValueData *pData, unsigned long ulVal);
void __ubsan_handle_load_invalid_value_abort(struct CInvalidValueData *pData, unsigned long ulVal);
void __ubsan_handle_missing_return(struct CUnreachableData *pData);
void __ubsan_handle_mul_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_mul_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_negate_overflow(struct OverflowData *pData, unsigned long ulOldVal);
void __ubsan_handle_negate_overflow_abort(struct OverflowData *pData, unsigned long ulOldVal);
void __ubsan_handle_nonnull_arg(struct CNonNullArgData *pData);
void __ubsan_handle_nonnull_arg_abort(struct CNonNullArgData *pData);
void __ubsan_handle_nonnull_return_v1(struct CNonNullArgData *pData);
void __ubsan_handle_nonnull_return_v1_abort(struct CNonNullArgData *pData);
void __ubsan_handle_nullability_arg(struct CNonNullArgData *pData);
void __ubsan_handle_nullability_arg_abort(struct CNonNullArgData *pData);
void __ubsan_handle_nullability_return_v1(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
void __ubsan_handle_nullability_return_v1_abort(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer);
void __ubsan_handle_out_of_bounds(struct COutOfBoundsData *pData, unsigned long ulIndex);
void __ubsan_handle_out_of_bounds_abort(struct COutOfBoundsData *pData, unsigned long ulIndex);
void __ubsan_handle_pointer_overflow(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult);
void __ubsan_handle_pointer_overflow_abort(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult);
void __ubsan_handle_shift_out_of_bounds(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_shift_out_of_bounds_abort(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_sub_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_sub_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS);
void __ubsan_handle_type_mismatch_v1(struct CTypeMismatchData *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_v1_abort(struct CTypeMismatchData *pData, unsigned long ulPointer);
void __ubsan_handle_vla_bound_not_positive(struct CVLABoundData *pData, unsigned long ulBound);
void __ubsan_handle_vla_bound_not_positive_abort(struct CVLABoundData *pData, unsigned long ulBound);
void __ubsan_get_current_report_data(const char **ppOutIssueKind, const char **ppOutMessage, const char **ppOutFilename, uint32_t *pOutLine, uint32_t *pOutCol, char **ppOutMemoryAddr);



static void
report(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#if defined(_KERNEL)
	vprintf(fmt, ap);
#elif defined(_LIBC)
	if (ubsan_flags == -1) {
		char buf[1024];
		char *p;

		ubsan_flags = UBSAN_STDERR;

		for (p = getenv_r("LIBC_UBSAN", buf, sizeof(buf));
		     p && *p; p++) {
			switch (*p) {
			case 'a':
				ubsan_flags |= UBSAN_ABORT;
				break;
			case 'A':
				ubsan_flags &= ~UBSAN_ABORT;
				break;
			case 'e':
				ubsan_flags |= UBSAN_STDERR;
				break;
			case 'E':
				ubsan_flags &= ~UBSAN_STDERR;
				break;
			case 'l':
				ubsan_flags |= UBSAN_SYSLOG;
				break;
			case 'L':
				ubsan_flags &= ~UBSAN_SYSLOG;
				break;
			case 'o':
				ubsan_flags |= UBSAN_STDOUT;
				break;
			case 'O':
				ubsan_flags &= ~UBSAN_STDOUT;
				break;
			default:
				break;
			}
		}
	}

	if (ubsan_flags & UBSAN_STDOUT) {
		vprintf(fmt, ap);
		fflush(stdout);
	}
	if (ubsan_flags & UBSAN_STDERR) {
		vfprintf(stderr, fmt, ap);
		fflush(stderr);
	}
	if (ubsan_flags & UBSAN_SYSLOG) {
		struct syslog_data sdata = SYSLOG_DATA_INIT;
		vsyslog_ss(LOG_DEBUG | LOG_USER, &sdata, msg, ap);
	}
	if (ubsan_flags & UBSAN_ABORT)
		abort();
#endif
	va_end(ap);
}
