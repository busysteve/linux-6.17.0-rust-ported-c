/* SPDX-License-Identifier: GPL-2.0 */

/**
 * Kernel C macros - replaces Rust procedural macros
 */

#ifndef _LINUX_KERNEL_MACROS_H
#define _LINUX_KERNEL_MACROS_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/stringify.h>

/**
 * Module declaration macro - replaces Rust module! macro
 * 
 * Usage:
 * KERNEL_MODULE_DECLARE(
 *     .type = my_module_type,
 *     .name = "my_kernel_module", 
 *     .author = "Author Name",
 *     .description = "My kernel module",
 *     .license = "GPL",
 *     .version = "1.0"
 * );
 */
#define KERNEL_MODULE_DECLARE(args...) \
    KERNEL_MODULE_DECLARE_IMPL(args)

#define KERNEL_MODULE_DECLARE_IMPL(...) \
    static struct kernel_module_config { \
        __VA_ARGS__ \
    } __module_config; \
    \
    static int __init __kernel_module_init(void) { \
        return kernel_module_init_with_config(&__module_config); \
    } \
    \
    static void __exit __kernel_module_exit(void) { \
        kernel_module_exit_with_config(&__module_config); \
    } \
    \
    module_init(__kernel_module_init); \
    module_exit(__kernel_module_exit); \
    \
    MODULE_AUTHOR(__module_config.author); \
    MODULE_DESCRIPTION(__module_config.description); \
    MODULE_LICENSE(__module_config.license); \
    MODULE_VERSION(__module_config.version)

/**
 * Module configuration structure
 */
struct kernel_module_config {
    const char *name;
    const char *author;
    const char *description;
    const char *license;
    const char *version;
    const char **aliases;
    const char **firmware;
    int (*init_fn)(struct module *);
    void (*exit_fn)(void);
};

/**
 * VTable macro - replaces Rust #[vtable] attribute
 * 
 * Usage:
 * KERNEL_VTABLE_DECLARE(my_ops,
 *     int (*operation1)(void *self);
 *     int (*operation2)(void *self, int arg);
 * );
 */
#define KERNEL_VTABLE_DECLARE(name, ops...) \
    struct name##_vtable { \
        ops \
    }; \
    \
    struct name##_has_flags { \
        KERNEL_VTABLE_HAS_FLAGS(ops) \
    }

/**
 * Helper to generate HAS_* flags for vtable methods
 */
#define KERNEL_VTABLE_HAS_FLAGS(ops) \
    KERNEL_VTABLE_HAS_FLAGS_IMPL(ops)

#define KERNEL_VTABLE_HAS_FLAGS_IMPL(...) \
    /* This would need to be implemented per vtable */ \
    bool has_method : 1;

/**
 * VTable implementation macro
 */
#define KERNEL_VTABLE_IMPL(type, vtable_name, ...) \
    static const struct vtable_name##_vtable type##_vtable = { \
        __VA_ARGS__ \
    }; \
    \
    static const struct vtable_name##_has_flags type##_has_flags = { \
        KERNEL_VTABLE_IMPL_HAS_FLAGS(__VA_ARGS__) \
    }

/**
 * Export macro - replaces Rust #[export] attribute
 * 
 * Usage:
 * KERNEL_EXPORT(my_function, "Expected C signature");
 */
#define KERNEL_EXPORT(func, signature) \
    extern typeof(func) func; \
    EXPORT_SYMBOL_GPL(func); \
    static const char __attribute__((unused)) \
        __##func##_signature[] = signature

/**
 * Identifier concatenation - replaces Rust concat_idents! macro
 * 
 * Usage:
 * KERNEL_CONCAT_IDENTS(prefix, suffix) -> prefix##suffix
 */
#define KERNEL_CONCAT_IDENTS(a, b) a##b

/**
 * Paste macro - replaces Rust paste! macro
 * 
 * Usage:
 * KERNEL_PASTE(some_, foo, _bar) -> some_foo_bar
 */
#define KERNEL_PASTE(...) KERNEL_PASTE_IMPL(__VA_ARGS__)
#define KERNEL_PASTE_IMPL(...) KERNEL_PASTE_HELPER(__VA_ARGS__)
#define KERNEL_PASTE_HELPER(a, ...) a##__VA_ARGS__

/**
 * Advanced paste with modifiers
 */
#define KERNEL_PASTE_LOWER(str) KERNEL_TOLOWER(str)
#define KERNEL_PASTE_UPPER(str) KERNEL_TOUPPER(str)

/* Helper macros for case conversion (limited support) */
#define KERNEL_TOLOWER(str) str  /* Would need compiler support */
#define KERNEL_TOUPPER(str) str  /* Would need compiler support */

/**
 * KUnit test macro - replaces Rust #[kunit_tests] attribute
 * 
 * Usage:
 * KERNEL_KUNIT_TEST_SUITE(my_test_suite) {
 *     KERNEL_KUNIT_TEST(test_function_1);
 *     KERNEL_KUNIT_TEST(test_function_2);
 * }
 */
#ifdef CONFIG_KUNIT

#include <kunit/test.h>

