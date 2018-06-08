/*	$NetBSD$*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/proc.h>
//#include <x86/pmap.h>

#include <uvm/uvm.h>

#include <amd64/pmap.h>
#include <amd64/vmparam.h>

#define BTSEG_HEAP 4
#define BTSEG_STACK 5

#define NO_OF_SEGS 6

#define MINKERN 0
#define MAXKERN 1
#define MINVM 2
#define MAXVM 3

/*
 *
 * To use this device you need to do:
 *     mknod /dev/kernel_map c 210 0
 *
 */

dev_type_open(kernel_map_open);
dev_type_close(kernel_map_close);
dev_type_read(kernel_map_read);

static struct cdevsw kernel_map_cdevsw = {
	.d_open = kernel_map_open,
	.d_close = kernel_map_close,
	.d_read = kernel_map_read,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER
};

struct seg_details {
        const char * name;
        int64_t * vaddr;
        int64_t size;
};

static struct seg_details kmap[6];

struct const_details {
        const char * name;
        int64_t * vaddr;
};

static struct const_details consts[6];

void DumpSegments(void);
void DumpConstants(void);
void initailize_names(void);

extern struct bootspace bootspace;

void
DumpSegments(void)
{
	size_t i;

        // Copy the addresses of the bootspace structure into our kernel structure

	for (i = 0; i < BTSPACE_NSEGS; i++) {
		if (bootspace.segs[i].type == BTSEG_NONE) {
			continue;
		}
	        kmap[bootspace.segs[i].type].vaddr = (void *)bootspace.segs[i].va;
                kmap[bootspace.segs[i].type].size = bootspace.segs[i].sz;
	}

        // Find the address and size of the Heap and the Stack and copy them.

        kmap[BTSEG_HEAP].vaddr = (void *)VM_MIN_KERNEL_ADDRESS;
        kmap[BTSEG_HEAP].size = VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS;
        kmap[BTSEG_STACK].vaddr = (void *)VM_MIN_KERNEL_ADDRESS;
        kmap[BTSEG_STACK].size = VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS;
}

void
DumpConstants(void)
{
        consts[MINKERN].vaddr = (void *)VM_MIN_KERNEL_ADDRESS;
        consts[MAXKERN].vaddr = (void *)VM_MAX_KERNEL_ADDRESS;
        consts[MINVM].vaddr = (void *)VM_MIN_ADDRESS;
        consts[MAXVM].vaddr = (void *)VM_MAX_ADDRESS;
}


void
initailize_names(void)
{

        kmap[BTSEG_NONE].name = "none";
	kmap[BTSEG_TEXT].name = "text";
	kmap[BTSEG_RODATA].name = "rodata";
	kmap[BTSEG_DATA].name = "data";
        kmap[BTSEG_HEAP].name = "heap";
        kmap[BTSEG_STACK].name = "stack";

        consts[MINKERN].name = "Minimum Kernel address" ;
        consts[MAXKERN].name = "Maximum Kernel address";
        consts[MINVM].name = "Minimum User address";
        consts[MAXVM].name = "Maximum User address";
}


int
kernel_map_open(dev_t self __unused, int flag __unused, int mode __unused,
           struct lwp *l __unused)
{

        // Initalize the name of all the regions

        initailize_names();

        // Dump all the regions into the structure.

        DumpSegments();

        // Dump all constants into the structure

        DumpConstants();

	return 0;
}

int
kernel_map_close(dev_t self __unused, int flag __unused, int mode __unused,
            struct lwp *l __unused)
{
	return 0;
}

int
kernel_map_read(dev_t self __unused, struct uio *uio, int flags __unused)
{
        size_t i;

        printf("\n------ Segments ------\n");

        for(i = 1; i < NO_OF_SEGS; i++) {
                printf("Segment %zu (%s): va=%p size=%zu\n", i,
                        kmap[i].name, kmap[i].vaddr, kmap[i].size);
        }

        printf("\n------ Kernel and User Map Constants ------\n");

        for(i = 0; i < NO_OF_SEGS - 2; i++) {
                printf("Constant %zu (%s): va=%p\n", i, consts[i].name,
                        consts[i].vaddr);
        }


	return 0;
}

MODULE(MODULE_CLASS_MISC, kernel_map, NULL);

static int
kernel_map_modcmd(modcmd_t cmd, void *arg __unused)
{
	/* The major should be verified and changed if needed to avoid
	 * conflicts with other devices. */
	int cmajor = 210, bmajor = -1;

	switch (cmd) {
	case MODULE_CMD_INIT:
		if (devsw_attach("kernel_map", NULL, &bmajor, &kernel_map_cdevsw,
		                 &cmajor))
			return ENXIO;
		return 0;
	case MODULE_CMD_FINI:
		devsw_detach(NULL, &kernel_map_cdevsw);
		return 0;
	default:
		return ENOTTY;
	}
}
