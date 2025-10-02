/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel error handling implementation
 * C port of kernel/error.rs
 */

#include <linux/errno.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/printk.h>

#include "error.h"

/**
 * Error constants - implementation of the error codes
 */
static const char *kernel_error_names[] = {
    [1] = "EPERM",
    [2] = "ENOENT", 
    [3] = "ESRCH",
    [4] = "EINTR",
    [5] = "EIO",
    [6] = "ENXIO",
    [7] = "E2BIG",
    [8] = "ENOEXEC",
    [9] = "EBADF",
    [10] = "ECHILD",
    [11] = "EAGAIN",
    [12] = "ENOMEM",
    [13] = "EACCES",
    [14] = "EFAULT",
    [15] = "ENOTBLK",
    [16] = "EBUSY",
    [17] = "EEXIST",
    [18] = "EXDEV",
    [19] = "ENODEV",
    [20] = "ENOTDIR",
    [21] = "EISDIR",
    [22] = "EINVAL",
    [23] = "ENFILE",
    [24] = "EMFILE",
    [25] = "ENOTTY",
    [26] = "ETXTBSY",
    [27] = "EFBIG",
    [28] = "ENOSPC",
    [29] = "ESPIPE",
    [30] = "EROFS",
    [31] = "EMLINK",
    [32] = "EPIPE",
    [33] = "EDOM",
    [34] = "ERANGE",
    [75] = "EOVERFLOW",
    [110] = "ETIMEDOUT",
    [512] = "ERESTARTSYS",
    [513] = "ERESTARTNOINTR",
    [514] = "ERESTARTNOHAND",
    [515] = "ENOIOCTLCMD",
    [516] = "ERESTART_RESTARTBLOCK",
    [517] = "EPROBE_DEFER",
    [518] = "EOPENSTALE",
    [519] = "ENOPARAM",
};

static const char *kernel_error_descriptions[] = {
    [1] = "Operation not permitted",
    [2] = "No such file or directory",
    [3] = "No such process", 
    [4] = "Interrupted system call",
    [5] = "I/O error",
    [6] = "No such device or address",
    [7] = "Argument list too long",
    [8] = "Exec format error",
    [9] = "Bad file number",
    [10] = "No child processes",
    [11] = "Try again",
    [12] = "Out of memory",
    [13] = "Permission denied",
    [14] = "Bad address",
    [15] = "Block device required",
    [16] = "Device or resource busy",
    [17] = "File exists",
    [18] = "Cross-device link",
    [19] = "No such device",
    [20] = "Not a directory",
    [21] = "Is a directory",
    [22] = "Invalid argument",
    [23] = "File table overflow",
    [24] = "Too many open files",
    [25] = "Not a typewriter",
    [26] = "Text file busy",
    [27] = "File too large",
    [28] = "No space left on device",
    [29] = "Illegal seek",
    [30] = "Read-only file system",
    [31] = "Too many links",
    [32] = "Broken pipe",
    [33] = "Math argument out of domain of func",
    [34] = "Math result not representable",
    [75] = "Value too large for defined data type",
    [110] = "Connection timed out",
    [512] = "Restart the system call",
    [513] = "System call was interrupted by a signal and will be restarted",
    [514] = "Restart if no handler",
    [515] = "No ioctl command",
    [516] = "Restart by calling sys_restart_syscall",
    [517] = "Driver requests probe retry",
    [518] = "Open found a stale dentry",
    [519] = "Parameter not supported",
};

/**
 * kernel_error_try_from_errno - Create error from errno value
 * @errno_val: The errno value (positive)
 * 
 * Returns valid kernel error or invalid error if errno is out of range
 */
kernel_error_t kernel_error_try_from_errno(int errno_val)
{
    /* Ensure errno is positive */
    errno_val = abs(errno_val);
    
    /* Check if errno is in valid range */
    if (errno_val == 0 || errno_val > MAX_ERRNO)
        return 0; /* Invalid error */
    
    return -errno_val;
}
EXPORT_SYMBOL_GPL(kernel_error_try_from_errno);

/**
 * kernel_error_from_errno - Create error from errno (unchecked)
 * @errno_val: The errno value
 * 
 * Returns kernel error. Assumes errno is valid.
 */
kernel_error_t kernel_error_from_errno_unchecked(int errno_val)
{
    return -abs(errno_val);
}
EXPORT_SYMBOL_GPL(kernel_error_from_errno_unchecked);

/**
 * kernel_error_to_errno_unchecked - Convert error to errno
 * @error: The kernel error
 * 
 * Returns positive errno value
 */
int kernel_error_to_errno_unchecked(kernel_error_t error)
{
    return -error;
}
EXPORT_SYMBOL_GPL(kernel_error_to_errno_unchecked);

/**
 * kernel_error_name - Get error name string
 * @error: The kernel error
 * 
 * Returns string name of the error or "UNKNOWN" if not found
 */
const char *kernel_error_name(kernel_error_t error)
{
    int errno_val = kernel_error_to_errno(error);
    
    if (errno_val < 0 || errno_val >= ARRAY_SIZE(kernel_error_names))
        return "UNKNOWN";
    
    if (!kernel_error_names[errno_val])
        return "UNKNOWN";
    
    return kernel_error_names[errno_val];
}
EXPORT_SYMBOL_GPL(kernel_error_name);

