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
__RCSID("$NetBSD$");

#if defined(_KERNEL)
#include <sys/types.h>
#include <sys/stdarg.h>
#elif defined(_LIBC)
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#endif

#ifdef _LIBC
static bool initialized;
static bool halt_on_error;
static char exe_name[PATH_MAX];
static bool log_exe_name;
static bool log_to_syslog;
#endif

static void print_report(const char *);




static void
print_report(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#if defined(_KERNEL)
	if (halt_on_error)
		vpanic(fmt, ap);
	else
		vprintf(fmt, ap);
#elif defined(_LIBC)
	vfprintf(stderr, fmt, ap);
	if (log_to_syslog) {
		struct syslog_data sdata = SYSLOG_DATA_INIT;
		vsyslog(halt_on_error ? LOG_ERR : LOG_WARNING, &sdata, msg, ap);
	}
	if (halt_on_error) {
		struct sigaction sa;
		sigset_t mask;

		(void)sigfillset(&mask);
		(void)sigdelset(&mask, SIGABRT);
		(void)sigprocmask(SIG_BLOCK, &mask, NULL);

		(void)memset(&sa, 0, sizeof(sa));
		(void)sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		(void)sigaction(SIGABRT, &sa, NULL);
		(void)raise(SIGABRT);
		_exit(127);
	}
#endif
	va_end(ap);
}

#ifdef _LIBC
void __section(".text.startup")
__libc_ubsan_init(void)
{
}
#endif
