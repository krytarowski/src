/*	$NetBSD: asan.c,v 1.1 2018/08/17 19:48:16 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard, and Siddharth Muralee.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/types.h>

#include <uvm/uvm.h>
#include <amd64/pmap.h>
#include <amd64/vmparam.h>

#define VIRTUAL_SHIFT		47	/* 48bit address space, cut half */
#define CANONICAL_BASE		0xFFFF800000000000

#define KASAN_SHADOW_SCALE_SHIFT	3
#define KASAN_SHADOW_SCALE_SIZE		(1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK		(KASAN_SHADOW_SCALE_SIZE - 1)

#define KASAN_SHADOW_SIZE	(1ULL << (VIRTUAL_SHIFT - KASAN_SHADOW_SCALE_SHIFT))
#define KASAN_SHADOW_START	(VA_SIGN_NEG((L4_SLOT_KASAN * NBPD_L4)))
#define KASAN_SHADOW_END	(KASAN_SHADOW_START + KASAN_SHADOW_SIZE)

typedef unsigned long	shad_t;

void kasan_shadow_map(void *, size_t);

#ifdef kasan_unused
static inline void *kasan_shad_to_addr(shad_t shad)
{
	return (void *)(shad << KASAN_SHADOW_SCALE_SHIFT);
}
#endif

static inline shad_t kasan_addr_to_shad(void *addr)
{
	vaddr_t va = (vaddr_t)addr;
	return ((shad_t)(va - CANONICAL_BASE) >> KASAN_SHADOW_SCALE_SHIFT);
}

static void
kasan_shadow_map_page(vaddr_t va)
{
	paddr_t pa;

	if (!pmap_valid_entry(L4_BASE[pl4_i(va)])) {
		pa = pmap_get_physpage();
		L4_BASE[pl4_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L3_BASE[pl3_i(va)])) {
		pa = pmap_get_physpage();
		L3_BASE[pl3_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L2_BASE[pl2_i(va)])) {
		pa = pmap_get_physpage();
		L2_BASE[pl2_i(va)] = pa | PG_KW | pmap_pg_nx | PG_V;
	}
	if (!pmap_valid_entry(L1_BASE[pl1_i(va)])) {
		pa = pmap_get_physpage();
		L1_BASE[pl1_i(va)] = pa | PG_KW | pmap_pg_g | pmap_pg_nx | PG_V;
	}
}

/*
 * Allocate the necessary stuff in the shadow, so that we can monitor the
 * passed area.
 */
void
kasan_shadow_map(void *addr, size_t size)
{
	size_t sz, npages, i;
	vaddr_t va;

	va = (vaddr_t)(KASAN_SHADOW_START + kasan_addr_to_shad(addr));
	sz = roundup(size, KASAN_SHADOW_SCALE_SIZE) / KASAN_SHADOW_SCALE_SIZE;
	va = rounddown(va, PAGE_SIZE);
	npages = roundup(sz, PAGE_SIZE) / PAGE_SIZE;

	KASSERT(va >= KASAN_SHADOW_START && va < KASAN_SHADOW_END);

	for (i = 0; i < npages; i++) {
		kasan_shadow_map_page(va + i * PAGE_SIZE);
	}
}

/* -------------------------------------------------------------------------- */

#ifdef __HAVE_PCPU_AREA
#error "PCPU area not allowed with KASAN"
#endif
#ifdef __HAVE_DIRECT_MAP
#error "DMAP not allowed with KASAN"
#endif

void kasan_init(void);

/*
 * Create the shadow mapping. We don't create the 'User' area, because we
 * exclude it from the monitoring. The 'Main' area is created dynamically
 * in pmap_growkernel.
 */
void
kasan_init(void)
{
	extern struct bootspace bootspace;
	size_t i;

	CTASSERT((KASAN_SHADOW_SIZE / NBPD_L4) == NL4_SLOT_KASAN);

	/* Kernel. */
	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_NONE) {
			continue;
		}
		kasan_shadow_map((void *)bootspace.segs[i].va,
		    bootspace.segs[i].sz);
	}

	/* Module map. */
	kasan_shadow_map((void *)bootspace.smodule,
	    (size_t)(bootspace.emodule - bootspace.smodule));
}

/* -------------------------------------------------------------------------- */

/*
 * Part of the compiler ABI.
 */
struct __asan_global_source_location {
	const char *filename;
	int line_no;
	int column_no;
};
struct __asan_global {
	const void *beg;		/* address of the global variable */
	size_t size;			/* size of the global variable */
	size_t size_with_redzone;	/* size with the redzone */
	const void *name;		/* name of the variable */
	const void *module_name;	/* name of the module where the var is declared */
	unsigned long has_dynamic_init;	/* the var has dyn initializer (c++) */
	struct __asan_global_source_location *location;
	uintptr_t odr_indicator;	/* the address of the ODR indicator symbol */
};

void __asan_register_globals(struct __asan_global *, size_t);
void
__asan_register_globals(struct __asan_global *globals, size_t size)
{
}

void __asan_unregister_globals(struct __asan_global *, size_t);
void
__asan_unregister_globals(struct __asan_global *globals, size_t size)
{
}

#define ASAN_LOAD_STORE(size)					\
	void __asan_load##size(unsigned long);			\
	void __asan_load##size(unsigned long addr)		\
	{							\
	} 							\
	void __asan_load##size##_noabort(unsigned long);	\
	void __asan_load##size##_noabort(unsigned long addr)	\
	{							\
	}							\
	void __asan_store##size(unsigned long);			\
	void __asan_store##size(unsigned long addr)		\
	{							\
	}							\
	void __asan_store##size##_noabort(unsigned long);	\
	void __asan_store##size##_noabort(unsigned long addr)	\
	{							\
	}

ASAN_LOAD_STORE(1);
ASAN_LOAD_STORE(2);
ASAN_LOAD_STORE(4);
ASAN_LOAD_STORE(8);
ASAN_LOAD_STORE(16);

void __asan_loadN(unsigned long, size_t);
void
__asan_loadN(unsigned long addr, size_t size)
{
}

void __asan_loadN_noabort(unsigned long, size_t);
void
__asan_loadN_noabort(unsigned long addr, size_t size)
{
}

void __asan_storeN(unsigned long, size_t);
void
__asan_storeN(unsigned long addr, size_t size)
{
}

void __asan_storeN_noabort(unsigned long, size_t);
void
__asan_storeN_noabort(unsigned long addr, size_t size)
{
}

void __asan_handle_no_return(void);
void
__asan_handle_no_return(void)
{
}

void __asan_poison_stack_memory(const void *, size_t);
void
__asan_poison_stack_memory(const void *addr, size_t size)
{
}

void __asan_unpoison_stack_memory(const void *, size_t);
void
__asan_unpoison_stack_memory(const void *addr, size_t size)
{
}

void __asan_alloca_poison(unsigned long, size_t);
void
__asan_alloca_poison(unsigned long addr, size_t size)
{
}

void __asan_allocas_unpoison(const void *, const void *);
void
__asan_allocas_unpoison(const void *stack_top, const void *stack_bottom)
{
}

#define ASAN_SET_SHADOW(byte) \
	void __asan_set_shadow_##byte(void *, size_t);			\
	void __asan_set_shadow_##byte(void *addr, size_t size)		\
	{								\
	}

ASAN_SET_SHADOW(00);
ASAN_SET_SHADOW(f1);
ASAN_SET_SHADOW(f2);
ASAN_SET_SHADOW(f3);
ASAN_SET_SHADOW(f5);
ASAN_SET_SHADOW(f8);
