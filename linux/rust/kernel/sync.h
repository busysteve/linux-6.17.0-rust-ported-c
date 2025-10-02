/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel synchronization primitives - C port of kernel/sync module
 */

#ifndef _LINUX_KERNEL_SYNC_H
#define _LINUX_KERNEL_SYNC_H

#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/atomic.h>
#include <linux/refcount.h>
#include <linux/rwlock.h>
#include <linux/seqlock.h>
#include <linux/rcu.h>

/**
 * Arc - Atomically reference-counted wrapper
 * Replaces Rust Arc<T>
 */
struct kernel_arc {
    void *data;
    refcount_t refcount;
    void (*destructor)(void *);
    struct mutex lock;  /* For interior mutability */
};

/**
 * ARef - Atomically reference-counted reference
 * Replaces Rust ARef<T>
 */
struct kernel_aref {
    struct kernel_arc *arc;
};

/**
 * Mutex wrapper - replaces Rust Mutex<T>
 */
struct kernel_mutex {
    struct mutex lock;
    void *data;
};

/**
 * SpinLock wrapper - replaces Rust SpinLock<T>
 */
struct kernel_spinlock {
    spinlock_t lock;
    void *data;
};

/**
 * RwLock wrapper - replaces Rust RwLock<T>
 */
struct kernel_rwlock {
    rwlock_t lock;
    void *data;
};

/**
 * Completion wrapper - replaces Rust Completion
 */
struct kernel_completion {
    struct completion completion;
};

/**
 * Atomic operations - replaces Rust atomic types
 */
struct kernel_atomic_i32 {
    atomic_t value;
};

struct kernel_atomic_i64 {
    atomic64_t value;
};

struct kernel_atomic_ptr {
    void *value;
    spinlock_t lock;  /* For non-atomic pointer operations */
};

/**
 * Barrier operations
 */
static inline void kernel_barrier_compiler(void)
{
    barrier();
}

static inline void kernel_barrier_memory(void)
{
    smp_mb();
}

static inline void kernel_barrier_read(void)
{
    smp_rmb();
}

static inline void kernel_barrier_write(void)
{
    smp_wmb();
}

/**
 * Arc operations
 */
static inline struct kernel_arc *kernel_arc_new(void *data, void (*destructor)(void *))
{
    struct kernel_arc *arc;
    
    arc = kmalloc(sizeof(*arc), GFP_KERNEL);
    if (!arc)
        return NULL;
    
    arc->data = data;
    refcount_set(&arc->refcount, 1);
    arc->destructor = destructor;
    mutex_init(&arc->lock);
    
    return arc;
}

static inline struct kernel_aref kernel_arc_clone(struct kernel_arc *arc)
{
    struct kernel_aref aref = { .arc = NULL };
    
    if (arc) {
        refcount_inc(&arc->refcount);
        aref.arc = arc;
    }
    
    return aref;
}

static inline void kernel_aref_drop(struct kernel_aref *aref)
{
    if (aref && aref->arc) {
        if (refcount_dec_and_test(&aref->arc->refcount)) {
            if (aref->arc->destructor && aref->arc->data)
                aref->arc->destructor(aref->arc->data);
            kfree(aref->arc);
        }
        aref->arc = NULL;
    }
}

static inline void *kernel_aref_get(struct kernel_aref *aref)
{
    return (aref && aref->arc) ? aref->arc->data : NULL;
}

/**
 * Mutex operations
 */
static inline struct kernel_mutex *kernel_mutex_new(void *data)
{
    struct kernel_mutex *kmutex;
    
    kmutex = kmalloc(sizeof(*kmutex), GFP_KERNEL);
    if (!kmutex)
        return NULL;
    
    mutex_init(&kmutex->lock);
    kmutex->data = data;
    
    return kmutex;
}

static inline void kernel_mutex_free(struct kernel_mutex *kmutex)
{
    if (kmutex) {
        mutex_destroy(&kmutex->lock);
        kfree(kmutex);
    }
}

static inline void kernel_mutex_lock(struct kernel_mutex *kmutex)
{
    if (kmutex)
        mutex_lock(&kmutex->lock);
}

static inline int kernel_mutex_lock_interruptible(struct kernel_mutex *kmutex)
{
    return kmutex ? mutex_lock_interruptible(&kmutex->lock) : -EINVAL;
}

static inline int kernel_mutex_trylock(struct kernel_mutex *kmutex)
{
    return kmutex ? mutex_trylock(&kmutex->lock) : 0;
}

static inline void kernel_mutex_unlock(struct kernel_mutex *kmutex)
{
    if (kmutex)
        mutex_unlock(&kmutex->lock);
}

static inline void *kernel_mutex_get_data(struct kernel_mutex *kmutex)
{
    return kmutex ? kmutex->data : NULL;
}

/**
 * SpinLock operations
 */
static inline struct kernel_spinlock *kernel_spinlock_new(void *data)
{
    struct kernel_spinlock *kspinlock;
    
    kspinlock = kmalloc(sizeof(*kspinlock), GFP_KERNEL);
    if (!kspinlock)
        return NULL;
    
    spin_lock_init(&kspinlock->lock);
    kspinlock->data = data;
    
    return kspinlock;
}

static inline void kernel_spinlock_free(struct kernel_spinlock *kspinlock)
{
    kfree(kspinlock);
}

