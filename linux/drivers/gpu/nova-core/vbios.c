// SPDX-License-Identifier: GPL-2.0

/*
 * VBIOS extraction and parsing.
 *
 * C port of Rust nova-core VBIOS module
 * Original Rust version: drivers/gpu/nova-core/vbios.rs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/string.h>

/* The offset of the VBIOS ROM in the BAR0 space. */
#define ROM_OFFSET		0x300000
/* The maximum length of the VBIOS ROM to scan into. */
#define BIOS_MAX_SCAN_LEN	0x100000
/* The size to read ahead when parsing initial BIOS image headers. */
#define BIOS_READ_AHEAD_SIZE	1024
/* The bit in the last image indicator byte for the PCI Data Structure that
 * indicates the last image. Bit 0-6 are reserved, bit 7 is last image bit.
 */
#define LAST_IMAGE_BIT_MASK	0x80

/* PMU lookup table entry types. Used to locate PMU table entries
 * in the Fwsec image, corresponding to falcon ucodes.
 */
#define FALCON_UCODE_ENTRY_APPID_FIRMWARE_SEC_LIC	0x05
#define FALCON_UCODE_ENTRY_APPID_FWSEC_DBG		0x45
#define FALCON_UCODE_ENTRY_APPID_FWSEC_PROD		0x85

/* BIT Token ID for Falcon data */
#define BIT_TOKEN_ID_FALCON_DATA	0x70

/**
 * struct pcir_struct - PCI Data Structure as defined in PCI Firmware Specification
 */
struct pcir_struct {
	u8 signature[4];		/* PCI Data Structure signature ("PCIR" or "NPDS") */
	u16 vendor_id;			/* PCI Vendor ID (e.g., 0x10DE for NVIDIA) */
	u16 device_id;			/* PCI Device ID */
	u16 device_list_ptr;		/* Device List Pointer */
	u16 pci_data_struct_len;	/* PCI Data Structure Length */
	u8 pci_data_struct_rev;		/* PCI Data Structure Revision */
	u8 class_code[3];		/* Class code (3 bytes, 0x03 for display controller) */
	u16 image_len;			/* Size of this image in 512-byte blocks */
	u16 vendor_rom_rev;		/* Revision Level of the Vendor's ROM */
	u8 code_type;			/* ROM image type (0x00 = PC-AT, 0x03 = EFI, 0x70 = NBSI) */
	u8 last_image;			/* Last image indicator (0x00 = Not last, 0x80 = Last) */
	u16 max_runtime_image_len;	/* Maximum Run-time Image Length (units of 512 bytes) */
} __packed;

/**
 * struct bit_header - BIOS Information Table (BIT) Header
 */
struct bit_header {
	u16 id;			/* 0h: BIT Header Identifier (BMP=0x7FFF/BIT=0xB8FF) */
	u8 signature[4];	/* 2h: BIT Header Signature ("BIT\0") */
	u16 bcd_version;	/* 6h: Binary Coded Decimal Version, ex: 0x0100 is 1.00. */
	u8 header_size;		/* 8h: Size of BIT Header (in bytes) */
	u8 token_size;		/* 9h: Size of BIT Tokens (in bytes) */
	u8 token_entries;	/* 10h: Number of token entries that follow */
	u8 checksum;		/* 11h: BIT Header Checksum */
} __packed;

/**
 * struct bit_token - BIT Token Entry
 */
struct bit_token {
	u8 id;			/* 00h: Token identifier */
	u8 data_version;	/* 01h: Version of the token data */
	u16 data_size;		/* 02h: Size of token data in bytes */
	u16 data_offset;	/* 04h: Offset to the token data */
} __packed;

/**
 * struct pci_rom_header - PCI ROM Expansion Header
 */