#define KERNEL_KUNIT_TEST_SUITE(suite_name) \
    static struct kunit_case suite_name##_test_cases[] = 

#define KERNEL_KUNIT_TEST(test_name) \
    KUNIT_CASE(test_name)

#define KERNEL_KUNIT_TEST_SUITE_END(suite_name) \
    {}; \
    \
    static struct kunit_suite suite_name##_test_suite = { \
        .name = #suite_name, \
        .test_cases = suite_name##_test_cases, \
    }; \
    \
    kunit_test_suite(suite_name##_test_suite)

#define KERNEL_KUNIT_ASSERT_EQ(test, left, right) \
    KUNIT_ASSERT_EQ(test, left, right)

#define KERNEL_KUNIT_EXPECT_EQ(test, left, right) \
    KUNIT_EXPECT_EQ(test, left, right)

#else

/* KUnit disabled - provide empty implementations */
#define KERNEL_KUNIT_TEST_SUITE(suite_name) \
    static void __attribute__((unused)) suite_name##_dummy(void) {}

#define KERNEL_KUNIT_TEST(test_name) /* empty */
#define KERNEL_KUNIT_TEST_SUITE_END(suite_name) /* empty */
#define KERNEL_KUNIT_ASSERT_EQ(test, left, right) /* empty */
#define KERNEL_KUNIT_EXPECT_EQ(test, left, right) /* empty */

#endif /* CONFIG_KUNIT */

/**
 * Build error macro - replaces Rust build_error! macro
 */
#define KERNEL_BUILD_ERROR(msg) \
    _Static_assert(0, msg)

/**
 * Container of macro - enhanced version
 */
#define KERNEL_CONTAINER_OF(ptr, type, member) \
    ({ \
        const typeof(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member)); \
    })

/**
 * Type assertion macro
 */
#define KERNEL_ASSERT_SAME_TYPE(a, b) \
    _Static_assert(__builtin_types_compatible_p(typeof(a), typeof(b)), \
                   "Types must be the same")

/**
 * Assembly macro - replaces Rust asm! macro
 */
#ifdef CONFIG_X86
#define KERNEL_ASM(asm_str, ...) \
    asm volatile(asm_str : : ##__VA_ARGS__ : "memory")
#else
#define KERNEL_ASM(asm_str, ...) \
    asm volatile(asm_str : : ##__VA_ARGS__ : "memory")
#endif

/**
 * Inline assembly with AT&T syntax for x86
 */
#ifdef CONFIG_X86
#define KERNEL_ASM_ATT(asm_str, ...) \
    asm volatile(asm_str : : ##__VA_ARGS__ : "memory")
#else
#define KERNEL_ASM_ATT(asm_str, ...) \
    KERNEL_ASM(asm_str, ##__VA_ARGS__)
#endif

/**
 * Literal concatenation - replaces Rust concat_literals! macro
 */
#define KERNEL_CONCAT_LITERALS(...) \
    KERNEL_STRINGIFY(__VA_ARGS__)

/**
 * Module metadata extraction macros
 */
#define KERNEL_MODULE_NAME(config) ((config)->name)
#define KERNEL_MODULE_AUTHOR(config) ((config)->author)
#define KERNEL_MODULE_DESCRIPTION(config) ((config)->description)
#define KERNEL_MODULE_LICENSE(config) ((config)->license)
#define KERNEL_MODULE_VERSION(config) ((config)->version)

/**
 * Conditional compilation helpers
 */
#define KERNEL_IF_ENABLED(config, code) \
    __maybe_unused static void config##_enabled(void) { \
        if (IS_ENABLED(CONFIG_##config)) { \
            code \
        } \
    }

/**
 * Feature testing macros
 */
#define KERNEL_HAS_FEATURE(feature) IS_ENABLED(CONFIG_##feature)

/**
 * Rust-style attribute emulation
 */
#define KERNEL_MAYBE_UNUSED __maybe_unused
#define KERNEL_MUST_USE __must_check
#define KERNEL_DEPRECATED __deprecated
#define KERNEL_COLD __cold
#define KERNEL_HOT __hot

/**
 * Error propagation helpers
 */
#define KERNEL_TRY_ASSIGN(var, expr) \
    do { \
        typeof(expr) __tmp = (expr); \
        if (IS_ERR(__tmp)) \
            return PTR_ERR(__tmp); \
        (var) = __tmp; \
    } while (0)

#define KERNEL_TRY_RETURN(expr) \
    do { \
        int __ret = (expr); \
        if (__ret < 0) \
            return __ret; \
    } while (0)

/**
 * Module parameter helpers
 */
#define KERNEL_MODULE_PARAM(name, type, perm) \
    module_param(name, type, perm)

#define KERNEL_MODULE_PARAM_DESC(name, desc) \
    MODULE_PARM_DESC(name, desc)

/**
 * Module alias helpers
 */
#define KERNEL_MODULE_ALIAS(alias) \
    MODULE_ALIAS(alias)

#define KERNEL_MODULE_DEVICE_TABLE(type, table) \
    MODULE_DEVICE_TABLE(type, table)

#endif /* _LINUX_KERNEL_MACROS_H */