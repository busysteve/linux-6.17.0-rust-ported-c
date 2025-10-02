// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2023 FUJITA Tomonori <fujita.tomonori@gmail.com>

/*
 * C port of Rust Asix PHYs driver
 *
 * Original Rust version: drivers/net/phy/ax88796b_rust.rs
 */

#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/phy.h>
#include <linux/mdio.h>

#define ASIX_PHY_NAME_AX88772A "Asix Electronics AX88772A"
#define ASIX_PHY_NAME_AX88772C "Asix Electronics AX88772C"
#define ASIX_PHY_NAME_AX88796B "Asix Electronics AX88796B"

#define AX88772A_PHY_ID 0x003b1861
#define AX88772C_PHY_ID 0x003b1881
#define AX88796B_PHY_ID 0x003b1841

/* Performs a software PHY reset using the standard
 * BMCR_RESET bit and poll for the reset bit to be cleared.
 * Toggle BMCR_RESET bit off to accommodate broken AX8796B PHY implementation
 * such as used on the Individual Computers' X-Surf 100 Zorro card.
 */
static int asix_soft_reset(struct phy_device *phydev)
{
	int ret;

	ret = phy_write(phydev, MII_BMCR, 0);
	if (ret < 0)
		return ret;

	return genphy_soft_reset(phydev);
}

/* AX88772A is not working properly with some old switches (NETGEAR EN 108TP):
 * after autoneg is done and the link status is reported as active, the MII_LPA
 * register is 0. This issue is not reproducible on AX88772C.
 */
static int ax88772a_read_status(struct phy_device *phydev)
{
	int ret, bmcr;

	ret = genphy_update_link(phydev);
	if (ret)
		return ret;

	if (!phydev->link)
		return 0;

	/* If MII_LPA is 0, phy_resolve_aneg_linkmode() will fail to resolve
	 * linkmode so use MII_BMCR as default values.
	 */
	bmcr = phy_read(phydev, MII_BMCR);
	if (bmcr < 0)
		return bmcr;

	if (bmcr & BMCR_SPEED100)
		phydev->speed = SPEED_100;
	else
		phydev->speed = SPEED_10;

	if (bmcr & BMCR_FULLDPLX)
		phydev->duplex = DUPLEX_FULL;
	else
		phydev->duplex = DUPLEX_HALF;

	ret = genphy_read_lpa(phydev);
	if (ret < 0)
		return ret;

	if (phydev->autoneg == AUTONEG_ENABLE && phydev->autoneg_complete)
		phy_resolve_aneg_linkmode(phydev);

	return 0;
}

static void ax88772a_link_change_notify(struct phy_device *phydev)
{
	/* Reset PHY, otherwise MII_LPA will provide outdated information.
	 * This issue is reproducible only with some link partner PHYs.
	 */
	if (phydev->state == PHY_NOLINK) {
		phy_init_hw(phydev);
		phy_start_aneg(phydev);
	}
}

static struct phy_driver ax88796b_drivers[] = {
	{
		.phy_id		= AX88772A_PHY_ID,
		.phy_id_mask	= 0xffffffff,
		.name		= ASIX_PHY_NAME_AX88772A,
		.flags		= PHY_IS_INTERNAL,
		.read_status	= ax88772a_read_status,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.soft_reset	= asix_soft_reset,
		.link_change_notify = ax88772a_link_change_notify,
	}, {
		.phy_id		= AX88772C_PHY_ID,
		.phy_id_mask	= 0xffffffff,
		.name		= ASIX_PHY_NAME_AX88772C,
		.flags		= PHY_IS_INTERNAL,
		.suspend	= genphy_suspend,
		.resume		= genphy_resume,
		.soft_reset	= asix_soft_reset,
	}, {
		.phy_id		= AX88796B_PHY_ID,
		.phy_id_mask	= 0xfffffff0,
		.name		= ASIX_PHY_NAME_AX88796B,
		.soft_reset	= asix_soft_reset,
	}
};

module_phy_driver(ax88796b_drivers);

static struct mdio_device_id __maybe_unused ax88796b_tbl[] = {
	{ AX88772A_PHY_ID, 0xffffffff },
	{ AX88772C_PHY_ID, 0xffffffff },
	{ AX88796B_PHY_ID, 0xfffffff0 },
	{ }
};

MODULE_DEVICE_TABLE(mdio, ax88796b_tbl);
MODULE_DESCRIPTION("C port of Rust Asix PHYs driver");
MODULE_AUTHOR("FUJITA Tomonori <fujita.tomonori@gmail.com>");
MODULE_LICENSE("GPL");