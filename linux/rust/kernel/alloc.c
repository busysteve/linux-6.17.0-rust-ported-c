/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Implementation of the kernel's memory allocation infrastructure.
 * C port of kernel/alloc.rs
 */

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/err.h>

#include "alloc.h"
#include "error.h"

/**
 * Allocation flags implementation
 */
struct kernel_alloc_flags {
    u32 flags;
};

/* Allocation flags constants */
const kernel_alloc_flags_t KERNEL_GFP_ZERO = __GFP_ZERO;
const kernel_alloc_flags_t KERNEL_GFP_HIGHMEM_FLAG = __GFP_HIGHMEM;
const kernel_alloc_flags_t KERNEL_GFP_ATOMIC_FLAG = GFP_ATOMIC;
const kernel_alloc_flags_t KERNEL_GFP_KERNEL_FLAG = GFP_KERNEL;
const kernel_alloc_flags_t KERNEL_GFP_NOWAIT_FLAG = GFP_NOWAIT;
const kernel_alloc_flags_t KERNEL_GFP_NOIO_FLAG = GFP_NOIO;
const kernel_alloc_flags_t KERNEL_GFP_NOFS_FLAG = GFP_NOFS;
const kernel_alloc_flags_t KERNEL_GFP_USER_FLAG = GFP_USER;
const kernel_alloc_flags_t KERNEL_GFP_DMA_FLAG = GFP_DMA;
const kernel_alloc_flags_t KERNEL_GFP_DMA32_FLAG = GFP_DMA32;

EXPORT_SYMBOL_GPL(KERNEL_GFP_ZERO);
EXPORT_SYMBOL_GPL(KERNEL_GFP_HIGHMEM_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_ATOMIC_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_KERNEL_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_NOWAIT_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_NOIO_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_NOFS_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_USER_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_DMA_FLAG);
EXPORT_SYMBOL_GPL(KERNEL_GFP_DMA32_FLAG);

/**
 * kernel_alloc_flags_contains - Check if flags are contained
 * @self: The flags to check
 * @flags: The flags to look for
 * 
 * Returns true if all flags in @flags are set in @self
 */
bool kernel_alloc_flags_contains(kernel_alloc_flags_t self, kernel_alloc_flags_t flags)
{
    return (self & flags) == flags;
}
EXPORT_SYMBOL_GPL(kernel_alloc_flags_contains);

/**
 * kernel_alloc_flags_or - Combine allocation flags
 * @lhs: Left hand side flags
 * @rhs: Right hand side flags
 * 
 * Returns combined flags
 */
kernel_alloc_flags_t kernel_alloc_flags_or(kernel_alloc_flags_t lhs, kernel_alloc_flags_t rhs)
{
    return lhs | rhs;
}
EXPORT_SYMBOL_GPL(kernel_alloc_flags_or);

/**
 * kernel_alloc_flags_and - AND allocation flags
 * @lhs: Left hand side flags
 * @rhs: Right hand side flags
 * 
 * Returns AND result of flags
 */
kernel_alloc_flags_t kernel_alloc_flags_and(kernel_alloc_flags_t lhs, kernel_alloc_flags_t rhs)
{
    return lhs & rhs;
}
EXPORT_SYMBOL_GPL(kernel_alloc_flags_and);

/**
 * kernel_alloc_flags_not - Invert allocation flags
 * @flags: Flags to invert
 * 
 * Returns inverted flags
 */
kernel_alloc_flags_t kernel_alloc_flags_not(kernel_alloc_flags_t flags)
{
    return ~flags;
}
EXPORT_SYMBOL_GPL(kernel_alloc_flags_not);

/**
 * Layout operations implementation
 */

/**
 * kernel_layout_from_size_align_impl - Create layout from size and alignment
 * @size: Size in bytes
 * @align: Alignment requirement
 * 
 * Returns a layout structure or error if invalid
 */
struct kernel_layout kernel_layout_from_size_align_impl(size_t size, size_t align)
{
    struct kernel_layout layout;
    
    /* Validate alignment is power of 2 */
    if (align == 0 || (align & (align - 1)) != 0) {
        layout.size = 0;
        layout.align = 0;
        return layout;
    }
    
    /* Check for overflow */
    if (size > SIZE_MAX - (align - 1)) {
        layout.size = 0;
        layout.align = 0;
        return layout;
    }
    
    layout.size = size;
    layout.align = align;
    return layout;
}
EXPORT_SYMBOL_GPL(kernel_layout_from_size_align_impl);

/**
 * kernel_layout_array_impl - Create layout for array
 * @elem_layout: Layout for single element
 * @n: Number of elements
 * 
 * Returns layout for array or invalid layout on overflow
 */
struct kernel_layout kernel_layout_array_impl(struct kernel_layout elem_layout, size_t n)
{
    struct kernel_layout layout;
    size_t total_size;
    
    if (!kernel_layout_is_valid(&elem_layout)) {
        layout.size = 0;
        layout.align = 0;
        return layout;
    }
    
    /* Check for overflow in multiplication */
    if (n > 0 && elem_layout.size > SIZE_MAX / n) {
        layout.size = 0;
        layout.align = 0;
        return layout;
    }
    
    total_size = elem_layout.size * n;
    
    /* Round up to alignment boundary */
    if (elem_layout.align > 1) {
        size_t remainder = total_size % elem_layout.align;
        if (remainder != 0) {
            if (total_size > SIZE_MAX - (elem_layout.align - remainder)) {
                layout.size = 0;
                layout.align = 0;
                return layout;
            }
            total_size += elem_layout.align - remainder;
        }
    }
    
    layout.size = total_size;
    layout.align = elem_layout.align;
    return layout;
}
EXPORT_SYMBOL_GPL(kernel_layout_array_impl);

