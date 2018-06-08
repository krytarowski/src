#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/cprng.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/filedesc.h>

#define DEFINE_ASAN_LOAD_STORE(size)				\
	void __asan_load##size(unsigned long);			\
	void __asan_load##size(unsigned long addr)		\
	{}							\
	void __asan_load##size##_noabort(unsigned long);	\
	void __asan_load##size##_noabort(unsigned long addr)	\
	{}							\
	void __asan_store##size(unsigned long);			\
	void __asan_store##size(unsigned long addr)		\
	{}							\
	void __asan_store##size##_noabort(unsigned long);	\
	void __asan_store##size##_noabort(unsigned long addr)	\
	{}							\


DEFINE_ASAN_LOAD_STORE(1);
DEFINE_ASAN_LOAD_STORE(2);
DEFINE_ASAN_LOAD_STORE(4);
DEFINE_ASAN_LOAD_STORE(8);
DEFINE_ASAN_LOAD_STORE(16);

void __asan_loadN(unsigned long, size_t);
void __asan_loadN(unsigned long addr, size_t size)
{}

void __asan_loadN_noabort(unsigned long, size_t);
void __asan_loadN_noabort(unsigned long addr, size_t size)
{}

void __asan_storeN(unsigned long, size_t);
void __asan_storeN(unsigned long addr, size_t size)
{}

void __asan_storeN_noabort(unsigned long, size_t);
void __asan_storeN_noabort(unsigned long addr, size_t size)
{}

/* to shut up compiler complaints */

void __asan_handle_no_return(void);
void __asan_handle_no_return(void) {}
