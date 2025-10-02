// SPDX-License-Identifier: GPL-2.0

/*
 * Nova Core GPU Driver - Main module
 *
 * C port of Rust nova-core module
 * Original Rust version: drivers/gpu/nova-core/nova_core.rs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

/* External function declarations */
extern int nova_core_init(void);
extern void nova_core_exit(void);

/**
 * nova_core_module_init - Module initialization wrapper
 */
static int __init nova_core_module_init(void)
{
	return nova_core_init();
}

/**
 * nova_core_module_exit - Module cleanup wrapper
 */
static void __exit nova_core_module_exit(void)
{
	nova_core_exit();
}

module_init(nova_core_module_init);
module_exit(nova_core_module_exit);

MODULE_DESCRIPTION("Nova Core GPU driver");
MODULE_AUTHOR("Danilo Krummrich");
MODULE_LICENSE("GPL v2");
MODULE_FIRMWARE("nova-core-firmware.bin");