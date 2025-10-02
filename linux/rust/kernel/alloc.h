/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel allocation support - C port of kernel/alloc.rs
 */

#ifndef _LINUX_KERNEL_ALLOC_H
#define _LINUX_KERNEL_ALLOC_H

#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

/**
 * Allocation flags - replaces Rust AllocFlags
 */
typedef gfp_t kernel_alloc_flags_t;

#define KERNEL_GFP_KERNEL    GFP_KERNEL
#define KERNEL_GFP_ATOMIC    GFP_ATOMIC
#define KERNEL_GFP_NOWAIT    GFP_NOWAIT
#define KERNEL_GFP_NOIO      GFP_NOIO
#define KERNEL_GFP_NOFS      GFP_NOFS
#define KERNEL_GFP_USER      GFP_USER
#define KERNEL_GFP_DMA       GFP_DMA
#define KERNEL_GFP_DMA32     GFP_DMA32
#define KERNEL_GFP_HIGHMEM   GFP_HIGHMEM

/**
 * Allocation error handling
 */
typedef enum {
    KERNEL_ALLOC_OK = 0,
    KERNEL_ALLOC_ERR_NOMEM = -ENOMEM,
    KERNEL_ALLOC_ERR_INVAL = -EINVAL,
    KERNEL_ALLOC_ERR_FAULT = -EFAULT,
} kernel_alloc_error_t;

/**
 * Layout structure for allocation - replaces Rust Layout
 */
struct kernel_layout {
    size_t size;
    size_t align;
};

/**
 * KBox - kernel boxed allocation
 * Replaces Rust KBox<T>
 */
struct kernel_kbox {
    void *data;
    size_t size;
    kernel_alloc_flags_t flags;
};

/**
 * KVec - kernel vector
 * Replaces Rust KVec<T>
 */
struct kernel_kvec {
    void *data;
    size_t len;
    size_t capacity;
    size_t elem_size;
    kernel_alloc_flags_t flags;
};

/**
 * Layout operations
 */
static inline struct kernel_layout kernel_layout_new(size_t size, size_t align)
{
    struct kernel_layout layout = {
        .size = size,
        .align = align,
    };
    return layout;
}

static inline struct kernel_layout kernel_layout_from_size_align(size_t size, size_t align)
{
    return kernel_layout_new(size, align);
}

static inline size_t kernel_layout_size(const struct kernel_layout *layout)
{
    return layout->size;
}

static inline size_t kernel_layout_align(const struct kernel_layout *layout)
{
    return layout->align;
}

static inline bool kernel_layout_is_valid(const struct kernel_layout *layout)
{
    return layout->size > 0 && 
           layout->align > 0 && 
           (layout->align & (layout->align - 1)) == 0; /* power of 2 */
}

/**
 * KBox operations
 */
static inline struct kernel_kbox *kernel_kbox_new(size_t size, kernel_alloc_flags_t flags)
{
    struct kernel_kbox *kbox;
    
    kbox = kmalloc(sizeof(*kbox), flags);
    if (!kbox)
        return NULL;
    
    kbox->data = kmalloc(size, flags);
    if (!kbox->data) {
        kfree(kbox);
        return NULL;
    }
    
    kbox->size = size;
    kbox->flags = flags;
    return kbox;
}

static inline struct kernel_kbox *kernel_kbox_new_zeroed(size_t size, kernel_alloc_flags_t flags)
{
    struct kernel_kbox *kbox;
    
    kbox = kmalloc(sizeof(*kbox), flags);
    if (!kbox)
        return NULL;
    
    kbox->data = kzalloc(size, flags);
    if (!kbox->data) {
        kfree(kbox);
        return NULL;
    }
    
    kbox->size = size;
    kbox->flags = flags;
    return kbox;
}

static inline void kernel_kbox_free(struct kernel_kbox *kbox)
{
    if (kbox) {
        kfree(kbox->data);
        kfree(kbox);
    }
}

static inline void *kernel_kbox_leak(struct kernel_kbox *kbox)
{
    void *data = NULL;
    if (kbox) {
        data = kbox->data;
        kbox->data = NULL;  /* Prevent double free */
        kfree(kbox);
    }
    return data;
}

static inline void *kernel_kbox_as_ptr(struct kernel_kbox *kbox)
{
    return kbox ? kbox->data : NULL;
}

