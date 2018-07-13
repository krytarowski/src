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
__RCSID("$NetBSD$");

#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/stdarg.h>
#elif defined(_LIBC)
#include "namespace.h"
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include "extern.h"
#endif

#ifdef _LIBC
static int	ubsan_flags = -1;

enum {
	UBSAN_ABORT	=	1<<0,
	UBSAN_STDOUT	=	1<<1,
	UBSAN_STDERR	=	1<<2,
	UBSAN_SYSLOG	=	1<<3
};
#endif

static void report(const char *);




static void
report(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#if defined(_KERNEL)
	if (halt_on_error)
		vpanic(fmt, ap);
	else
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