struct pci_rom_header {
	u16 signature;			/* 00h: Signature (0xAA55) */
	u8 reserved[20];		/* 02h: Reserved bytes for processor architecture */
	u16 nbsi_data_offset;		/* 16h: NBSI Data Offset (NBSI-specific) */
	u16 pci_data_struct_offset;	/* 18h: Pointer to PCI Data Structure */
	u32 size_of_block;		/* 1Ah: Size of block (NBSI-specific) */
} __packed;

/**
 * struct npde_struct - NVIDIA PCI Data Extension Structure
 */
struct npde_struct {
	u8 signature[4];	/* 00h: Signature ("NPDE") */
	u16 npci_data_ext_rev;	/* 04h: NVIDIA PCI Data Extension Revision */
	u16 npci_data_ext_len;	/* 06h: NVIDIA PCI Data Extension Length */
	u16 subimage_len;	/* 08h: Sub-image Length (in 512-byte units) */
	u8 last_image;		/* 0Ah: Last image indicator flag */
} __packed;

/**
 * struct pmu_lookup_table_entry - PMU lookup table entry
 */
struct pmu_lookup_table_entry {
	u8 application_id;
	u8 target_id;
	u32 data;
} __packed;

/**
 * struct pmu_lookup_table - PMU lookup table
 */
struct pmu_lookup_table {
	u8 version;
	u8 header_len;
	u8 entry_len;
	u8 entry_count;
	void *table_data;
};

/**
 * struct bios_image_base - Base BIOS image structure
 */
struct bios_image_base {
	struct pci_rom_header rom_header;
	struct pcir_struct pcir;
	struct npde_struct *npde;	/* Optional */
	void *data;
	size_t data_len;
};

/**
 * struct fwsec_bios_image - FWSEC BIOS image
 */
struct fwsec_bios_image {
	struct bios_image_base base;
	size_t falcon_ucode_offset;
};

/**
 * struct vbios - Main VBIOS structure
 */
struct vbios {
	struct fwsec_bios_image fwsec_image;
};

/**
 * pcir_struct_new - Create PCI Data Structure from data
 * @pdev: PCI device
 * @data: Raw data
 * @pcir: Output structure
 */
static int pcir_struct_new(struct pci_dev *pdev, const u8 *data,
			   struct pcir_struct *pcir)
{
	if (!data || !pcir)
		return -EINVAL;

	memcpy(pcir, data, sizeof(*pcir));

	/* Validate signature */
	if (memcmp(pcir->signature, "PCIR", 4) != 0 &&
	    memcmp(pcir->signature, "NPDS", 4) != 0) {
		dev_err(&pdev->dev, "Invalid PCIR signature\n");
		return -EINVAL;
	}

