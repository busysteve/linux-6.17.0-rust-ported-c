// SPDX-License-Identifier: GPL-2.0
// Copyright (C) Tehuti Networks Ltd.
// Copyright (C) 2024 FUJITA Tomonori <fujita.tomonori@gmail.com>

/*
 * Applied Micro Circuits Corporation QT2025 PHY driver
 *
 * This driver is based on the vendor driver `QT2025_phy.c`. This source
 * and firmware can be downloaded on the EN-9320SFP+ support site.
 *
 * The QT2025 PHY integrates an Intel 8051 micro-controller.
 *
 * Original Rust version: drivers/net/phy/qt2025.rs
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/mdio.h>
#include <linux/firmware.h>
#include <linux/sizes.h>

#define QT2025_PHY_ID		0x0043a400
#define QT2025_PHY_ID_MASK	0xffffffff

#define QT2025_FIRMWARE_NAME	"qt2025-2.0.3.3.fw"

/* Register definitions */
#define QT2025_MICRO_RESETN	0xc300
#define QT2025_SREFCLK_FREQ	0xc302
#define QT2025_LOOPBACK_MODE	0xc319
#define QT2025_LAN_WAN_CONFIG	0xc31a
#define QT2025_MICRO_START_BOOT	0xe854
#define QT2025_MICRO_START_SRAM	0xe854

static int qt2025_probe(struct phy_device *phydev)
{
	const struct firmware *fw;
	int ret, hw_rev;
	size_t src_idx, dst_offset;
	u8 dst_mmd;
	const u8 *fw_data;
	size_t fw_len;

	/* Check the hardware revision code.
	 * Only 0xb3 works with this driver and firmware.
	 */
	hw_rev = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, 0xd001);
	if (hw_rev < 0)
		return hw_rev;

	if ((hw_rev >> 8) != 0xb3)
		return -ENODEV;

	/* MICRO_RESETN: hold the micro-controller in reset while configuring. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QT2025_MICRO_RESETN, 0x0000);
	if (ret)
		return ret;

	/* SREFCLK_FREQ: configure clock frequency of the micro-controller. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QT2025_SREFCLK_FREQ, 0x0004);
	if (ret)
		return ret;

	/* Non loopback mode. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QT2025_LOOPBACK_MODE, 0x0038);
	if (ret)
		return ret;

	/* CUS_LAN_WAN_CONFIG: select between LAN and WAN (WIS) mode. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QT2025_LAN_WAN_CONFIG, 0x0098);
	if (ret)
		return ret;

	/* The following writes use standardized registers (3.38 through
	 * 3.41 5/10/25GBASE-R PCS test pattern seed B) for something else.
	 * We don't know what.
	 */
	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, 0x0026, 0x0e00);
	if (ret)
		return ret;

	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, 0x0027, 0x0893);
	if (ret)
		return ret;

	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, 0x0028, 0xa528);
	if (ret)
		return ret;

	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, 0x0029, 0x0003);
	if (ret)
		return ret;

	/* Configure transmit and recovered clock. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, 0xa30a, 0x06e1);
	if (ret)
		return ret;

	/* MICRO_RESETN: release the micro-controller from the reset state. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, QT2025_MICRO_RESETN, 0x0002);
	if (ret)
		return ret;

	/* The micro-controller will start running from the boot ROM. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, QT2025_MICRO_START_BOOT, 0x00c0);
	if (ret)
		return ret;

	ret = request_firmware(&fw, QT2025_FIRMWARE_NAME, &phydev->mdio.dev);
	if (ret) {
		phydev_err(phydev, "Failed to load firmware %s: %d\n",
			   QT2025_FIRMWARE_NAME, ret);
		return ret;
	}

	fw_data = fw->data;
	fw_len = fw->size;

	if (fw_len > SZ_16K + SZ_8K) {
		phydev_err(phydev, "Firmware too large: %zu bytes\n", fw_len);
		release_firmware(fw);
		return -EFBIG;
	}

	/* The 24kB of program memory space is accessible by MDIO.
	 * The first 16kB of memory is located in the address range 3.8000h - 3.BFFFh.
	 * The next 8kB of memory is located at 4.8000h - 4.9FFFh.
	 */
	dst_offset = 0;
	dst_mmd = MDIO_MMD_PCS;
	
	for (src_idx = 0; src_idx < fw_len; src_idx++) {
		if (src_idx == SZ_16K) {
			/* Start writing to the next register with no offset */
			dst_offset = 0;
			dst_mmd = MDIO_MMD_PHYXS;
		}

		ret = phy_write_mmd(phydev, dst_mmd, 0x8000 + dst_offset,
				    fw_data[src_idx]);
		if (ret) {
			release_firmware(fw);
			return ret;
		}

		dst_offset++;
	}

	release_firmware(fw);

	/* The micro-controller will start running from SRAM. */
	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, QT2025_MICRO_START_SRAM, 0x0040);
	if (ret)
		return ret;

	/* TODO: sleep here until the hw becomes ready. */
	return 0;
}

static int qt2025_read_status(struct phy_device *phydev)
{
	return genphy_c45_read_status(phydev);
}

static struct phy_driver qt2025_driver[] = {
	{
		.phy_id		= QT2025_PHY_ID,
		.phy_id_mask	= QT2025_PHY_ID_MASK,
		.name		= "QT2025 10Gpbs SFP+",
		.probe		= qt2025_probe,
		.read_status	= qt2025_read_status,
	}
};

module_phy_driver(qt2025_driver);

static struct mdio_device_id __maybe_unused qt2025_tbl[] = {
	{ QT2025_PHY_ID, QT2025_PHY_ID_MASK },
	{ }
};

MODULE_DEVICE_TABLE(mdio, qt2025_tbl);
MODULE_FIRMWARE(QT2025_FIRMWARE_NAME);
MODULE_DESCRIPTION("AMCC QT2025 PHY driver");
MODULE_AUTHOR("FUJITA Tomonori <fujita.tomonori@gmail.com>");
MODULE_LICENSE("GPL");