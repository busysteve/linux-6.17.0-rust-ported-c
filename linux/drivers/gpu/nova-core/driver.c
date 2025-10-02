// SPDX-License-Identifier: GPL-2.0

/*
 * Nova Core GPU Driver - Main driver file
 *
 * C port of Rust nova-core driver module
 * Original Rust version: drivers/gpu/nova-core/driver.rs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/auxiliary_bus.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/sizes.h>

#define BAR0_SIZE	SZ_16M

/**
 * struct nova_core - Nova Core device structure
 */
struct nova_core {
	struct pci_dev *pdev;
	void __iomem *bar0;
	struct auxiliary_device aux_dev;
	bool aux_registered;
};

/* PCI device table */
static const struct pci_device_id nova_core_pci_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_NVIDIA, PCI_ANY_ID) },
	{ }
};
MODULE_DEVICE_TABLE(pci, nova_core_pci_table);

/**
 * nova_core_probe - PCI probe function
 * @pdev: PCI device
 * @id: PCI device ID
 */
static int nova_core_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct nova_core *nova;
	int ret;

	dev_dbg(&pdev->dev, "Probe Nova Core GPU driver.\n");

	nova = devm_kzalloc(&pdev->dev, sizeof(*nova), GFP_KERNEL);
	if (!nova)
		return -ENOMEM;

	nova->pdev = pdev;
	pci_set_drvdata(pdev, nova);

	/* Enable device and setup memory access */
	ret = pci_enable_device_mem(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to enable PCI device: %d\n", ret);
		return ret;
	}

	pci_set_master(pdev);

	/* Map BAR0 */
	ret = pci_request_region(pdev, 0, "nova-core/bar0");
	if (ret) {
		dev_err(&pdev->dev, "Failed to request BAR0: %d\n", ret);
		goto err_disable_device;
	}

	nova->bar0 = pci_iomap(pdev, 0, BAR0_SIZE);
	if (!nova->bar0) {
		dev_err(&pdev->dev, "Failed to map BAR0\n");
		ret = -ENOMEM;
		goto err_release_region;
	}

	/* Initialize auxiliary device for DRM communication */
	nova->aux_dev.name = "nova-drm";
	nova->aux_dev.dev.parent = &pdev->dev;
	nova->aux_dev.dev.release = NULL; /* Will be managed by devm */
	nova->aux_dev.id = 0;

	ret = auxiliary_device_init(&nova->aux_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to initialize auxiliary device: %d\n", ret);
		goto err_unmap;
	}

	ret = auxiliary_device_add(&nova->aux_dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add auxiliary device: %d\n", ret);
		goto err_uninit_aux;
	}

	nova->aux_registered = true;

	dev_info(&pdev->dev, "Nova Core GPU driver loaded successfully\n");
	return 0;

err_uninit_aux:
	auxiliary_device_uninit(&nova->aux_dev);
err_unmap:
	pci_iounmap(pdev, nova->bar0);
err_release_region:
	pci_release_region(pdev, 0);
err_disable_device:
	pci_disable_device(pdev);
	return ret;
}

/**
 * nova_core_remove - PCI remove function
 * @pdev: PCI device
 */
static void nova_core_remove(struct pci_dev *pdev)
{
	struct nova_core *nova = pci_get_drvdata(pdev);

	if (!nova)
		return;

	if (nova->aux_registered) {
		auxiliary_device_delete(&nova->aux_dev);
		auxiliary_device_uninit(&nova->aux_dev);
	}

	if (nova->bar0)
		pci_iounmap(pdev, nova->bar0);

	pci_release_region(pdev, 0);
	pci_disable_device(pdev);

	dev_info(&pdev->dev, "Nova Core GPU driver unloaded\n");
}

/**
 * nova_core_shutdown - PCI shutdown function
 * @pdev: PCI device
 */
static void nova_core_shutdown(struct pci_dev *pdev)
{
	struct nova_core *nova = pci_get_drvdata(pdev);

	if (nova && nova->aux_registered) {
		auxiliary_device_delete(&nova->aux_dev);
		nova->aux_registered = false;
	}
}

static struct pci_driver nova_core_pci_driver = {
	.name = "NovaCore",
	.id_table = nova_core_pci_table,
	.probe = nova_core_probe,
	.remove = nova_core_remove,
	.shutdown = nova_core_shutdown,
};

/**
 * nova_core_init - Module initialization
 */
static int __init nova_core_init(void)
{
	int ret;

	pr_info("Nova Core GPU driver initializing\n");

	ret = pci_register_driver(&nova_core_pci_driver);
	if (ret) {
		pr_err("Failed to register PCI driver: %d\n", ret);
		return ret;
	}

	return 0;
}

/**
 * nova_core_exit - Module cleanup
 */
static void __exit nova_core_exit(void)
{
	pci_unregister_driver(&nova_core_pci_driver);
	pr_info("Nova Core GPU driver unloaded\n");
}

module_init(nova_core_init);
module_exit(nova_core_exit);

MODULE_DESCRIPTION("Nova Core GPU driver");
MODULE_AUTHOR("Danilo Krummrich");
MODULE_LICENSE("GPL v2");
MODULE_FIRMWARE("nova-core-firmware.bin"); /* Placeholder firmware name */