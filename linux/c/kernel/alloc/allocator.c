// SPDX-License-Identifier: GPL-2.0

/*
 * Allocator support.
 *
 * Documentation for the kernel's memory allocators can be found in the
 * "Memory Allocation Guide" in the kernel docs.
 */

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/string.h>

// Type alias for allocation flags
typedef gfp_t Flags;

// Helper for alignment
static inline size_t pad_to_align(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

// Kmalloc: physically contiguous allocator
static void *kmalloc_realloc(void *ptr, size_t size, Flags flags) {
    return krealloc(ptr, size, flags);
}

// Vmalloc: virtually contiguous allocator
static void *vmalloc_realloc(void *ptr, size_t size, Flags flags) {
    return vrealloc(ptr, size, flags);
}

// KVmalloc: tries kmalloc, falls back to vmalloc
static void *kvmalloc_realloc(void *ptr, size_t size, Flags flags) {
    return kvrealloc(ptr, size, flags);
}

// Kmalloc aligned layout
static inline size_t kmalloc_aligned_size(size_t size, size_t align) {
    return pad_to_align(size, align);
}

// Kmalloc allocator
static void *Kmalloc_realloc(void *ptr, size_t size, size_t align, Flags flags) {
    size_t padded = kmalloc_aligned_size(size, align);
    return kmalloc_realloc(ptr, padded, flags);
}

// Vmalloc allocator
static void *Vmalloc_realloc(void *ptr, size_t size, size_t align, Flags flags) {
    if (align > PAGE_SIZE) {
        pr_warn("Vmalloc does not support alignments larger than PAGE_SIZE yet.\n");
        return NULL;
    }
    return vmalloc_realloc(ptr, size, flags);
}

// KVmalloc allocator
static void *KVmalloc_realloc(void *ptr, size_t size, size_t align, Flags flags) {
    size_t padded = kmalloc_aligned_size(size, align);
    if (align > PAGE_SIZE) {
        pr_warn("KVmalloc does not support alignments larger than PAGE_SIZE yet.\n");
        return NULL;
    }
    return kvmalloc_realloc(ptr, padded, flags);
}
