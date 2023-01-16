/*
 * Copyright (C) 2016 Deniz Eren <deniz.eren@outlook.com>
 * Copyright (C) 2022 Harri Järvi <harri.jarvi@atostek.com>
 *
 * Parts of this software are based on (derived) the following:
 *
 * - Driver for CAN cards (PCIE-1680/PCI-1680/PCM-3680/PCL-841/MIC-3680)
 *   Copyright (C) 2011, ADVANTECH Co, Ltd.
 *
 * - Works of:
 *   Copyright (C) 2011 Pavel Samarkin <pavel.samarkin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <asm/io.h>

#include "sja1000.h"

#define DRV_NAME  "adv_pci"

MODULE_AUTHOR("Deniz Eren (deniz.eren@icloud.com)");
MODULE_DESCRIPTION("Socket-CAN driver for Advantech PCI cards");
MODULE_LICENSE("GPL v2");

#define MAX_NO_OF_CHANNELS        4 /* max no of channels on a single card */

struct adv_pci {
	struct pci_dev *pci_dev;
	struct net_device *slave_dev[MAX_NO_OF_CHANNELS];
	int no_channels;
};

struct adv_board_data {
	int reg_shift;
};

#define ADV_PCI_CAN_CLOCK      (16000000 / 2)

/*
* Depends on the board configuration
*/
#define ADV_PCI_OCR            (OCR_TX0_PULLDOWN | OCR_TX0_PULLUP)

/*
* In the CDR register, you should set CBP to 1.
*/
#define ADV_PCI_CDR            (CDR_CBP | CDR_CLKOUT_MASK)

/*
* The PCI device and vendor IDs
*/
#define ADV_PCI_VENDOR_ID         0x13fe

static const struct pci_device_id adv_pci_tbl[] = {
	{ADV_PCI_VENDOR_ID, 0x1680, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0x3680, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0x2052, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0x1681, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc001, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc002, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc004, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc101, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc102, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc104, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc201, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc202, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc204, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc301, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc302, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0xc304, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0x00c5, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, 0x00d7, PCI_ANY_ID, PCI_ANY_ID,},
	{0,}
};

MODULE_DEVICE_TABLE(pci, adv_pci_tbl);

static u8 adv_pci_read_reg(const struct sja1000_priv *priv, int port)
{
	struct adv_board_data *board_data = priv->priv;

	return ioread8(priv->reg_base + (port << board_data->reg_shift));
}

static void adv_pci_write_reg(const struct sja1000_priv *priv,
				int port, u8 val)
{
	struct adv_board_data *board_data = priv->priv;

	iowrite8(val, priv->reg_base + (port << board_data->reg_shift));
}

static int adv_pci_device_support_check(const struct pci_dev *pdev)
{
	int err;
	const struct pci_device_id *ids;

	ids = adv_pci_tbl;
	err = -ENOMEM;

	if (ids) {
		while (ids->vendor || ids->subvendor || ids->class_mask) {
			if (ids->device == pdev->device) {
				err = 0;
				break;
			}
			ids++;
		}
	}

	return err;
}

static int number_of_sja1000_chips(const struct pci_dev *pdev)
{
	int no_of_chips = 0;

	switch (pdev->device) {
	case 0x1680:
	case 0x2052:
	case 0x00c5:
	case 0x00d7:
		no_of_chips = 2;
		break;
	case 0x1681:
		no_of_chips = 1;
		break;
	default:
		no_of_chips = pdev->device & 0x7;
		break;
	}

	return no_of_chips;
}

static int adv_pci_bar_no(const struct pci_dev *pdev)
{
	int bar_no = 0;

	switch (pdev->device) {
	case 0x1680:
	case 0x2052:
	case 0x1681:
		bar_no = 2;
		break;
	default:
		break;
	}

	return bar_no;
}

static int adv_pci_bar_offset(const struct pci_dev *pdev)
{
	int bar_offset = 0x100;

	switch (pdev->device) {
	case 0xc201:
	case 0xc202:
	case 0xc204:
	case 0xc301:
	case 0xc302:
	case 0xc304:
	case 0x00c5:
	case 0x00d7:
		bar_offset = 0x400;
		break;
	case 0x1680:
	case 0x2052:
	case 0x1681:
		bar_offset = 0x0;
		break;
	default:
		break;
	}

	return bar_offset;
}

static int adv_pci_bar_len(const struct pci_dev *pdev)
{
	int len = 0x100;

	switch (pdev->device) {
    case 0xc001:
    case 0xc002:
    case 0xc004:
    case 0xc101:
    case 0xc102:
    case 0xc104:
        len = 0x100;
        break;

	case 0xc201:
	case 0xc202:
	case 0xc204:
	case 0xc301:
	case 0xc302:
	case 0xc304:
	case 0x00c5:
	case 0x00d7:
		len = 0x400;
		break;

	case 0x1680:
	case 0x1681:
	case 0x2052:
    case 0x3680:
		len = 0x80;
		break;

	default:
		break;
	}

	return len;
}

static int adv_pci_is_multi_bar(struct pci_dev *pdev)
{
	int is_multi_bar = 0;

	switch (pdev->device) {
	case 0x1680:
	case 0x2052:
	case 0x1681:
		is_multi_bar = 1;
		break;
	default:
		break;
	}

	return is_multi_bar;
}

static int adv_pci_reg_shift(struct pci_dev *pdev)
{
	int reg_shift = 0;

	switch (pdev->device) {
	case 0xc201:
	case 0xc202:
	case 0xc204:
	case 0xc301:
	case 0xc302:
	case 0xc304:
	case 0x00c5:
	case 0x00d7:
		// These devices support memory mapped IO.
		// (not implemented by the driver)
		reg_shift = 2;
		break;
	default:
		break;
	}

	return reg_shift;
}

