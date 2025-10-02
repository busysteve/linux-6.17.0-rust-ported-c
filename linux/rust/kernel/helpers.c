/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel helper functions - C ports of helpers/ directory
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/atomic.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/bug.h>
#include <linux/build_bug.h>

/**
 * Atomic operations helpers - port of helpers/atomic.c
 */

/**
 * kernel_atomic_add_return_wrapper - Atomic add and return wrapper
 * @value: Value to add
 * @atomic: Atomic variable
 * 
 * Returns the new value after addition
 */
int kernel_atomic_add_return_wrapper(int value, atomic_t *atomic)
{
    return atomic_add_return(value, atomic);
}
EXPORT_SYMBOL_GPL(kernel_atomic_add_return_wrapper);

/**
 * kernel_atomic_sub_return_wrapper - Atomic subtract and return wrapper
 * @value: Value to subtract
 * @atomic: Atomic variable
 * 
 * Returns the new value after subtraction
 */
int kernel_atomic_sub_return_wrapper(int value, atomic_t *atomic)
{
    return atomic_sub_return(value, atomic);
}
EXPORT_SYMBOL_GPL(kernel_atomic_sub_return_wrapper);

/**
 * kernel_atomic_cmpxchg_wrapper - Atomic compare and exchange wrapper
 * @atomic: Atomic variable
 * @old: Expected old value
 * @new: New value to set
 * 
 * Returns the previous value
 */
int kernel_atomic_cmpxchg_wrapper(atomic_t *atomic, int old, int new)
{
    return atomic_cmpxchg(atomic, old, new);
}
EXPORT_SYMBOL_GPL(kernel_atomic_cmpxchg_wrapper);

/**
 * Barrier helpers - port of helpers/barrier.c
 */

/**
 * kernel_smp_mb_wrapper - Memory barrier wrapper
 */
void kernel_smp_mb_wrapper(void)
{
    smp_mb();
}
EXPORT_SYMBOL_GPL(kernel_smp_mb_wrapper);

/**
 * kernel_smp_rmb_wrapper - Read memory barrier wrapper
 */
void kernel_smp_rmb_wrapper(void)
{
    smp_rmb();
}
EXPORT_SYMBOL_GPL(kernel_smp_rmb_wrapper);

/**
 * kernel_smp_wmb_wrapper - Write memory barrier wrapper
 */
void kernel_smp_wmb_wrapper(void)
{
    smp_wmb();
}
EXPORT_SYMBOL_GPL(kernel_smp_wmb_wrapper);

/**
 * Bug helpers - port of helpers/bug.c
 */

/**
 * kernel_bug_wrapper - BUG() wrapper
 */
void kernel_bug_wrapper(void)
{
    BUG();
}
EXPORT_SYMBOL_GPL(kernel_bug_wrapper);

/**
 * kernel_warn_wrapper - WARN() wrapper
 * @condition: Condition to check
 * @fmt: Format string
 */
void kernel_warn_wrapper(int condition, const char *fmt, ...)
{
    va_list args;
    
    if (condition) {
        va_start(args, fmt);
        vprintk(fmt, args);
        va_end(args);
        dump_stack();
    }
}
EXPORT_SYMBOL_GPL(kernel_warn_wrapper);

/**
 * Build assertion helpers - port of helpers/build_assert.c
 */

/**
 * kernel_build_assert_wrapper - Build-time assertion wrapper
 * @condition: Condition that must be true at compile time
 */
#define kernel_build_assert(condition) BUILD_BUG_ON(!(condition))

/**
 * Completion helpers - port of helpers/completion.c
 */

/**
 * kernel_completion_reinit_wrapper - Reinitialize completion
 * @completion: Completion to reinitialize
 */
void kernel_completion_reinit_wrapper(struct completion *completion)
{
    if (completion)
        reinit_completion(completion);
}
EXPORT_SYMBOL_GPL(kernel_completion_reinit_wrapper);

/**
 * kernel_wait_for_completion_timeout_wrapper - Wait for completion with timeout
 * @completion: Completion to wait for
 * @timeout: Timeout in jiffies
 * 
 * Returns remaining time or 0 on timeout
 */
unsigned long kernel_wait_for_completion_timeout_wrapper(struct completion *completion,
                                                        unsigned long timeout)
{
    return completion ? wait_for_completion_timeout(completion, timeout) : 0;
}
EXPORT_SYMBOL_GPL(kernel_wait_for_completion_timeout_wrapper);

/**
 * Mutex helpers - port of helpers/mutex.c
 */

/**
 * kernel_mutex_lock_interruptible_wrapper - Interruptible mutex lock
 * @mutex: Mutex to lock
 * 
 * Returns 0 on success, -EINTR if interrupted
 */
int kernel_mutex_lock_interruptible_wrapper(struct mutex *mutex)
{
    return mutex ? mutex_lock_interruptible(mutex) : -EINVAL;
}
EXPORT_SYMBOL_GPL(kernel_mutex_lock_interruptible_wrapper);

/**
 * kernel_mutex_trylock_wrapper - Try to lock mutex
 * @mutex: Mutex to try to lock
 * 
 * Returns 1 if lock acquired, 0 otherwise
 */
int kernel_mutex_trylock_wrapper(struct mutex *mutex)
{
    return mutex ? mutex_trylock(mutex) : 0;
}
EXPORT_SYMBOL_GPL(kernel_mutex_trylock_wrapper);

/**
 * Spinlock helpers - port of helpers/spinlock.c
 */

