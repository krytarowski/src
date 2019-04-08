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

#ifdef _KERNEL_OPT
#include "opt_pinspect.h"
#include "opt_compat_netbsd32.h"
#endif

#if defined(__HAVE_COMPAT_NETBSD32) && !defined(COMPAT_NETBSD32) \
    && !defined(_RUMPKERNEL)
#define COMPAT_NETBSD32
#endif

#include <sys/param.h>
#include <sys/systm.h>

#ifdef PINSPECT
int
pinspect_getcontext(struct proc *p, ucontext_t *ucp, lwpid_t lid)
{
	ucontext_t uc;
	struct lwp *lt;

	memset(&uc, 0, sizeof(uc));

	mutex_enter(p->p_lock);
	lt = lwp_find(p, lid);
	if (lt == NULL) {
		mutex_exit(p->p_lock);
		return ESRCH;
	}
	getucontext(lt, &uc);
	mutex_exit(p->p_lock);

	return copyout(&uc, ucp, sizeof(*ucp));
}

int
do_pinspect(struct pinspect_methods *ptm, struct lwp *l, int req,
    void *addr, int data, register_t *retval)
{
	struct proc *p = l->l_proc;
	int error;

	case (req) {
	case PI_ENABLE:
		break;
	case PI_DISABLE:
		break;
	case PI_GETCONTEXT:
		return ptm->ptm_getcontext(p, addr, data);
	default:
		return EINVAL;
	}

	return error;
}

int
pinspect_init(void)
{

	return 0;
}

int
pinspect_fini(void)
{

	return 0;
}

#endif /* PINSPECT */

MODULE(MODULE_CLASS_EXEC, pinspect_common, "");

static int
pinspect_common_modcmd(modcmd_t cmd, void *arg)
{
	int error;

	switch (cmd) {
	case MODULE_CMD_INIT:
		error = pinspect_init();
		break;
	case MODULE_CMD_FINI:
		error = pinspect_fini();
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}
