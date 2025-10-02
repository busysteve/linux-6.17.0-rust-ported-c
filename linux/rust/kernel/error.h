/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel error handling - C port of kernel/error.rs
 * 
 * Contains the C-compatible error codes and error handling utilities.
 */

#ifndef _LINUX_KERNEL_ERROR_H
#define _LINUX_KERNEL_ERROR_H

#include <linux/errno.h>
#include <linux/err.h>
#include <uapi/asm-generic/errno-base.h>

/**
 * Error code constants - replaces Rust error::code module
 */
#define KERNEL_EPERM        (-EPERM)        /* Operation not permitted */
#define KERNEL_ENOENT       (-ENOENT)       /* No such file or directory */
#define KERNEL_ESRCH        (-ESRCH)        /* No such process */
#define KERNEL_EINTR        (-EINTR)        /* Interrupted system call */
#define KERNEL_EIO          (-EIO)          /* I/O error */
#define KERNEL_ENXIO        (-ENXIO)        /* No such device or address */
#define KERNEL_E2BIG        (-E2BIG)        /* Argument list too long */
#define KERNEL_ENOEXEC      (-ENOEXEC)      /* Exec format error */
#define KERNEL_EBADF        (-EBADF)        /* Bad file number */
#define KERNEL_ECHILD       (-ECHILD)       /* No child processes */
#define KERNEL_EAGAIN       (-EAGAIN)       /* Try again */
#define KERNEL_ENOMEM       (-ENOMEM)       /* Out of memory */
#define KERNEL_EACCES       (-EACCES)       /* Permission denied */
#define KERNEL_EFAULT       (-EFAULT)       /* Bad address */
#define KERNEL_ENOTBLK      (-ENOTBLK)      /* Block device required */
#define KERNEL_EBUSY        (-EBUSY)        /* Device or resource busy */
#define KERNEL_EEXIST       (-EEXIST)       /* File exists */
#define KERNEL_EXDEV        (-EXDEV)        /* Cross-device link */
#define KERNEL_ENODEV       (-ENODEV)       /* No such device */
#define KERNEL_ENOTDIR      (-ENOTDIR)      /* Not a directory */
#define KERNEL_EISDIR       (-EISDIR)       /* Is a directory */
#define KERNEL_EINVAL       (-EINVAL)       /* Invalid argument */
#define KERNEL_ENFILE       (-ENFILE)       /* File table overflow */
#define KERNEL_EMFILE       (-EMFILE)       /* Too many open files */
#define KERNEL_ENOTTY       (-ENOTTY)       /* Not a typewriter */
#define KERNEL_ETXTBSY      (-ETXTBSY)      /* Text file busy */
#define KERNEL_EFBIG        (-EFBIG)        /* File too large */
#define KERNEL_ENOSPC       (-ENOSPC)       /* No space left on device */
#define KERNEL_ESPIPE       (-ESPIPE)       /* Illegal seek */
#define KERNEL_EROFS        (-EROFS)        /* Read-only file system */
#define KERNEL_EMLINK       (-EMLINK)       /* Too many links */
#define KERNEL_EPIPE        (-EPIPE)        /* Broken pipe */
#define KERNEL_EDOM         (-EDOM)         /* Math argument out of domain */
#define KERNEL_ERANGE       (-ERANGE)       /* Math result not representable */
#define KERNEL_EOVERFLOW    (-EOVERFLOW)    /* Value too large for defined data type */
#define KERNEL_ETIMEDOUT    (-ETIMEDOUT)    /* Connection timed out */
#define KERNEL_ERESTARTSYS  (-ERESTARTSYS)  /* Restart the system call */
#define KERNEL_ERESTARTNOINTR (-ERESTARTNOINTR) /* System call was interrupted by a signal and will be restarted */
#define KERNEL_ERESTARTNOHAND (-ERESTARTNOHAND) /* Restart if no handler */
#define KERNEL_ENOIOCTLCMD  (-ENOIOCTLCMD)  /* No ioctl command */
#define KERNEL_ERESTART_RESTARTBLOCK (-ERESTART_RESTARTBLOCK) /* Restart by calling sys_restart_syscall */
#define KERNEL_EPROBE_DEFER (-EPROBE_DEFER) /* Driver requests probe retry */
#define KERNEL_EOPENSTALE   (-EOPENSTALE)   /* Open found a stale dentry */
#define KERNEL_ENOPARAM     (-ENOPARAM)     /* Parameter not supported */