static inline void kernel_spinlock_lock(struct kernel_spinlock *kspinlock)
{
    if (kspinlock)
        spin_lock(&kspinlock->lock);
}

static inline void kernel_spinlock_unlock(struct kernel_spinlock *kspinlock)
{
    if (kspinlock)
        spin_unlock(&kspinlock->lock);
}

static inline void kernel_spinlock_lock_irqsave(struct kernel_spinlock *kspinlock, 
                                                unsigned long *flags)
{
    if (kspinlock)
        spin_lock_irqsave(&kspinlock->lock, *flags);
}

static inline void kernel_spinlock_unlock_irqrestore(struct kernel_spinlock *kspinlock, 
                                                     unsigned long flags)
{
    if (kspinlock)
        spin_unlock_irqrestore(&kspinlock->lock, flags);
}

static inline void *kernel_spinlock_get_data(struct kernel_spinlock *kspinlock)
{
    return kspinlock ? kspinlock->data : NULL;
}

/**
 * RwLock operations
 */
static inline struct kernel_rwlock *kernel_rwlock_new(void *data)
{
    struct kernel_rwlock *krwlock;
    
    krwlock = kmalloc(sizeof(*krwlock), GFP_KERNEL);
    if (!krwlock)
        return NULL;
    
    rwlock_init(&krwlock->lock);
    krwlock->data = data;
    
    return krwlock;
}

static inline void kernel_rwlock_free(struct kernel_rwlock *krwlock)
{
    kfree(krwlock);
}

static inline void kernel_rwlock_read_lock(struct kernel_rwlock *krwlock)
{
    if (krwlock)
        read_lock(&krwlock->lock);
}

static inline void kernel_rwlock_read_unlock(struct kernel_rwlock *krwlock)
{
    if (krwlock)
        read_unlock(&krwlock->lock);
}

static inline void kernel_rwlock_write_lock(struct kernel_rwlock *krwlock)
{
    if (krwlock)
        write_lock(&krwlock->lock);
}

static inline void kernel_rwlock_write_unlock(struct kernel_rwlock *krwlock)
{
    if (krwlock)
        write_unlock(&krwlock->lock);
}

static inline void *kernel_rwlock_get_data(struct kernel_rwlock *krwlock)
{
    return krwlock ? krwlock->data : NULL;
}

/**
 * Completion operations
 */
static inline struct kernel_completion *kernel_completion_new(void)
{
    struct kernel_completion *kcompletion;
    
    kcompletion = kmalloc(sizeof(*kcompletion), GFP_KERNEL);
    if (!kcompletion)
        return NULL;
    
    init_completion(&kcompletion->completion);
    
    return kcompletion;
}

static inline void kernel_completion_free(struct kernel_completion *kcompletion)
{
    kfree(kcompletion);
}

static inline void kernel_completion_wait(struct kernel_completion *kcompletion)
{
    if (kcompletion)
        wait_for_completion(&kcompletion->completion);
}

static inline int kernel_completion_wait_timeout(struct kernel_completion *kcompletion, 
                                                 unsigned long timeout)
{
    return kcompletion ? wait_for_completion_timeout(&kcompletion->completion, timeout) : 0;
}

static inline void kernel_completion_complete(struct kernel_completion *kcompletion)
{
    if (kcompletion)
        complete(&kcompletion->completion);
}

static inline void kernel_completion_complete_all(struct kernel_completion *kcompletion)
{
    if (kcompletion)
        complete_all(&kcompletion->completion);
}

/**
 * Atomic operations
 */
static inline struct kernel_atomic_i32 kernel_atomic_i32_new(int value)
{
    struct kernel_atomic_i32 atomic;
    atomic_set(&atomic.value, value);
    return atomic;
}

static inline int kernel_atomic_i32_load(struct kernel_atomic_i32 *atomic)
{
    return atomic ? atomic_read(&atomic->value) : 0;
}

static inline void kernel_atomic_i32_store(struct kernel_atomic_i32 *atomic, int value)
{
    if (atomic)
        atomic_set(&atomic->value, value);
}

static inline int kernel_atomic_i32_add_return(struct kernel_atomic_i32 *atomic, int value)
{
    return atomic ? atomic_add_return(value, &atomic->value) : 0;
}

static inline int kernel_atomic_i32_sub_return(struct kernel_atomic_i32 *atomic, int value)
{
    return atomic ? atomic_sub_return(value, &atomic->value) : 0;
}

static inline bool kernel_atomic_i32_compare_exchange(struct kernel_atomic_i32 *atomic, 
                                                      int *expected, int desired)
{
    int old;
    
    if (!atomic || !expected)
        return false;
    
    old = atomic_cmpxchg(&atomic->value, *expected, desired);
    if (old == *expected)
        return true;
    
    *expected = old;
    return false;
}

/**
 * RCU operations
 */
static inline void kernel_rcu_read_lock(void)
{
    rcu_read_lock();
}

static inline void kernel_rcu_read_unlock(void)
{
    rcu_read_unlock();
}

static inline void kernel_synchronize_rcu(void)
{
    synchronize_rcu();
}

static inline void kernel_call_rcu(struct rcu_head *head, rcu_callback_t func)
{
    call_rcu(head, func);
}

#endif /* _LINUX_KERNEL_SYNC_H */