static int adv_pci_reset(const struct sja1000_priv *priv)
{
	unsigned char res;

	/* Make sure SJA1000 is in reset mode */
	priv->write_reg(priv, SJA1000_MOD, 1);

	/* Set PeliCAN mode */
	priv->write_reg(priv, SJA1000_CDR, CDR_PELICAN);

	/* check if mode is set */
	res = priv->read_reg(priv, SJA1000_CDR);

	if (res != CDR_PELICAN)
		return -EIO;

	return 0;
}

static void adv_pci_del_all_channels(struct pci_dev *pdev)
{
	struct net_device *dev;
	struct sja1000_priv *priv;
	struct adv_pci *board;
	int i;

	board = pci_get_drvdata(pdev);
	if (!board)
		return;

	for (i = 0; i < board->no_channels; i++) {
		if (!board->slave_dev[i])
			continue;

		dev = board->slave_dev[i];
		if (!dev)
			continue;

		dev_info(&board->pci_dev->dev, "Removing device %s\n",
			dev->name);

		priv = netdev_priv(dev);

		unregister_sja1000dev(dev);

		pci_iounmap(board->pci_dev, priv->reg_base);

		free_sja1000dev(dev);
		board->slave_dev[i] = NULL;
	}
}

static int adv_pci_add_chan(struct pci_dev *pdev, int channel, int bar_no)
{
	struct net_device *dev;
	struct sja1000_priv *priv;
	struct adv_pci *board;
	struct adv_board_data *board_data;
	int err;
	int bar_offset, bar_len, reg_shift;
    uintptr_t base_addr;

	/* The following function calls assume device is supported */
	bar_offset = adv_pci_bar_offset(pdev);
    bar_len = adv_pci_bar_len(pdev);
	reg_shift = adv_pci_reg_shift(pdev);

	dev = alloc_sja1000dev(sizeof(struct adv_board_data));
	if (dev == NULL)
		return -ENOMEM;

	priv = netdev_priv(dev);
	board_data = priv->priv;

	board_data->reg_shift = reg_shift;

    base_addr = pci_resource_start(pdev, bar_no) + bar_offset * channel;
    priv->reg_base = ioremap(base_addr, bar_len);

	priv->read_reg = adv_pci_read_reg;
	priv->write_reg = adv_pci_write_reg;

	priv->can.clock.freq = ADV_PCI_CAN_CLOCK;

	priv->ocr = ADV_PCI_OCR;
	priv->cdr = ADV_PCI_CDR;

	priv->irq_flags = IRQF_SHARED;
	dev->irq = pdev->irq;

	board = pci_get_drvdata(pdev);
	board->pci_dev = pdev;
	board->slave_dev[channel] = dev;

	dev_info(&pdev->dev, "reg_base=%p irq=%d\n",
            priv->reg_base, dev->irq);

	if (adv_pci_reset(priv) == 0) {
	    SET_NETDEV_DEV(dev, &pdev->dev);
	    dev->dev_id = channel;

	    /* Register SJA1000 device */
	    err = register_sja1000dev(dev);
	    if (err) {
		    dev_err(&pdev->dev, "Registering device failed (err=%d)\n",
			    err);
		    goto failure;
	    }
    }
    else {
        log_err("adv_pci_reset failed\n");

        free_sja1000dev(dev);
    }

	return 0;

failure:
	adv_pci_del_all_channels(pdev);
	return err;
}

static void adv_pci_remove_one(struct pci_dev *pdev)
{
	struct adv_pci *board = pci_get_drvdata(pdev);

	dev_info(&pdev->dev, "Removing card\n");

	adv_pci_del_all_channels(pdev);

	kfree(board);

	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
}

static int adv_pci_init_one(struct pci_dev *pdev,
				const struct pci_device_id *ent)
{
	struct adv_pci *board;

	int i, err;
	int no_channels, bar_no, is_multi_bar;

	err = 0;

	dev_info(&pdev->dev, "initializing device %04x:%04x\n",
		pdev->vendor, pdev->device);

	board = kzalloc(sizeof(struct adv_pci), GFP_KERNEL);
	if (board == NULL)
		goto failure;

	err = pci_enable_device(pdev);
	if (err)
		goto failure;

	err = adv_pci_device_support_check(pdev);
	if (err)
		goto failure_release_pci;

	no_channels = number_of_sja1000_chips(pdev);
	if (no_channels == 0) {
		err = -ENOMEM;
		goto failure_release_pci;
	}

	/* The following function calls assume device is supported */
	bar_no = adv_pci_bar_no(pdev);
	is_multi_bar = adv_pci_is_multi_bar(pdev);

	board->no_channels = no_channels;

	pci_set_drvdata(pdev, board);

	for (i = 0; i < no_channels; i++) {
		err = adv_pci_add_chan(pdev, i, bar_no);
		if (err)
			goto failure_cleanup;

		if (is_multi_bar)
			bar_no++;
	}
	return 0;

failure_cleanup:
	adv_pci_remove_one(pdev);

failure_release_pci:
	pci_disable_device(pdev);

failure:
	return err;
}

/*static */struct pci_driver adv_pci_driver = {
	.name = DRV_NAME,
	.id_table = adv_pci_tbl,
	.probe = adv_pci_init_one,
	.remove = adv_pci_remove_one
};

module_pci_driver(adv_pci_driver);