/**
 * Generic integer kernel error
 * Replaces Rust Error struct
 */
typedef int kernel_error_t;

/**
 * Result type - replaces Rust Result<T, Error>
 */
#define KERNEL_RESULT(type) \
    struct { \
        union { \
            type ok; \
            kernel_error_t err; \
        }; \
        bool is_err; \
    }

/**
 * Helper macros for creating results
 */
#define KERNEL_OK_RESULT(val) { .ok = (val), .is_err = false }
#define KERNEL_ERR_RESULT(err) { .err = (err), .is_err = true }

/**
 * Error checking macros
 */
#define IS_KERNEL_ERR(result) ((result).is_err)
#define IS_KERNEL_OK(result) (!(result).is_err)

#define KERNEL_UNWRAP(result) \
    ({ \
        if (IS_KERNEL_ERR(result)) { \
            panic("Called KERNEL_UNWRAP on error result: %d", (result).err); \
        } \
        (result).ok; \
    })

#define KERNEL_UNWRAP_OR(result, default_val) \
    (IS_KERNEL_OK(result) ? (result).ok : (default_val))

#define KERNEL_UNWRAP_ERR(result) \
    ({ \
        if (IS_KERNEL_OK(result)) { \
            panic("Called KERNEL_UNWRAP_ERR on ok result"); \
        } \
        (result).err; \
    })

/**
 * Error creation functions
 */
static inline kernel_error_t kernel_error_from_errno(int errno_val)
{
    return -abs(errno_val);
}

static inline int kernel_error_to_errno(kernel_error_t error)
{
    return -error;
}

static inline bool kernel_error_is_valid(kernel_error_t error)
{
    return error < 0 && error >= -MAX_ERRNO;
}

/**
 * Try macros for error propagation
 */
#define KERNEL_TRY(expr, result_type) \
    ({ \
        auto __tmp_result = (expr); \
        if (IS_KERNEL_ERR(__tmp_result)) { \
            KERNEL_RESULT(result_type) __err_result; \
            __err_result.err = __tmp_result.err; \
            __err_result.is_err = true; \
            return __err_result; \
        } \
        __tmp_result.ok; \
    })

/**
 * Error conversion functions for common kernel error types
 */
static inline kernel_error_t kernel_error_from_alloc_error(void)
{
    return KERNEL_ENOMEM;
}

static inline kernel_error_t kernel_error_from_layout_error(void)
{
    return KERNEL_EINVAL;
}

static inline kernel_error_t kernel_error_from_try_from_int_error(void)
{
    return KERNEL_ERANGE;
}

static inline kernel_error_t kernel_error_from_utf8_error(void)
{
    return KERNEL_EINVAL;
}

/**
 * VTABLE_DEFAULT_ERROR constant for vtable defaults
 */
#define VTABLE_DEFAULT_ERROR KERNEL_ENOSYS

/**
 * Display and debugging support
 */
static inline void kernel_error_print(kernel_error_t error)
{
    if (kernel_error_is_valid(error)) {
        printk(KERN_ERR "Kernel error: %d (%s)\n", 
               kernel_error_to_errno(error), 
               strerror(kernel_error_to_errno(error)));
    } else {
        printk(KERN_ERR "Invalid kernel error: %d\n", error);
    }
}

#endif /* _LINUX_KERNEL_ERROR_H */