/**
 * kernel_error_description - Get error description string
 * @error: The kernel error
 * 
 * Returns description string of the error or "Unknown error" if not found
 */
const char *kernel_error_description(kernel_error_t error)
{
    int errno_val = kernel_error_to_errno(error);
    
    if (errno_val < 0 || errno_val >= ARRAY_SIZE(kernel_error_descriptions))
        return "Unknown error";
    
    if (!kernel_error_descriptions[errno_val])
        return "Unknown error";
    
    return kernel_error_descriptions[errno_val];
}
EXPORT_SYMBOL_GPL(kernel_error_description);

/**
 * Error conversion from various kernel error types
 */

/**
 * kernel_error_from_errno_impl - Implementation of from_errno
 * @errno_val: Errno value to convert
 * 
 * Returns kernel error
 */
kernel_error_t kernel_error_from_errno_impl(int errno_val)
{
    return kernel_error_from_errno(errno_val);
}
EXPORT_SYMBOL_GPL(kernel_error_from_errno_impl);

/**
 * kernel_error_from_ptr_err - Convert PTR_ERR to kernel error
 * @ptr: Error pointer from PTR_ERR
 * 
 * Returns kernel error
 */
kernel_error_t kernel_error_from_ptr_err(long ptr_err)
{
    if (ptr_err > -MAX_ERRNO && ptr_err < 0)
        return (kernel_error_t)ptr_err;
    
    return KERNEL_EINVAL;
}
EXPORT_SYMBOL_GPL(kernel_error_from_ptr_err);

/**
 * Error debugging and display support
 */

/**
 * kernel_error_print_impl - Enhanced error printing
 * @error: The kernel error
 * @prefix: Optional prefix string
 * @file: Source file name (optional)
 * @line: Source line number
 * @func: Function name (optional)
 */
void kernel_error_print_impl(kernel_error_t error, const char *prefix,
                             const char *file, int line, const char *func)
{
    const char *err_name = kernel_error_name(error);
    const char *err_desc = kernel_error_description(error);
    int errno_val = kernel_error_to_errno(error);
    
    if (prefix && file && func) {
        printk(KERN_ERR "%s: Error %s (%d): %s at %s:%d in %s()\n",
               prefix, err_name, errno_val, err_desc, file, line, func);
    } else if (prefix) {
        printk(KERN_ERR "%s: Error %s (%d): %s\n",
               prefix, err_name, errno_val, err_desc);
    } else {
        printk(KERN_ERR "Kernel error %s (%d): %s\n",
               err_name, errno_val, err_desc);
    }
}
EXPORT_SYMBOL_GPL(kernel_error_print_impl);

/**
 * Helper macros implementation
 */

/**
 * kernel_error_print_debug - Debug version of error printing
 * @error: The kernel error
 */
void kernel_error_print_debug(kernel_error_t error)
{
    kernel_error_print_impl(error, "DEBUG", __FILE__, __LINE__, __func__);
}
EXPORT_SYMBOL_GPL(kernel_error_print_debug);

/**
 * Result helper functions
 */

/**
 * kernel_result_is_ok_impl - Check if result is OK (implementation helper)
 * @is_err: Error flag from result
 * 
 * Returns true if result is OK
 */
bool kernel_result_is_ok_impl(bool is_err)
{
    return !is_err;
}
EXPORT_SYMBOL_GPL(kernel_result_is_ok_impl);

/**
 * kernel_result_is_err_impl - Check if result is error (implementation helper)
 * @is_err: Error flag from result  
 * 
 * Returns true if result is error
 */
bool kernel_result_is_err_impl(bool is_err)
{
    return is_err;
}
EXPORT_SYMBOL_GPL(kernel_result_is_err_impl);

/**
 * Error context and backtrace support (basic implementation)
 */

/**
 * kernel_error_context - Structure for error context
 */
struct kernel_error_context {
    kernel_error_t error;
    const char *message;
    const char *file;
    int line;
    const char *func;
};

/**
 * kernel_error_with_context - Create error with context
 * @error: The base error
 * @message: Context message
 * @file: Source file
 * @line: Source line
 * @func: Function name
 * 
 * Returns error with context information logged
 */
kernel_error_t kernel_error_with_context(kernel_error_t error, const char *message,
                                         const char *file, int line, const char *func)
{
    if (message) {
        printk(KERN_ERR "Error context: %s\n", message);
    }
    
    kernel_error_print_impl(error, "CONTEXT", file, line, func);
    
    return error;
}
EXPORT_SYMBOL_GPL(kernel_error_with_context);

/**
 * Convenience macro for error with context
 */
#define KERNEL_ERR_WITH_CONTEXT(err, msg) \
    kernel_error_with_context(err, msg, __FILE__, __LINE__, __func__)

/**
 * Module initialization
 */
static int __init kernel_error_init(void)
{
    pr_info("Kernel error handling module initialized\n");
    return 0;
}

static void __exit kernel_error_exit(void)
{
    pr_info("Kernel error handling module cleanup\n");
}

module_init(kernel_error_init);
module_exit(kernel_error_exit);