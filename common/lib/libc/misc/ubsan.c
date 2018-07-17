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
 * The uBSSan versions is suitable for inclusion into libc or used standalone
 * with ATF tests.
 *
 * This file due to long symbol names and licensing reasons does not fully
 * follow the KNF style with 80-column limit. Hungarian style variables
 * and function names are on the same purpose (Pascal and Snake style names,
 * are used in different implementations).
 */

#include <sys/cdefs.h>
#if defined(_KERNEL)
__KERNEL_RCSID(0, "$NetBSD$");
#else
__RCSID("$NetBSD$");
#endif

#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/stdarg.h>
#define ASSERT(x) KASSERT(x)
#else
#if defined(_LIBC)
#include "namespace.h"
#endif
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#if defined(_LIBC)
#include "extern.h"
#define ubsan_vsyslog vsyslog_ss
#define ASSERT(x) _DIAGASSERT(x)
#else
#define ubsan_vsyslog vsyslog_r
#define ASSERT(x) assert(x)
#endif
/* These macros are available in _KERNEL only */
#define SET(t, f)	((t) |= (f))
#define ISSET(t, f)	((t) & (f))
#define CLR(t, f)	((t) &= ~(f))
#endif

#define ACK_REPORTED	__BIT(31)

#define MUL_CHARACTER	0x2a
#define PLUS_CHARACTER	0x2b
#define MINUS_CHARACTER	0x2d
#define DIV_CHARACTER	0x2f

#define NUMBER_MAXLEN	128
#define LOCATION_MAXLEN	(PATH_MAX + 32 /* ':LINE:COLUMN' */)

#define WIDTH_8		8
#define WIDTH_16	16
#define WIDTH_32	32
#define WIDTH_64	64
#define WIDTH_80	80
#define WIDTH_96	96
#define WIDTH_128	128

#define NUMBER_SIGNED_BIT	1U

#if __SIZEOF_INT128__
typedef __int128 longest;
typedef unsigned __int128 ulongest;
#endif

#ifndef _KERNEL
static int ubsan_flags = -1;
#define UBSAN_ABORT	__BIT(0)
#define UBSAN_STDOUT	__BIT(1)
#define UBSAN_STDERR	__BIT(2)
#define UBSAN_SYSLOG	__BIT(3)
#endif

/* Undefined Behavior specific defines and structures */

#define KIND_INTEGER	0
#define KIND_FLOAT	1
#define KIND_UNKNOWN	UINT16_MAX

