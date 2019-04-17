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
__RCSID("$NetBSD$");

#include <sys/types.h>
#include <sys/pinspect.h>

#include "pthread.h"
#include "pthread_int.h"

/*
 * Process self INtro SPECTion
 */

int
pthread_stop_world_np(pthread_t * __restrict threads, size_t * __restrict num)
{
	pthread_t thr;
	int error = 0;
	size_t rnum = 0;

	/* Ensure integrity of the POSIX threads internals */
	pthread_rwlock_rdlock(&pthread__alltree_lock);

	/* Stap all other threads in this process */
	if ((error = pinspect(PI_ENABLE, NULL, 0)) != 0)
		goto err;

	if (threads || num) {
		RB_TREE_FOREACH(thr, &pthread__alltree) {
			/* skip dead and zombie threads */
			if (thr && thr->pt_state == PT_STATE_RUNNING) {
				if (threads)
					threads[rnum] = thr;
				++rnum;
			}
		}
		if (num)
			*num = rnum;
	}

err:
	pthread_rwlock_unlock(&pthread__alltree_lock);

	return error;
}

int
pthread_start_world_np(void)
{

	return pinspect(PI_DISABLE, NULL, 0);
}

int
pthread_getcontext_np(pthread_t thread, ucontext_t *uc)
{

	if (pthread__find(thread) != 0)
		return ESRCH;

	return pinspect(PI_GETCONTEXT, uc, thread->pt_lid);
}
