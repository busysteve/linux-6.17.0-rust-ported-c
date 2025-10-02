// SPDX-License-Identifier: GPL-2.0

/*
 * Nova DRM Driver - Main module file
 *
 * C port of Rust nova DRM driver
 * Original Rust version: drivers/gpu/drm/nova/nova.rs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/auxiliary_bus.h>

/* Forward declarations */
extern struct auxiliary_driver nova_auxiliary_driver;

/**
 * nova_drm_init - Module initialization
 */
static int __init nova_drm_init(void)
{
	int ret;

	pr_info("Nova DRM driver initializing\n");

	ret = auxiliary_driver_register(&nova_auxiliary_driver);
	if (ret) {
		pr_err("Failed to register auxiliary driver: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * nova_drm_exit - Module cleanup
 */
static void __exit nova_drm_exit(void)
{
	auxiliary_driver_unregister(&nova_auxiliary_driver);
	pr_info("Nova DRM driver unloaded\n");
}

module_init(nova_drm_init);
module_exit(nova_drm_exit);

MODULE_DESCRIPTION("Nova GPU driver");
MODULE_AUTHOR("Danilo Krummrich");
MODULE_LICENSE("GPL v2");