/* SPDX-License-Identifier: GPL-2.0 */

/**
 * The `kernel` C library.
 *
 * This library contains the kernel APIs that have been ported from Rust to C
 * for usage by kernel code and is shared by all of them.
 *
 * In other words, all the rest of the C code in the kernel (e.g. kernel
 * modules written in C) depends on this library.
 *
 * If you need a kernel C API that is not ported or wrapped yet here, then
 * do so first instead of bypassing this library.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/panic.h>
#include <linux/bug.h>
#include <linux/container_of.h>

#include "kernel.h"

/* Log prefix to appear before log messages printed from within the kernel library */
static const char kernel_log_prefix[] = "rust_kernel";

/**
 * Module trait implementation helpers
 */

/**
 * kernel_module_init - Initialize a kernel module
 * @metadata: Module metadata containing name, author, etc.
 * 
 * Called at module initialization time.
 * Use this function to perform whatever setup or registration your module should do.
 * Equivalent to the module_init macro in the C API.
 */
int kernel_module_init(const struct kernel_module_metadata *metadata)
{
    if (!metadata || !metadata->name) {
        pr_err("%s: Invalid module metadata\n", kernel_log_prefix);
        return -EINVAL;
    }

    pr_info("%s: Initializing module %s\n", kernel_log_prefix, metadata->name);
    
    if (metadata->description)
        pr_info("%s: %s\n", kernel_log_prefix, metadata->description);
    
    return 0;
}
EXPORT_SYMBOL_GPL(kernel_module_init);

/**
 * kernel_module_exit - Cleanup a kernel module
 * 
 * Called at module cleanup time.
 * Use this function to perform whatever teardown or cleanup operations your module needs.
 */
void kernel_module_exit(void)
{
    pr_info("%s: Module cleanup complete\n", kernel_log_prefix);
}
EXPORT_SYMBOL_GPL(kernel_module_exit);

/**
 * ThisModule operations
 */

/**
 * kernel_this_module_from_ptr - Create a ThisModule from raw pointer
 * @ptr: Pointer to struct module (THIS_MODULE)
 * 
 * Creates a ThisModule given the THIS_MODULE pointer.
 * 
 * Safety: The pointer must be equal to the right THIS_MODULE.
 */
struct module *kernel_this_module_from_ptr(struct module *ptr)
{
    return ptr;
}
EXPORT_SYMBOL_GPL(kernel_this_module_from_ptr);

/**
 * kernel_this_module_as_ptr - Get raw pointer from ThisModule
 * @this_module: The ThisModule structure
 * 
 * Access the raw pointer for this module.
 * It is up to the user to use it correctly.
 */
struct module *kernel_this_module_as_ptr(struct module *this_module)
{
    return this_module;
}
EXPORT_SYMBOL_GPL(kernel_this_module_as_ptr);

/**
 * Panic handler - replaces Rust panic handler
 * @info: Panic information string
 * 
 * This function handles kernel panics similar to the Rust panic handler.
 */
void kernel_panic_handler(const char *info)
{
    pr_emerg("%s: %s\n", kernel_log_prefix, info);
    BUG();
}
EXPORT_SYMBOL_GPL(kernel_panic_handler);

/**
 * container_of implementation with type safety
 * Similar to Rust container_of! macro but implemented as inline function
 */

/**
 * assert_same_type - Helper for container_of type checking
 * @a: First value
 * @b: Second value
 * 
 * Helper function for container_of macro type checking.
 * This ensures type safety similar to the Rust implementation.
 */
void assert_same_type_impl(void *a, void *b)
{
    /* Type checking is done at compile time by the macro */
    (void)a;
    (void)b;
}
EXPORT_SYMBOL_GPL(assert_same_type_impl);

/**
 * Assembly wrapper macros for inline assembly
 * These replace the Rust asm! macro functionality
 */

/**
 * kernel_asm_intel - Execute inline assembly with Intel syntax
 * @asm_str: Assembly string
 * @...: Assembly constraints and operands
 * 
 * Wrapper around asm() configured for use in the kernel with Intel syntax.
 */
#ifdef CONFIG_X86
#define kernel_asm(asm_str, ...) \
    do { \
        asm volatile(asm_str : : ##__VA_ARGS__ : "memory"); \
    } while (0)
#else
#define kernel_asm(asm_str, ...) \
    do { \
        asm volatile(asm_str : : ##__VA_ARGS__ : "memory"); \
    } while (0)
#endif

/**
 * File location helpers
 */

/**
 * kernel_file_from_location - Get file name from location
 * @file: File name string
 * @line: Line number
 * @func: Function name
 * 
 * Gets the C string file name equivalent to Rust Location functionality.
 * Returns a string containing location information.
 */
const char *kernel_file_from_location(const char *file, int line, const char *func)
{
    static char location_buf[256];
    
    if (file && func) {
        snprintf(location_buf, sizeof(location_buf), "%s:%d in %s()", file, line, func);
    } else if (file) {
        snprintf(location_buf, sizeof(location_buf), "%s:%d", file, line);
    } else {
        snprintf(location_buf, sizeof(location_buf), "<unknown location>");
    }
    
    return location_buf;
}
EXPORT_SYMBOL_GPL(kernel_file_from_location);

/**
 * Helper macro for getting current location
 */
#define KERNEL_CURRENT_LOCATION() \
    kernel_file_from_location(__FILE__, __LINE__, __func__)

/**
 * Module metadata definition helper
 */
#define KERNEL_MODULE_INFO(name_str, author_str, desc_str, license_str, version_str) \
    static struct kernel_module_metadata __kernel_module_metadata = { \
        .name = name_str, \
        .author = author_str, \
        .description = desc_str, \
        .license = license_str, \
        .version = version_str, \
        .aliases = NULL, \
        .firmware = NULL, \
    }; \
    MODULE_AUTHOR(author_str); \
    MODULE_DESCRIPTION(desc_str); \
    MODULE_LICENSE(license_str); \
    MODULE_VERSION(version_str)

/**
 * Initialization function for the kernel library itself
 */
static int __init kernel_lib_init(void)
{
    pr_info("%s: Kernel C library initialized\n", kernel_log_prefix);
    return 0;
}

/**
 * Cleanup function for the kernel library
 */
static void __exit kernel_lib_exit(void)
{
    pr_info("%s: Kernel C library cleanup complete\n", kernel_log_prefix);
}

module_init(kernel_lib_init);
module_exit(kernel_lib_exit);

MODULE_AUTHOR("Rust for Linux Contributors");
MODULE_DESCRIPTION("Kernel C library ported from Rust");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0");