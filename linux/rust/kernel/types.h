/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel types - C port of kernel/types.rs
 */

#ifndef _LINUX_KERNEL_TYPES_H
#define _LINUX_KERNEL_TYPES_H

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/refcount.h>

/**
 * Opaque type - replaces Rust Opaque<T>
 * Used for wrapping C types that need to be used from kernel modules
 */
struct kernel_opaque {
    void *ptr;
    size_t size;
    void (*destructor)(void *);
};

/**
 * ARef - Atomically reference-counted wrapper
 * Replaces Rust ARef<T>
 */
struct kernel_aref {
    void *data;
    refcount_t refcount;
    void (*release)(struct kernel_aref *);
};

/**
 * AlwaysRefCounted interface
 * Replaces Rust AlwaysRefCounted trait
 */
struct always_ref_counted_ops {
    void (*inc_ref)(void *obj);
    void (*dec_ref)(void *obj);
    void (*dec_ref_non_zero)(void *obj);
};

/**
 * ForeignOwnable implementation
 * Replaces Rust ForeignOwnable trait
 */
struct kernel_foreign_ownable {
    void *data;
    const struct foreign_ownable_ops *ops;
};

/**
 * Allocation functions
 */
static inline void *kernel_alloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags);
}

static inline void *kernel_zalloc(size_t size, gfp_t flags)
{
    return kzalloc(size, flags);
}

static inline void kernel_free(void *ptr)
{
    kfree(ptr);
}

/**
 * ARef operations
 */
static inline struct kernel_aref *kernel_aref_new(void *data, 
    void (*release)(struct kernel_aref *))
{
    struct kernel_aref *aref = kernel_alloc(sizeof(*aref), GFP_KERNEL);
    if (!aref)
        return NULL;
    
    aref->data = data;
    refcount_set(&aref->refcount, 1);
    aref->release = release;
    return aref;
}

static inline void kernel_aref_get(struct kernel_aref *aref)
{
    if (aref)
        refcount_inc(&aref->refcount);
}

static inline void kernel_aref_put(struct kernel_aref *aref)
{
    if (aref && refcount_dec_and_test(&aref->refcount)) {
        if (aref->release)
            aref->release(aref);
        kernel_free(aref);
    }
}

/**
 * Opaque type operations
 */
static inline struct kernel_opaque *kernel_opaque_new(void *ptr, size_t size,
    void (*destructor)(void *))
{
    struct kernel_opaque *opaque = kernel_alloc(sizeof(*opaque), GFP_KERNEL);
    if (!opaque)
        return NULL;
    
    opaque->ptr = ptr;
    opaque->size = size;
    opaque->destructor = destructor;
    return opaque;
}

static inline void kernel_opaque_free(struct kernel_opaque *opaque)
{
    if (opaque) {
        if (opaque->destructor && opaque->ptr)
            opaque->destructor(opaque->ptr);
        kernel_free(opaque);
    }
}

static inline void *kernel_opaque_get(struct kernel_opaque *opaque)
{
    return opaque ? opaque->ptr : NULL;
}

/**
 * ForeignOwnable operations
 */
static inline void *kernel_foreign_into_foreign(struct kernel_foreign_ownable *obj)
{
    if (!obj || !obj->ops || !obj->ops->into_foreign)
        return NULL;
    return obj->ops->into_foreign(obj->data);
}

static inline void *kernel_foreign_from_foreign(void *ptr, 
    const struct foreign_ownable_ops *ops)
{
    if (!ptr || !ops || !ops->from_foreign)
        return NULL;
    return ops->from_foreign(ptr);
}

static inline void *kernel_foreign_try_from_foreign(void *ptr,
    const struct foreign_ownable_ops *ops)
{
    if (!ptr)
        return NULL;
    return kernel_foreign_from_foreign(ptr, ops);
}

static inline void *kernel_foreign_borrow(void *ptr,
    const struct foreign_ownable_ops *ops)
{
    if (!ptr || !ops || !ops->borrow)
        return NULL;
    return ops->borrow(ptr);
}

static inline void *kernel_foreign_borrow_mut(void *ptr,
    const struct foreign_ownable_ops *ops)
{
    if (!ptr || !ops || !ops->borrow_mut)
        return NULL;
    return ops->borrow_mut(ptr);
}

#endif /* _LINUX_KERNEL_TYPES_H */