/**
 * Global allocator implementation
 */

/**
 * kernel_allocator_alloc - Allocate memory with layout
 * @layout: Memory layout requirements
 * @flags: Allocation flags
 * 
 * Returns pointer to allocated memory or NULL on failure
 */
void *kernel_allocator_alloc(struct kernel_layout layout, kernel_alloc_flags_t flags)
{
    void *ptr;
    
    if (!kernel_layout_is_valid(&layout))
        return NULL;
    
    ptr = kmalloc(layout.size, flags);
    if (!ptr)
        return NULL;
    
    /* Check alignment - kmalloc provides suitable alignment for most cases */
    if (((uintptr_t)ptr & (layout.align - 1)) != 0) {
        kfree(ptr);
        
        /* Try aligned allocation for stricter requirements */
        if (layout.align > ARCH_KMALLOC_MINALIGN) {
            /* For large alignments, we might need vmalloc or specific allocators */
            return NULL;
        }
        return NULL;
    }
    
    return ptr;
}
EXPORT_SYMBOL_GPL(kernel_allocator_alloc);

/**
 * kernel_allocator_alloc_zeroed - Allocate zeroed memory with layout
 * @layout: Memory layout requirements  
 * @flags: Allocation flags
 * 
 * Returns pointer to allocated zeroed memory or NULL on failure
 */
void *kernel_allocator_alloc_zeroed(struct kernel_layout layout, kernel_alloc_flags_t flags)
{
    void *ptr;
    
    ptr = kernel_allocator_alloc(layout, flags | __GFP_ZERO);
    if (!ptr) {
        /* Fallback: allocate and manually zero */
        ptr = kernel_allocator_alloc(layout, flags);
        if (ptr)
            memset(ptr, 0, layout.size);
    }
    
    return ptr;
}
EXPORT_SYMBOL_GPL(kernel_allocator_alloc_zeroed);

/**
 * kernel_allocator_realloc - Reallocate memory
 * @ptr: Existing pointer (can be NULL)
 * @old_layout: Layout of existing allocation
 * @new_layout: New layout requirements
 * @flags: Allocation flags
 * 
 * Returns pointer to reallocated memory or NULL on failure
 */
void *kernel_allocator_realloc(void *ptr, struct kernel_layout old_layout, 
                               struct kernel_layout new_layout, kernel_alloc_flags_t flags)
{
    void *new_ptr;
    size_t copy_size;
    
    if (!kernel_layout_is_valid(&new_layout))
        return NULL;
    
    /* Handle NULL pointer case */
    if (!ptr)
        return kernel_allocator_alloc(new_layout, flags);
    
    /* Handle zero size case */
    if (new_layout.size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    /* Try krealloc first */
    new_ptr = krealloc(ptr, new_layout.size, flags);
    if (new_ptr) {
        /* Check if alignment is still satisfied */
        if (((uintptr_t)new_ptr & (new_layout.align - 1)) == 0)
            return new_ptr;
        
        /* Alignment not satisfied, need to allocate new memory */
        kfree(new_ptr);
    }
    
    /* Allocate new memory and copy */
    new_ptr = kernel_allocator_alloc(new_layout, flags);
    if (!new_ptr)
        return NULL;
    
    copy_size = min(old_layout.size, new_layout.size);
    memcpy(new_ptr, ptr, copy_size);
    kfree(ptr);
    
    return new_ptr;
}
EXPORT_SYMBOL_GPL(kernel_allocator_realloc);

/**
 * kernel_allocator_dealloc - Deallocate memory
 * @ptr: Pointer to memory to deallocate
 * @layout: Layout of the allocation (for validation)
 */
void kernel_allocator_dealloc(void *ptr, struct kernel_layout layout)
{
    if (ptr && kernel_layout_is_valid(&layout))
        kfree(ptr);
}
EXPORT_SYMBOL_GPL(kernel_allocator_dealloc);

/**
 * High-level allocation functions
 */

/**
 * kernel_alloc_layout - Allocate memory with specific layout
 * @layout: Layout requirements
 * @flags: Allocation flags
 * 
 * Returns result containing pointer or error
 */
void *kernel_alloc_layout(struct kernel_layout layout, kernel_alloc_flags_t flags)
{
    return kernel_allocator_alloc(layout, flags);
}
EXPORT_SYMBOL_GPL(kernel_alloc_layout);

/**
 * kernel_alloc_layout_zeroed - Allocate zeroed memory with specific layout  
 * @layout: Layout requirements
 * @flags: Allocation flags
 * 
 * Returns result containing pointer or error
 */
void *kernel_alloc_layout_zeroed(struct kernel_layout layout, kernel_alloc_flags_t flags)
{
    return kernel_allocator_alloc_zeroed(layout, flags);
}
EXPORT_SYMBOL_GPL(kernel_alloc_layout_zeroed);

/**
 * kernel_realloc_layout - Reallocate memory with new layout
 * @ptr: Existing pointer
 * @old_layout: Current layout  
 * @new_layout: New layout requirements
 * @flags: Allocation flags
 * 
 * Returns result containing new pointer or error
 */
void *kernel_realloc_layout(void *ptr, struct kernel_layout old_layout,
                            struct kernel_layout new_layout, kernel_alloc_flags_t flags)
{
    return kernel_allocator_realloc(ptr, old_layout, new_layout, flags);
}
EXPORT_SYMBOL_GPL(kernel_realloc_layout);

/**
 * kernel_dealloc_layout - Deallocate memory
 * @ptr: Pointer to deallocate
 * @layout: Layout of allocation
 */
void kernel_dealloc_layout(void *ptr, struct kernel_layout layout)
{
    kernel_allocator_dealloc(ptr, layout);
}
EXPORT_SYMBOL_GPL(kernel_dealloc_layout);