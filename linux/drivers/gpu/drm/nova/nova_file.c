// SPDX-License-Identifier: GPL-2.0

/*
 * Nova DRM Driver - File operations
 *
 * C port of Rust nova DRM file module
 * Original Rust version: drivers/gpu/drm/nova/file.rs
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <drm/drm_device.h>
#include <drm/drm_file.h>
#include <drm/drm_gem.h>
#include <drm/drm_ioctl.h>

#include "nova_drv.h"
#include "nova_gem.h"

/* UAPI constants */
#define NOVA_GETPARAM_VRAM_BAR_SIZE	0x1

/**
 * struct drm_nova_getparam - Get parameter IOCTL structure
 */
struct drm_nova_getparam {
	__u32 param;
	__u32 pad;
	__u64 value;
};

/**
 * struct drm_nova_gem_create - GEM create IOCTL structure
 */
struct drm_nova_gem_create {
	__u64 size;
	__u32 handle;
	__u32 pad;
};

/**
 * struct drm_nova_gem_info - GEM info IOCTL structure
 */
struct drm_nova_gem_info {
	__u32 handle;
	__u32 pad;
	__u64 size;
};

/**
 * nova_drm_open - DRM file open callback
 * @dev: DRM device
 * @file: DRM file
 */
static int nova_drm_open(struct drm_device *dev, struct drm_file *file)
{
	struct nova_file_priv *priv;

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	file->driver_priv = priv;
	return 0;
}

/**
 * nova_drm_postclose - DRM file close callback
 * @dev: DRM device
 * @file: DRM file
 */
static void nova_drm_postclose(struct drm_device *dev, struct drm_file *file)
{
	struct nova_file_priv *priv = file->driver_priv;

	kfree(priv);
}

/**
 * nova_ioctl_getparam - Get parameter IOCTL handler
 * @dev: DRM device
 * @data: IOCTL data
 * @file: DRM file
 */
static int nova_ioctl_getparam(struct drm_device *dev, void *data,
			       struct drm_file *file)
{
	struct nova_device *nova_dev = to_nova_device(dev);
	struct drm_nova_getparam *getparam = data;
	struct pci_dev *pdev;
	u64 value;

	/* Get the parent PCI device */
	pdev = to_pci_dev(nova_dev->adev->dev.parent);
	if (!pdev)
		return -ENOENT;

	switch (getparam->param) {
	case NOVA_GETPARAM_VRAM_BAR_SIZE:
		value = pci_resource_len(pdev, 1);
		break;
	default:
		return -EINVAL;
	}

	getparam->value = value;
	return 0;
}

/**
 * nova_ioctl_gem_create - GEM create IOCTL handler
 * @dev: DRM device
 * @data: IOCTL data
 * @file: DRM file
 */
static int nova_ioctl_gem_create(struct drm_device *dev, void *data,
				 struct drm_file *file)
{
	struct nova_device *nova_dev = to_nova_device(dev);
	struct drm_nova_gem_create *req = data;
	struct nova_gem_object *obj;
	int ret;

	if (req->size == 0)
		return -EINVAL;

	obj = nova_gem_object_create(nova_dev, req->size);
	if (IS_ERR(obj))
		return PTR_ERR(obj);

	ret = drm_gem_handle_create(file, &obj->base.base, &req->handle);
	if (ret) {
		drm_gem_object_put(&obj->base.base);
		return ret;
	}

	/* Release the reference from create, handle holds one now */
	drm_gem_object_put(&obj->base.base);
	return 0;
}

/**
 * nova_ioctl_gem_info - GEM info IOCTL handler
 * @dev: DRM device
 * @data: IOCTL data
 * @file: DRM file
 */
static int nova_ioctl_gem_info(struct drm_device *dev, void *data,
			       struct drm_file *file)
{
	struct drm_nova_gem_info *req = data;
	struct drm_gem_object *gem_obj;
	struct nova_gem_object *obj;

	gem_obj = drm_gem_object_lookup(file, req->handle);
	if (!gem_obj)
		return -ENOENT;

	obj = to_nova_gem_object(gem_obj);
	req->size = obj->base.base.size;

	drm_gem_object_put(gem_obj);
	return 0;
}

/* IOCTL definitions */
#define DRM_NOVA_GETPARAM	0x00
#define DRM_NOVA_GEM_CREATE	0x01
#define DRM_NOVA_GEM_INFO	0x02

#define DRM_IOCTL_NOVA_GETPARAM		DRM_IOWR(DRM_COMMAND_BASE + DRM_NOVA_GETPARAM, struct drm_nova_getparam)
#define DRM_IOCTL_NOVA_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + DRM_NOVA_GEM_CREATE, struct drm_nova_gem_create)
#define DRM_IOCTL_NOVA_GEM_INFO		DRM_IOWR(DRM_COMMAND_BASE + DRM_NOVA_GEM_INFO, struct drm_nova_gem_info)

static const struct drm_ioctl_desc nova_ioctls[] = {
	DRM_IOCTL_DEF_DRV(NOVA_GETPARAM, nova_ioctl_getparam, DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(NOVA_GEM_CREATE, nova_ioctl_gem_create, DRM_AUTH | DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(NOVA_GEM_INFO, nova_ioctl_gem_info, DRM_AUTH | DRM_RENDER_ALLOW),
};

const struct file_operations nova_drm_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.compat_ioctl = drm_compat_ioctl,
	.poll = drm_poll,
	.read = drm_read,
	.llseek = noop_llseek,
	.mmap = drm_gem_mmap,
};

const struct drm_driver nova_drm_driver = {
	.driver_features = DRIVER_GEM | DRIVER_RENDER,
	.open = nova_drm_open,
	.postclose = nova_drm_postclose,
	.ioctls = nova_ioctls,
	.num_ioctls = ARRAY_SIZE(nova_ioctls),
	.fops = &nova_drm_fops,
	.name = "nova",
	.desc = "Nova GPU",
	.date = "20240101",
	.major = 0,
	.minor = 0,
	.patchlevel = 0,
};