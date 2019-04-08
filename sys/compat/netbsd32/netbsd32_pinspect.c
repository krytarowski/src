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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#if defined(_KERNEL_OPT)
#include "opt_pinspect.h"
#include "opt_compat_netbsd.h"
#endif

#include <sys/param.h>
#include <sys/module.h>
#include <sys/pinspect.h>
#include <sys/syscallvar.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

/*
 * PINSPECT methods
 */

static int
netbsd32_getcontext(struct proc *p, ucontext_t *ucp, lwpid_t lid)
{
        ucontext_t uc;
        struct lwp *lt;
        int error;
        
        memset(&uc, 0, sizeof(uc));
        
        mutex_enter(p->p_lock);
        if (!ISSET(p->p_sflag, PS_INSPECTING)) {
                error = EINVAL;
                goto err;
        }
        lt = lwp_find(p, lid);
        if (lt == NULL) {
                error = ESRCH;
                goto err;
        }
        getucontext(lt, &uc);
        mutex_exit(p->p_lock); 
                
        return copyout(&uc, ucp, sizeof(*ucp));
err:
        mutex_exit(p->p_lock);
        return error;
}

static struct pinspect_methods netbsd32_ptm = {
	.ptm_getcontext = netbsd32_getcontext
};

int
netbsd32_pinspect(struct lwp *l, const struct netbsd32_pinspect_args *uap,
    register_t *retval)
{
        /* {
                syscallarg(int) req;
                syscallarg(netbsd32_voidp *) addr;
                syscallarg(int) data;
        } */

	return do_pinspect(&netbsd32_ptm, l, SCARG(uap, req),
            SCARG_P32(uap, addr), SCARG(uap, data), retval);
}

static const struct syscall_package compat_pinspect_syscalls[] = {
	{ NETBSD32_SYS_netbsd32_pinspect, 0, (sy_call_t *)netbsd32_pinspect },
	{ 0, 0, NULL },
};

#define DEPS "compat_netbsd32,pinspect_common"

MODULE(MODULE_CLASS_EXEC, compat_netbsd32_pinspect, DEPS);

static int
compat_netbsd32_pinspect_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = syscall_establish(&emul_netbsd32,
		    compat_pinspect_syscalls);
		break;
	case MODULE_CMD_FINI:
		error = syscall_disestablish(&emul_netbsd32,
		    compat_pinspect_syscalls);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}