struct CSourceLocation {
	char *mFilename;
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

struct CNonNullReturnData {
	struct CSourceLocation mAttributeLocation;
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

struct CTypeMismatchData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
	unsigned long mLogAlignment;
	uint8_t mTypeCheckKind;
};

struct CTypeMismatchData_v1 {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
	uint8_t mLogAlignment;
	uint8_t mTypeCheckKind;
};

struct CVLABoundData {
	struct CSourceLocation mLocation;
	struct CTypeDescriptor *mType;
};


/* Local utility functions */
static void Report(bool, const char *, ...);
static bool isAlreadyReported(struct CSourceLocation *);
static void DeserializeLocation(char *, size_t, struct CSourceLocation *);
#ifdef __SIZEOF_INT128__
static void DeserializeLongest(char *, size_t, ulongest *);
#endif
static void DeserializeIntegerOverPointer(char *, size_t, struct CTypeDescriptor *, unsigned long *);
static void DeserializeIntegerInlined(char *, size_t, struct CTypeDescriptor *, unsigned long);
#ifndef _KERNEL
static void DeserializeFloatOverPointer(char *, size_t, struct CTypeDescriptor *, unsigned long *);
static void DeserializeFloatInlined(char *, size_t, struct CTypeDescriptor *, unsigned long);
#endif
static void DeserializeNumber(char *, char *, size_t, struct CTypeDescriptor *, unsigned long);

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
void __ubsan_handle_negate_overflow(struct COverflowData *pData, unsigned long ulOldVal);
void __ubsan_handle_negate_overflow_abort(struct COverflowData *pData, unsigned long ulOldVal);
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
void __ubsan_handle_type_mismatch(struct CTypeMismatchData *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_abort(struct CTypeMismatchData *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_v1(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer);
void __ubsan_handle_type_mismatch_v1_abort(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer);
void __ubsan_handle_vla_bound_not_positive(struct CVLABoundData *pData, unsigned long ulBound);
void __ubsan_handle_vla_bound_not_positive_abort(struct CVLABoundData *pData, unsigned long ulBound);
void __ubsan_get_current_report_data(const char **ppOutIssueKind, const char **ppOutMessage, const char **ppOutFilename, uint32_t *pOutLine, uint32_t *pOutCol, char **ppOutMemoryAddr);

static void
HandleOverflow(bool isFatal, struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS, int iOperation)
{
	char szLocation[LOCATION_MAXLEN];
	char szLHS[NUMBER_MAXLEN];
	char szRHS[NUMBER_MAXLEN];

	ASSERT(pData);

	if (isAlreadyReported(&pData->mLocation))
		return;

	DeserializeLocation(szLocation, LOCATION_MAXLEN, &pData->mLocation);
	DeserializeNumber(szLocation, szLHS, NUMBER_MAXLEN, pData->mType, ulLHS);
	DeserializeNumber(szLocation, szRHS, NUMBER_MAXLEN, pData->mType, ulRHS);

	Report(isFatal, "UBSan: Undefined Behavior in %s, %s integer overflow: %s %c %s cannot be represented in type %s\n",
	       szLocation, ISSET(pData->mType->mTypeInfo, NUMBER_SIGNED_BIT) ? "signed" : "unsigned", szLHS, iOperation, szRHS, pData->mType->mTypeName);
}

static void
HandleNegateOverflow(bool isFatal, struct COverflowData *pData, unsigned long ulOldValue)
{
	char szLocation[LOCATION_MAXLEN];
	char szOldValue[NUMBER_MAXLEN];

	ASSERT(pData);

	if (isAlreadyReported(&pData->mLocation))
		return;

	DeserializeLocation(szLocation, LOCATION_MAXLEN, &pData->mLocation);
	DeserializeNumber(szLocation, szOldValue, NUMBER_MAXLEN, pData->mType, ulOldValue);

	Report(isFatal, "UBSan: Undefined Behavior in %s, negation of %s cannot be represented in type %s\n",
	       szLocation, szOldValue, pData->mType->mTypeName);
}

/* Definions of public symbols emitted by the instrumentation code */

void
__ubsan_handle_add_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);

	HandleOverflow(false, pData, ulLHS, ulRHS, PLUS_CHARACTER);
}

void
__ubsan_handle_add_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);

	HandleOverflow(true, pData, ulLHS, ulRHS, PLUS_CHARACTER);
}

void
__ubsan_handle_builtin_unreachable(struct CUnreachableData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_cfi_bad_type(struct CCFICheckFailData *pData, unsigned long ulVtable, bool bValidVtable, bool FromUnrecoverableHandler, unsigned long ProgramCounter, unsigned long FramePointer)
{

	ASSERT(pData);
}

void
__ubsan_handle_cfi_check_fail(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable)
{

	ASSERT(pData);
}

void
__ubsan_handle_cfi_check_fail_abort(struct CCFICheckFailData *pData, unsigned long ulValue, unsigned long ulValidVtable)
{

	ASSERT(pData);
}

void
__ubsan_handle_divrem_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);
}

void
__ubsan_handle_divrem_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);
}

void
__ubsan_handle_dynamic_type_cache_miss(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash)
{

	ASSERT(pData);
}

void
__ubsan_handle_dynamic_type_cache_miss_abort(struct CDynamicTypeCacheMissData *pData, unsigned long ulPointer, unsigned long ulHash)
{

	ASSERT(pData);
}

void
__ubsan_handle_float_cast_overflow(void *pData, unsigned long ulFrom)
{

	ASSERT(pData);
}

void
__ubsan_handle_float_cast_overflow_abort(void *pData, unsigned long ulFrom)
{

	ASSERT(pData);
}

void
__ubsan_handle_function_type_mismatch(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction)
{

	ASSERT(pData);
}

