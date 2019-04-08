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
static int
pinspect_enable(struct proc *p, struct lwp *l)
{
	int error = 0;

	KASSERT(l == curlwp)

	mutex_enter(p->p_lock);
	if (ISSET(p->p_sflag, PS_INSPECTING)) {
		error = EBUSY;
		goto err;
	}

	KASSERT(!ISSET(l->l_pflag, LP_INSPECTOR));

	SET(p->p_sflag, PS_INSPECTING);
	SET(l->l_pflag, LP_INSPECTOR);

err:
	mutex_exit(p->p_lock);
	return error;
}

static int
pinspect_disable(struct proc *p, struct lwp *l)
{
	int error = 0;

	KASSERT(l == curlwp)

	mutex_enter(p->p_lock);
	if (!ISSET(p->p_sflag, PS_INSPECTING)) {
		error = EINVAL;
		goto err;
	}

	KASSERT(ISSET(l->l_pflag, LP_INSPECTOR));

	CLR(p->p_sflag, PS_INSPECTING);
	CLR(l->l_pflag, LP_INSPECTOR);

err:
	mutex_exit(p->p_lock);

	return error;
}

static int
pinspect_getcontext(struct proc *p, ucontext_t *ucp, lwpid_t lid)
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

int
do_pinspect(struct pinspect_methods *ptm, struct lwp *l, int req,
    void *addr, int data, register_t *retval)
{
	struct proc *p = l->l_proc;

	*retval = 0;

	case (req) {
	case PI_ENABLE:
		return pinspect_enable(p, l);
	case PI_DISABLE:
		return pinspect_disable(p, l);
	case PI_GETCONTEXT:
		return ptm->ptm_getcontext(p, addr, data);
	default:
		return EINVAL;
	}
}
#endif /* PINSPECT */

MODULE(MODULE_CLASS_EXEC, pinspect_common, "");

static int
pinspect_common_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
	case MODULE_CMD_FINI:
		return 0;
	default:
		return ENOTTY;
	}
}
