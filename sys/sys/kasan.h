#ifndef _SYS_KASAN_H
#define _SYS_KASAN_H

#include <sys/cdefs.h>
#include <sys/types.h>

struct kmem_cache;
struct page;
struct vm_struct;
struct task_struct;

#include <amd64/kasan.h>

//#ifdef CONFIG_KASAN

/*
extern unsigned char kasan_zero_page[PAGE_SIZE];
extern pte_t kasan_zero_pte[PTRS_PER_PTE];
extern pmd_t kasan_zero_pmd[PTRS_PER_PMD];
extern pud_t kasan_zero_pud[PTRS_PER_PUD];
extern p4d_t kasan_zero_p4d[MAX_PTRS_PER_P4D];
*/
/*
void kasan_populate_zero_shadow(const void *shadow_start,
				const void *shadow_end);
*/
static inline void *kasan_mem_to_shadow(const void *addr)
{
	return (void *)(((unsigned long)addr >> KASAN_SHADOW_SCALE_SHIFT)
		+ KASAN_SHADOW_OFFSET);
}

/* Enable reporting bugs after kasan_disable_current() */
extern void kasan_enable_current(void);

/* Disable reporting bugs for current task */
extern void kasan_disable_current(void);
void kasan_unpoison_shadow(const void *address, size_t size);

void kasan_unpoison_task_stack(struct lwp *task);
void kasan_unpoison_stack_above_sp_to(const void *watermark);

void kasan_alloc_pages(struct page *page, unsigned int order);
void kasan_free_pages(struct page *page, unsigned int order);
/*
void kasan_cache_create(struct kmem_cache *cache, unsigned int *size,
			slab_flags_t *flags);
void kasan_cache_shrink(struct kmem_cache *cache);
void kasan_cache_shutdown(struct kmem_cache *cache);

void kasan_poison_slab(struct page *page);
void kasan_unpoison_object_data(struct kmem_cache *cache, void *object);
void kasan_poison_object_data(struct kmem_cache *cache, void *object);
void kasan_init_slab_obj(struct kmem_cache *cache, const void *object);

void kasan_kmalloc_large(const void *ptr, size_t size, gfp_t flags);
void kasan_kfree_large(void *ptr, unsigned long ip);
void kasan_poison_kfree(void *ptr, unsigned long ip);
void kasan_kmalloc(struct kmem_cache *s, const void *object, size_t size,
		  gfp_t flags);
void kasan_krealloc(const void *object, size_t new_size, gfp_t flags);

void kasan_slab_alloc(struct kmem_cache *s, void *object, gfp_t flags);
bool kasan_slab_free(struct kmem_cache *s, void *object, unsigned long ip);

struct kasan_cache {
	int alloc_meta_offset;
	int free_meta_offset;
};

int kasan_module_alloc(void *addr, size_t size);
void kasan_free_shadow(const struct vm_struct *vm);

size_t ksize(const void *);
static inline void kasan_unpoison_slab(const void *ptr) { ksize(ptr); }
size_t kasan_metadata_size(struct kmem_cache *cache);

bool kasan_save_enable_multi_shot(void);
void kasan_restore_multi_shot(bool enabled);
*/

#define KASAN_SHADOW_SCALE_SIZE (1UL << KASAN_SHADOW_SCALE_SHIFT)
#define KASAN_SHADOW_MASK       (KASAN_SHADOW_SCALE_SIZE - 1)

#define KASAN_FREE_PAGE         0xFF  /* page was freed */
#define KASAN_PAGE_REDZONE      0xFE  /* redzone for kmalloc_large allocations */
#define KASAN_KMALLOC_REDZONE   0xFC  /* redzone inside slub object */
#define KASAN_KMALLOC_FREE      0xFB  /* object was freed (kmem_cache_free/kfree) */
#define KASAN_GLOBAL_REDZONE    0xFA  /* redzone for global variable */

/*
 * Stack redzone shadow values
 * (Those are compiler's ABI, don't change them)
 */
#define KASAN_STACK_LEFT        0xF1
#define KASAN_STACK_MID         0xF2
#define KASAN_STACK_RIGHT       0xF3
#define KASAN_STACK_PARTIAL     0xF4
#define KASAN_USE_AFTER_SCOPE   0xF8

/*
 * alloca redzone shadow values
 */
#define KASAN_ALLOCA_LEFT	0xCA
#define KASAN_ALLOCA_RIGHT	0xCB

#define KASAN_ALLOCA_REDZONE_SIZE	32

/* Don't break randconfig/all*config builds */
#ifndef KASAN_ABI_VERSION
#define KASAN_ABI_VERSION 1
#endif

//int kasan_depth;