void
__ubsan_handle_function_type_mismatch_abort(struct CFunctionTypeMismatchData *pData, unsigned long ulFunction)
{

	ASSERT(pData);
}

void
__ubsan_handle_invalid_builtin(struct CInvalidBuiltinData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_invalid_builtin_abort(struct CInvalidBuiltinData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_load_invalid_value(struct CInvalidValueData *pData, unsigned long ulVal)
{

	ASSERT(pData);
}

void
__ubsan_handle_load_invalid_value_abort(struct CInvalidValueData *pData, unsigned long ulVal)
{

	ASSERT(pData);
}

void
__ubsan_handle_missing_return(struct CUnreachableData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_mul_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);

	HandleOverflow(false, pData, ulLHS, ulRHS, MUL_CHARACTER);
}

void
__ubsan_handle_mul_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);

	HandleOverflow(true, pData, ulLHS, ulRHS, MUL_CHARACTER);
}

void
__ubsan_handle_negate_overflow(struct COverflowData *pData, unsigned long ulOldValue)
{

	ASSERT(pData);

	HandleNegateOverflow(false, pData, ulOldValue);
}

void
__ubsan_handle_negate_overflow_abort(struct COverflowData *pData, unsigned long ulOldValue)
{

	ASSERT(pData);

	HandleNegateOverflow(true, pData, ulOldValue);
}