/**
 * KVec operations
 */
static inline struct kernel_kvec *kernel_kvec_new(size_t elem_size, kernel_alloc_flags_t flags)
{
    struct kernel_kvec *kvec;
    
    kvec = kmalloc(sizeof(*kvec), flags);
    if (!kvec)
        return NULL;
    
    kvec->data = NULL;
    kvec->len = 0;
    kvec->capacity = 0;
    kvec->elem_size = elem_size;
    kvec->flags = flags;
    return kvec;
}

static inline struct kernel_kvec *kernel_kvec_with_capacity(size_t capacity, 
    size_t elem_size, kernel_alloc_flags_t flags)
{
    struct kernel_kvec *kvec;
    
    kvec = kernel_kvec_new(elem_size, flags);
    if (!kvec)
        return NULL;
    
    if (capacity > 0) {
        kvec->data = kmalloc(capacity * elem_size, flags);
        if (!kvec->data) {
            kfree(kvec);
            return NULL;
        }
        kvec->capacity = capacity;
    }
    
    return kvec;
}

static inline void kernel_kvec_free(struct kernel_kvec *kvec)
{
    if (kvec) {
        kfree(kvec->data);
        kfree(kvec);
    }
}

static inline size_t kernel_kvec_len(const struct kernel_kvec *kvec)
{
    return kvec ? kvec->len : 0;
}

static inline size_t kernel_kvec_capacity(const struct kernel_kvec *kvec)
{
    return kvec ? kvec->capacity : 0;
}

static inline bool kernel_kvec_is_empty(const struct kernel_kvec *kvec)
{
    return kernel_kvec_len(kvec) == 0;
}

static inline int kernel_kvec_reserve(struct kernel_kvec *kvec, size_t additional)
{
    size_t new_capacity;
    void *new_data;
    
    if (!kvec)
        return -EINVAL;
    
    new_capacity = kvec->len + additional;
    if (new_capacity <= kvec->capacity)
        return 0;
    
    /* Grow capacity by 50% or to required size, whichever is larger */
    if (new_capacity < kvec->capacity + kvec->capacity / 2)
        new_capacity = kvec->capacity + kvec->capacity / 2;
    
    new_data = krealloc(kvec->data, new_capacity * kvec->elem_size, kvec->flags);
    if (!new_data)
        return -ENOMEM;
    
    kvec->data = new_data;
    kvec->capacity = new_capacity;
    return 0;
}

static inline int kernel_kvec_push(struct kernel_kvec *kvec, const void *elem)
{
    int ret;
    
    if (!kvec || !elem)
        return -EINVAL;
    
    ret = kernel_kvec_reserve(kvec, 1);
    if (ret)
        return ret;
    
    memcpy((char *)kvec->data + kvec->len * kvec->elem_size, elem, kvec->elem_size);
    kvec->len++;
    return 0;
}

static inline bool kernel_kvec_pop(struct kernel_kvec *kvec, void *elem)
{
    if (!kvec || kvec->len == 0)
        return false;
    
    kvec->len--;
    if (elem)
        memcpy(elem, (char *)kvec->data + kvec->len * kvec->elem_size, kvec->elem_size);
    
    return true;
}

static inline void *kernel_kvec_get(struct kernel_kvec *kvec, size_t index)
{
    if (!kvec || index >= kvec->len)
        return NULL;
    
    return (char *)kvec->data + index * kvec->elem_size;
}

/**
 * Global allocator interface
 */
static inline void *kernel_alloc_global(size_t size, kernel_alloc_flags_t flags)
{
    return kmalloc(size, flags);
}

static inline void *kernel_alloc_global_zeroed(size_t size, kernel_alloc_flags_t flags)
{
    return kzalloc(size, flags);
}

static inline void *kernel_realloc_global(void *ptr, size_t size, kernel_alloc_flags_t flags)
{
    return krealloc(ptr, size, flags);
}

static inline void kernel_free_global(void *ptr)
{
    kfree(ptr);
}

/**
 * Virtual memory allocation
 */
static inline void *kernel_vmalloc(size_t size)
{
    return vmalloc(size);
}

static inline void *kernel_vzalloc(size_t size)
{
    return vzalloc(size);
}

static inline void kernel_vfree(void *ptr)
{
    vfree(ptr);
}

#endif /* _LINUX_KERNEL_ALLOC_H */