struct kasan_access_info {
	const void *access_addr;
	const void *first_bad_addr;
	size_t access_size;
	bool is_write;
	unsigned long ip;
};

/* The layout of struct dictated by compiler */
struct kasan_source_location {
	const char *filename;
	int line_no;
	int column_no;
};

/* The layout of struct dictated by compiler */
struct kasan_global {
	const void *beg;		/* Address of the beginning of the global variable. */
	size_t size;			/* Size of the global variable. */
	size_t size_with_redzone;	/* Size of the variable + size of the red zone. 32 bytes aligned */
	const void *name;
	const void *module_name;	/* Name of the module where the global variable is declared. */
	unsigned long has_dynamic_init;	/* This needed for C++ */
#if KASAN_ABI_VERSION >= 4
	struct kasan_source_location *location;
#endif
#if KASAN_ABI_VERSION >= 5
	char *odr_indicator;
#endif
};

/**
 * Structures to keep alloc and free tracks *
 */

#define KASAN_STACK_DEPTH 64
/*
struct kasan_track {
	u32 pid;
	depot_stack_handle_t stack;
};

struct kasan_alloc_meta {
	struct kasan_track alloc_track;
	struct kasan_track free_track;
};

struct qlist_node {
	struct qlist_node *next;
};
struct kasan_free_meta {
*/	/* This field is used while the object is in the quarantine.
	 * Otherwise it might be used for the allocator freelist.
	 */
/*	struct qlist_node quarantine_link;
};

struct kasan_alloc_meta *get_alloc_info(struct kmem_cache *cache,
					const void *object);
struct kasan_free_meta *get_free_info(struct kmem_cache *cache,
					const void *object);
*/
static inline const void *kasan_shadow_to_mem(const void *shadow_addr)
{
	return (void *)(((unsigned long)shadow_addr - KASAN_SHADOW_OFFSET)
		<< KASAN_SHADOW_SCALE_SHIFT);
}

void kasan_report(unsigned long addr, size_t size,
		bool is_write, unsigned long ip);
void kasan_report_invalid_free(void *object, unsigned long ip);
/*
#if defined(CONFIG_SLAB) || defined(CONFIG_SLUB)
void quarantine_put(struct kasan_free_meta *info, struct kmem_cache *cache);
void quarantine_reduce(void);
*/
void quarantine_remove_cache(struct kmem_cache *cache);
/*
#else
static inline void quarantine_put(struct kasan_free_meta *info,
				struct kmem_cache *cache) { }
static inline void quarantine_reduce(void) { }

static inline void quarantine_remove_cache(struct kmem_cache *cache) { }
//#endif
*/
/*
 * Exported functions for interfaces called from assembly or from generated
 * code. Declarations here to avoid warning about missing declarations.
 */
//asmlinkage 
void kasan_unpoison_task_stack_below(const void *watermark);
void __asan_register_globals(struct kasan_global *globals, size_t size);
void __asan_unregister_globals(struct kasan_global *globals, size_t size);
void __asan_loadN(unsigned long addr, size_t size);
void __asan_storeN(unsigned long addr, size_t size);
void __asan_handle_no_return(void);
void __asan_poison_stack_memory(const void *addr, size_t size);
void __asan_unpoison_stack_memory(const void *addr, size_t size);
void __asan_alloca_poison(unsigned long addr, size_t size);
void __asan_allocas_unpoison(const void *stack_top, const void *stack_bottom);

void __asan_load1(unsigned long addr);
void __asan_store1(unsigned long addr);
void __asan_load2(unsigned long addr);
void __asan_store2(unsigned long addr);
void __asan_load4(unsigned long addr);
void __asan_store4(unsigned long addr);
void __asan_load8(unsigned long addr);
void __asan_store8(unsigned long addr);
void __asan_load16(unsigned long addr);
void __asan_store16(unsigned long addr);

void __asan_load1_noabort(unsigned long addr);
void __asan_store1_noabort(unsigned long addr);
void __asan_load2_noabort(unsigned long addr);
void __asan_store2_noabort(unsigned long addr);
void __asan_load4_noabort(unsigned long addr);
void __asan_store4_noabort(unsigned long addr);
void __asan_load8_noabort(unsigned long addr);
void __asan_store8_noabort(unsigned long addr);
void __asan_load16_noabort(unsigned long addr);
void __asan_store16_noabort(unsigned long addr);

void __asan_set_shadow_00(void *addr, size_t size);
void __asan_set_shadow_f1(void *addr, size_t size);
void __asan_set_shadow_f2(void *addr, size_t size);
void __asan_set_shadow_f3(void *addr, size_t size);
void __asan_set_shadow_f5(void *addr, size_t size);
void __asan_set_shadow_f8(void *addr, size_t size);

#endif