/**
 * kernel_spin_lock_irqsave_wrapper - Spin lock with IRQ save
 * @lock: Spinlock
 * @flags: Flags to save IRQ state
 */
void kernel_spin_lock_irqsave_wrapper(spinlock_t *lock, unsigned long *flags)
{
    if (lock && flags)
        spin_lock_irqsave(lock, *flags);
}
EXPORT_SYMBOL_GPL(kernel_spin_lock_irqsave_wrapper);

/**
 * kernel_spin_unlock_irqrestore_wrapper - Spin unlock with IRQ restore
 * @lock: Spinlock
 * @flags: Flags to restore IRQ state
 */
void kernel_spin_unlock_irqrestore_wrapper(spinlock_t *lock, unsigned long flags)
{
    if (lock)
        spin_unlock_irqrestore(lock, flags);
}
EXPORT_SYMBOL_GPL(kernel_spin_unlock_irqrestore_wrapper);

/**
 * Task helpers - port of helpers/task.c
 */

/**
 * kernel_get_current_wrapper - Get current task
 * 
 * Returns pointer to current task_struct
 */
struct task_struct *kernel_get_current_wrapper(void)
{
    return current;
}
EXPORT_SYMBOL_GPL(kernel_get_current_wrapper);

/**
 * kernel_get_current_pid_wrapper - Get current process ID
 * 
 * Returns PID of current task
 */
pid_t kernel_get_current_pid_wrapper(void)
{
    return current->pid;
}
EXPORT_SYMBOL_GPL(kernel_get_current_pid_wrapper);

/**
 * Time helpers - port of helpers/time.c
 */

/**
 * kernel_jiffies_to_msecs_wrapper - Convert jiffies to milliseconds
 * @jiffies: Jiffies value
 * 
 * Returns milliseconds
 */
unsigned int kernel_jiffies_to_msecs_wrapper(unsigned long jiffies)
{
    return jiffies_to_msecs(jiffies);
}
EXPORT_SYMBOL_GPL(kernel_jiffies_to_msecs_wrapper);

/**
 * kernel_msecs_to_jiffies_wrapper - Convert milliseconds to jiffies
 * @msecs: Milliseconds value
 * 
 * Returns jiffies
 */
unsigned long kernel_msecs_to_jiffies_wrapper(unsigned int msecs)
{
    return msecs_to_jiffies(msecs);
}
EXPORT_SYMBOL_GPL(kernel_msecs_to_jiffies_wrapper);

/**
 * Wait queue helpers - port of helpers/wait.c
 */

/**
 * kernel_wake_up_wrapper - Wake up wait queue
 * @wait_queue: Wait queue to wake up
 */
void kernel_wake_up_wrapper(wait_queue_head_t *wait_queue)
{
    if (wait_queue)
        wake_up(wait_queue);
}
EXPORT_SYMBOL_GPL(kernel_wake_up_wrapper);

/**
 * kernel_wake_up_interruptible_wrapper - Wake up interruptible wait queue
 * @wait_queue: Wait queue to wake up
 */
void kernel_wake_up_interruptible_wrapper(wait_queue_head_t *wait_queue)
{
    if (wait_queue)
        wake_up_interruptible(wait_queue);
}
EXPORT_SYMBOL_GPL(kernel_wake_up_interruptible_wrapper);

/**
 * Workqueue helpers - port of helpers/workqueue.c
 */

/**
 * kernel_schedule_work_wrapper - Schedule work
 * @work: Work structure
 * 
 * Returns true if work was queued, false otherwise
 */
bool kernel_schedule_work_wrapper(struct work_struct *work)
{
    return work ? schedule_work(work) : false;
}
EXPORT_SYMBOL_GPL(kernel_schedule_work_wrapper);

/**
 * kernel_flush_work_wrapper - Flush work
 * @work: Work structure
 * 
 * Returns true if work was pending
 */
bool kernel_flush_work_wrapper(struct work_struct *work)
{
    return work ? flush_work(work) : false;
}
EXPORT_SYMBOL_GPL(kernel_flush_work_wrapper);

/**
 * Error helpers - additional error handling utilities
 */

/**
 * kernel_is_err_wrapper - Check if pointer is error
 * @ptr: Pointer to check
 * 
 * Returns true if pointer contains error
 */
bool kernel_is_err_wrapper(const void *ptr)
{
    return IS_ERR(ptr);
}
EXPORT_SYMBOL_GPL(kernel_is_err_wrapper);

/**
 * kernel_ptr_err_wrapper - Get error from pointer
 * @ptr: Error pointer
 * 
 * Returns error code
 */
long kernel_ptr_err_wrapper(const void *ptr)
{
    return PTR_ERR(ptr);
}
EXPORT_SYMBOL_GPL(kernel_ptr_err_wrapper);

/**
 * kernel_err_ptr_wrapper - Create error pointer
 * @err: Error code
 * 
 * Returns error pointer
 */
void *kernel_err_ptr_wrapper(long err)
{
    return ERR_PTR(err);
}
EXPORT_SYMBOL_GPL(kernel_err_ptr_wrapper);

/**
 * Module initialization
 */
static int __init kernel_helpers_init(void)
{
    pr_info("Kernel helper functions initialized\n");
    return 0;
}

static void __exit kernel_helpers_exit(void)
{
    pr_info("Kernel helper functions cleanup\n");
}

module_init(kernel_helpers_init);
module_exit(kernel_helpers_exit);

MODULE_AUTHOR("Rust for Linux Contributors");
MODULE_DESCRIPTION("Kernel helper functions ported from Rust");
MODULE_LICENSE("GPL v2");