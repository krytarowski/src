/*	$NetBSD$	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kamil Rytarowski.
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


#include <sys/param.h>
#include <sys/types.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/syscall.h>
#include <sys/module.h>

static struct pinspect_methods native_ptm = {
	.ptm_getcontext = pinspect_getcontext,
};

static const struct syscall_package pinspect_syscalls[] = {
	{ SYS_pinspect, 0, (sy_call_t *)sys_pinspect },
	{ 0, 0, NULL },
};

/*
 * Process self INTro SPECTion system call.
 */
int
sys_ptrace(struct lwp *l, const struct sys_ptrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) req;
		syscallarg(void *) addr;
		syscallarg(int) data;
	} */

	return do_ptrace(&native_ptm, l, SCARG(uap, req), SCARG(uap, addr),
	    SCARG(uap, data), retval);
}

MODULE(MODULE_CLASS_EXEC, pinspect, "pinspect_common");

static int
pinspect_modcmd(modcmd_t cmd, void *arg)
{
	int error;
	
	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(&emul_netbsd, pinspect_syscalls);
		break;
	case MODULE_CMD_FINI:
		error = syscall_disestablish(&emul_netbsd, pinspect_syscalls);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