void
__ubsan_handle_nonnull_arg(struct CNonNullArgData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_nonnull_arg_abort(struct CNonNullArgData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_nonnull_return_v1(struct CNonNullArgData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_nonnull_return_v1_abort(struct CNonNullArgData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_nullability_arg(struct CNonNullArgData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_nullability_arg_abort(struct CNonNullArgData *pData)
{

	ASSERT(pData);
}

void
__ubsan_handle_nullability_return_v1(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer)
{

	ASSERT(pData);
	ASSERT(pLocationPointer);
}

void
__ubsan_handle_nullability_return_v1_abort(struct CNonNullReturnData *pData, struct CSourceLocation *pLocationPointer)
{

	ASSERT(pData);
	ASSERT(pLocationPointer);
}

void
__ubsan_handle_out_of_bounds(struct COutOfBoundsData *pData, unsigned long ulIndex)
{

	ASSERT(pData);
}

void
__ubsan_handle_out_of_bounds_abort(struct COutOfBoundsData *pData, unsigned long ulIndex)
{

	ASSERT(pData);
}

void
__ubsan_handle_pointer_overflow(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult)
{

	ASSERT(pData);
}

void
__ubsan_handle_pointer_overflow_abort(struct CPointerOverflowData *pData, unsigned long ulBase, unsigned long ulResult)
{

	ASSERT(pData);
}

void
__ubsan_handle_shift_out_of_bounds(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);
}

void
__ubsan_handle_shift_out_of_bounds_abort(struct CShiftOutOfBoundsData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);
}

void
__ubsan_handle_sub_overflow(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);

	HandleOverflow(false, pData, ulLHS, ulRHS, MINUS_CHARACTER);
}

void
__ubsan_handle_sub_overflow_abort(struct COverflowData *pData, unsigned long ulLHS, unsigned long ulRHS)
{

	ASSERT(pData);

	HandleOverflow(true, pData, ulLHS, ulRHS, MINUS_CHARACTER);
}

void
__ubsan_handle_type_mismatch(struct CTypeMismatchData *pData, unsigned long ulPointer)
{

	ASSERT(pData);
}

void
__ubsan_handle_type_mismatch_abort(struct CTypeMismatchData *pData, unsigned long ulPointer)
{

	ASSERT(pData);
}

void
__ubsan_handle_type_mismatch_v1(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer)
{

	ASSERT(pData);
}

void
__ubsan_handle_type_mismatch_v1_abort(struct CTypeMismatchData_v1 *pData, unsigned long ulPointer)
{

	ASSERT(pData);
}

void
__ubsan_handle_vla_bound_not_positive(struct CVLABoundData *pData, unsigned long ulBound)
{

	ASSERT(pData);
}

void
__ubsan_handle_vla_bound_not_positive_abort(struct CVLABoundData *pData, unsigned long ulBound)
{

	ASSERT(pData);
}

void
__ubsan_get_current_report_data(const char **ppOutIssueKind, const char **ppOutMessage, const char **ppOutFilename, uint32_t *pOutLine, uint32_t *pOutCol, char **ppOutMemoryAddr)
{
}


static void
Report(bool isFatal, const char *pFormat, ...)
{
	va_list ap;

	ASSERT(pFormat);

	va_start(ap, pFormat);
#if defined(_KERNEL)
	if (ifFatal)
		vpanic(pFormat, ap);
	else
		vprintf(pFormat, ap);
#else
	if (ubsan_flags == -1) {
		char buf[1024];
		char *p;

		ubsan_flags = UBSAN_STDERR;

		if (getenv_r("LIBC_UBSAN", buf, sizeof(buf)) != -1) {
			for (p = buf; *p; p++) {
				switch (*p) {
				case 'a':
					SET(ubsan_flags, UBSAN_ABORT);
					break;
				case 'A':
					CLR(ubsan_flags, UBSAN_ABORT);
					break;
				case 'e':
					SET(ubsan_flags, UBSAN_STDERR);
					break;
				case 'E':
					CLR(ubsan_flags, UBSAN_STDERR);
					break;
				case 'l':
					SET(ubsan_flags, UBSAN_SYSLOG);
					break;
				case 'L':
					CLR(ubsan_flags, UBSAN_SYSLOG);
					break;
				case 'o':
					SET(ubsan_flags, UBSAN_STDOUT);
					break;
				case 'O':
					CLR(ubsan_flags, UBSAN_STDOUT);
					break;
				default:
					break;
				}
			}
		}
	}

	if (ISSET(ubsan_flags, UBSAN_STDOUT)) {
		vprintf(pFormat, ap);
		fflush(stdout);
	}
	if (ISSET(ubsan_flags, UBSAN_STDERR)) {
		vfprintf(stderr, pFormat, ap);
		fflush(stderr);
	}
	if (ISSET(ubsan_flags, UBSAN_SYSLOG)) {
		struct syslog_data SyslogData = SYSLOG_DATA_INIT;
		ubsan_vsyslog(LOG_DEBUG | LOG_USER, &SyslogData, pFormat, ap);
	}
	if (isFatal || ISSET(ubsan_flags, UBSAN_ABORT)) {
		abort();
		/* NOTREACHED */
	}
#endif
	va_end(ap);
}

static bool
isAlreadyReported(struct CSourceLocation *pLocation)
{
	/*
	 * This code is shared between libc, kernel and standalone usage.
	 * It shall work in early bootstrap phase of both of them.
	 */

	uint32_t siOldValue;
	volatile uint32_t *pLine;

	ASSERT(pLocation);

	pLine = &pLocation->mLine;

	do {
		siOldValue = *pLine;
	} while (__sync_val_compare_and_swap(pLine, siOldValue, siOldValue | ACK_REPORTED) != siOldValue);

	return ISSET(siOldValue, ACK_REPORTED);
}

static size_t
zDeserializeTypeWidth(struct CTypeDescriptor *pType)
{
	size_t zWidth;

	ASSERT(pType);

	switch (pType->mTypeKind) {
	case KIND_INTEGER:
		zWidth = __BIT(__SHIFTOUT(pType->mTypeInfo, ~NUMBER_SIGNED_BIT));
		break;
	case KIND_FLOAT:
		zWidth = pType->mTypeInfo;
		break;
	default:
		Report(true, "UBSan: Unknown variable type %#04" PRIx16 "\n", pType->mTypeKind);
		/* NOTREACHED */
	}

	/* Invalid width will be transformed to 0 */
	ASSERT(zWidth > 0);

	return zWidth;
}

static void
DeserializeLocation(char *pBuffer, size_t zBUfferLength, struct CSourceLocation *pLocation)
{

	ASSERT(pLocation);
	ASSERT(pLocation->mFilename);

	snprintf(pBuffer, zBUfferLength, "%s:%" PRIu32 ":%" PRIu32, pLocation->mFilename, pLocation->mLine & ~ACK_REPORTED, pLocation->mColumn);
}

#ifdef __SIZEOF_INT128__
static void
DeserializeLongest(char *pBuffer, size_t zBUfferLength, ulongest *llliNumber)
{
	char szBuf[3]; /* 'XX\0' */
	char rgNumber[sizeof(ulongest)];
	size_t zI;

	memcpy(rgNumber, llliNumber, sizeof(ulongest));

	strlcpy(pBuffer, "Undecoded-128-bit-Integer-Type (0x", zBUfferLength);
	for (zI = 0; zI < sizeof(ulongest); zI++) {
		snprintf(szBuf, sizeof(szBuf), "%02" PRIx8, rgNumber[zI]);
		strlcat(pBuffer, szBuf, zBUfferLength);
	}
	strlcat(pBuffer, ")", zBUfferLength);
}
#endif

static void
DeserializeIntegerOverPointer(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long *pNumber)
{

	ASSERT(pBuffer);
	ASSERT(zBUfferLength > 0);
	ASSERT(pType);
	ASSERT(pNumber);
	/*
	 * This function handles 64-bit number over a pointer on 32-bit CPUs.
	 * 128-bit number are handled in DeserializeLongest().
	 */
	ASSERT((sizeof(*pNumber) * CHAR_BIT < WIDTH_64) && (zDeserializeTypeWidth(pType) == WIDTH_64));

	if (ISSET(pType->mTypeInfo, NUMBER_SIGNED_BIT)) {
		snprintf(pBuffer, zBUfferLength, "%" PRId64, *(int64_t *)pNumber);
	} else {
		snprintf(pBuffer, zBUfferLength, "%" PRIu64, *(uint64_t *)pNumber);
	}
}

static void
DeserializeIntegerInlined(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber)
{

	ASSERT(pBuffer);
	ASSERT(zBUfferLength > 0);
	ASSERT(pType);

	if (ISSET(pType->mTypeInfo, NUMBER_SIGNED_BIT)) {
		/* The serialized number is zero-extended */
		switch (zDeserializeTypeWidth(pType)) {
		case WIDTH_64:
			snprintf(pBuffer, zBUfferLength, "%" PRId64, (int64_t)(uint64_t)ulNumber);
			break;
		case WIDTH_32:
			snprintf(pBuffer, zBUfferLength, "%" PRId32, (int32_t)(uint32_t)ulNumber);
			break;
		case WIDTH_16:
			snprintf(pBuffer, zBUfferLength, "%" PRId16, (int16_t)(uint16_t)ulNumber);
			break;
		case WIDTH_8:
			snprintf(pBuffer, zBUfferLength, "%" PRId8, (int8_t)(uint8_t)ulNumber);
			break;
		}
	} else {
		snprintf(pBuffer, zBUfferLength, "%lu", ulNumber);
	}
}

#ifndef _KERNEL
static void
DeserializeFloatOverPointer(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long *pNumber)
{
	double D;
#ifdef __HAVE_LONG_DOUBLE
	long double LD;
#endif

	ASSERT(pBuffer);
	ASSERT(zBUfferLength > 0);
	ASSERT(pType);
	ASSERT(pNumber);
	/*
	 * This function handles 64-bit number over a pointer on 32-bit CPUs.
	 */
	ASSERT((sizeof(*pNumber) * CHAR_BIT < WIDTH_64) && (zDeserializeTypeWidth(pType) == WIDTH_64));
	ASSERT(sizeof(D) == sizeof(uint64_t));
#ifdef __HAVE_LONG_DOUBLE
	ASSERT(sizeof(LD) > sizeof(uint64_t));
#endif

	switch (zDeserializeTypeWidth(pType)) {
#ifdef __HAVE_LONG_DOUBLE
	case WIDTH_128:
	case WIDTH_96:
	case WIDTH_80:
		memcpy(&LD, pNumber, sizeof(long double));
		snprintf(pBuffer, zBUfferLength, "%Lg", LD);
		break;
#endif
	case WIDTH_64:
		memcpy(&D, pNumber, sizeof(double));
		snprintf(pBuffer, zBUfferLength, "%g", D);
		break;
	}
}

static void
DeserializeFloatInlined(char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber)
{
	float F;
	double D;
	uint32_t U32;

	ASSERT(pBuffer);
	ASSERT(zBUfferLength > 0);
	ASSERT(pType);
	ASSERT(sizeof(F) == sizeof(uint32_t));
	ASSERT(sizeof(D) == sizeof(uint64_t));

	switch (zDeserializeTypeWidth(pType)) {
	case WIDTH_64:
		memcpy(&D, &ulNumber, sizeof(double));
		snprintf(pBuffer, zBUfferLength, "%g", D);
		break;
	case WIDTH_32:
		/*
		 * On supported platforms sizeof(float)==sizeof(uint32_t)
		 * unsigned long is either 32 or 64-bit, cast it to 32-bit
		 * value in order to call memcpy(3) in an Endian-aware way.
		 */
		U32 = (uint32_t)ulNumber;
		memcpy(&F, &U32, sizeof(float));
		snprintf(pBuffer, zBUfferLength, "%g", D);
		break;
	case WIDTH_16:
		snprintf(pBuffer, zBUfferLength, "Undecoded-16-bit-Floating-Type (%#04" PRIx16 ")", (uint16_t)ulNumber);
		break;
	}
}
#endif

static void
DeserializeNumber(char *szLocation, char *pBuffer, size_t zBUfferLength, struct CTypeDescriptor *pType, unsigned long ulNumber)
{
	size_t zNumberWidth;

	ASSERT(pBuffer);
	ASSERT(szLocation);
	ASSERT(zBUfferLength > 0);
	ASSERT(pType);

	switch(pType->mTypeKind) {
	case KIND_INTEGER:
		zNumberWidth = zDeserializeTypeWidth(pType);
		switch (zNumberWidth) {
		default:
			Report(true, "UBSan: Unexpected %zu-Bit Type in %s\n", zNumberWidth, szLocation);
			/* NOTREACHED */
		case WIDTH_128:
#ifdef __SIZEOF_INT128__
			DeserializeLongest(pBuffer, zBUfferLength, (ulongest *)ulNumber);
#else
			Report(true, "UBSan: Unexpected 128-Bit Type in %s\n", szLocation);
			/* NOTREACHED */
#endif
			break;
		case WIDTH_64:
			if (sizeof(ulNumber) * CHAR_BIT < WIDTH_64) {
				DeserializeIntegerOverPointer(pBuffer, zBUfferLength, pType, (unsigned long *)ulNumber);
				break;
			}
		case WIDTH_32:
		case WIDTH_16:
		case WIDTH_8:
			DeserializeIntegerInlined(pBuffer, zBUfferLength, pType, ulNumber);
			break;
		}
		break;
	case KIND_FLOAT:
#ifdef _KERNEL
		Report(true, "UBSan: Unexpected Float Type in %s\n", szLocation);
		/* NOTREACHED */
#else
		zNumberWidth = zDeserializeTypeWidth(pType);
		switch (zNumberWidth) {
		default:
			Report(true, "UBSan: Unexpected %zu-Bit Type in %s\n", zNumberWidth, szLocation);
			/* NOTREACHED */
#ifdef __HAVE_LONG_DOUBLE
		case WIDTH_128:
		case WIDTH_96:
		case WIDTH_80:
			DeserializeFloatOverPointer(pBuffer, zBUfferLength, pType, (unsigned long *)ulNumber);
#endif
		case WIDTH_64:
			if (sizeof(ulNumber) * CHAR_BIT < WIDTH_64) {
				DeserializeFloatOverPointer(pBuffer, zBUfferLength, pType, (unsigned long *)ulNumber);
				break;
			}
		case WIDTH_32:
		case WIDTH_16:
			DeserializeFloatInlined(pBuffer, zBUfferLength, pType, ulNumber);
#endif
		}
		break;
	case KIND_UNKNOWN:
		Report(true, "UBSan: Unknown Type in %s\n", szLocation);
		/* NOTREACHED */
		break;
	}
}