	if (pcir->image_len == 0) {
		dev_err(&pdev->dev, "Invalid image length: 0\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * pcir_is_last - Check if this is the last image
 * @pcir: PCI Data Structure
 */
static bool pcir_is_last(const struct pcir_struct *pcir)
{
	return (pcir->last_image & LAST_IMAGE_BIT_MASK) != 0;
}

/**
 * pcir_image_size_bytes - Get image size in bytes
 * @pcir: PCI Data Structure
 */
static size_t pcir_image_size_bytes(const struct pcir_struct *pcir)
{
	return pcir->image_len * 512;
}

/**
 * bit_header_new - Create BIT header from data
 * @data: Raw data
 * @header: Output header
 */
static int bit_header_new(const u8 *data, struct bit_header *header)
{
	if (!data || !header)
		return -EINVAL;

	memcpy(header, data, sizeof(*header));

	/* Check header ID and signature */
	if (header->id != 0xB8FF || memcmp(header->signature, "BIT\0", 4) != 0)
		return -EINVAL;

	return 0;
}

/**
 * find_bit_header - Find BIT header in data
 * @data: Data to search in
 * @len: Length of data
 * @offset: Output offset where BIT header was found
 */
static int find_bit_header(const u8 *data, size_t len, size_t *offset)
{
	const u8 bit_pattern[] = {0xff, 0xb8, 'B', 'I', 'T', 0x00};
	size_t i;

	if (!data || !offset)
		return -EINVAL;

	for (i = 0; i <= len - sizeof(bit_pattern); i++) {
		if (memcmp(&data[i], bit_pattern, sizeof(bit_pattern)) == 0) {
			*offset = i;
			return 0;
		}
	}

	return -ENOENT;
}

/**
 * vbios_read_bar0 - Read data from BAR0
 * @bar0: BAR0 mapping
 * @offset: Offset to read from
 * @len: Length to read
 * @buffer: Output buffer
 */
static int vbios_read_bar0(void __iomem *bar0, size_t offset, size_t len, u8 *buffer)
{
	size_t i;

	if (!bar0 || !buffer)
		return -EINVAL;

	/* Ensure length is a multiple of 4 for 32-bit reads */
	if (len % 4 != 0)
		return -EINVAL;

	/* Read 32-bit words and convert to bytes */
	for (i = 0; i < len; i += 4) {
		u32 word = ioread32(bar0 + offset + i);
		buffer[i] = word & 0xff;
		buffer[i + 1] = (word >> 8) & 0xff;
		buffer[i + 2] = (word >> 16) & 0xff;
		buffer[i + 3] = (word >> 24) & 0xff;
	}

	return 0;
}

/**
 * vbios_new - Create new VBIOS structure
 * @pdev: PCI device
 * @bar0: BAR0 mapping
 */
struct vbios *vbios_new(struct pci_dev *pdev, void __iomem *bar0)
{
	struct vbios *vbios;
	u8 *rom_data;
	size_t current_offset = 0;
	bool last_found = false;
	int ret;

	if (!pdev || !bar0)
		return ERR_PTR(-EINVAL);

	vbios = kzalloc(sizeof(*vbios), GFP_KERNEL);
	if (!vbios)
		return ERR_PTR(-ENOMEM);

	/* Allocate buffer for ROM data */
	rom_data = kmalloc(BIOS_MAX_SCAN_LEN, GFP_KERNEL);
	if (!rom_data) {
		kfree(vbios);
		return ERR_PTR(-ENOMEM);
	}

	/* Simplified VBIOS parsing - read initial data */
	ret = vbios_read_bar0(bar0, ROM_OFFSET, BIOS_READ_AHEAD_SIZE, rom_data);
	if (ret) {
		dev_err(&pdev->dev, "Failed to read VBIOS ROM data: %d\n", ret);
		goto err_free;
	}

	/* For now, just initialize the basic structure */
	/* A full implementation would:
	 * 1. Parse all BIOS images in the ROM
	 * 2. Find PCI-AT, EFI, NBSI, and FWSEC images
	 * 3. Extract falcon data pointers
	 * 4. Setup PMU lookup tables
	 * 5. Locate falcon ucodes
	 */

	dev_info(&pdev->dev, "VBIOS parsing completed (simplified)\n");

	kfree(rom_data);
	return vbios;

err_free:
	kfree(rom_data);
	kfree(vbios);
	return ERR_PTR(ret);
}

/**
 * vbios_free - Free VBIOS structure
 * @vbios: VBIOS structure to free
 */
void vbios_free(struct vbios *vbios)
{
	if (vbios) {
		/* Free any allocated data in fwsec_image */
		if (vbios->fwsec_image.base.data)
			kfree(vbios->fwsec_image.base.data);
		if (vbios->fwsec_image.base.npde)
			kfree(vbios->fwsec_image.base.npde);
		kfree(vbios);
	}
}

/**
 * vbios_get_fwsec_image - Get FWSEC image from VBIOS
 * @vbios: VBIOS structure
 */
struct fwsec_bios_image *vbios_get_fwsec_image(struct vbios *vbios)
{
	if (!vbios)
		return NULL;
	
	return &vbios->fwsec_image;
}