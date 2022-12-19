/*
 * Copyright (C) 2016 Deniz Eren <deniz.eren@outlook.com>
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
#define ADV_PCI_OCR            (OCR_TX0_PUSHPULL | OCR_TX1_PUSHPULL | OCR_TX1_INVERT)

/*
* In the CDR register, you should set CBP to 1.
*/
#define ADV_PCI_CDR            (CDR_CBP)

/*
* The PCI device and vendor IDs
*/
#define ADV_PCI_VENDOR_ID         0x13fe

/* 2-port CAN UniversalPCI Communication Card with Isolation */
#define ADV_PCI_DEVICE_ID1    	0xc302

static const struct pci_device_id adv_pci_tbl[] = {
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID1, PCI_ANY_ID, PCI_ANY_ID,},
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
	return pdev->device & 0x7;
}

static int adv_pci_reg_shift(struct pci_dev *pdev)
{
	int reg_shift = 0;

	switch (pdev->device) {
	case ADV_PCI_DEVICE_ID1:
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

static int adv_pci_add_chan(struct pci_dev *pdev, int bar_no)
{
	struct net_device *dev;
	struct sja1000_priv *priv;
	struct adv_pci *board;
	struct adv_board_data *board_data;
	int err;
	int reg_shift;

	/* The following function calls assume device is supported */
	reg_shift = adv_pci_reg_shift(pdev);

	dev = alloc_sja1000dev(sizeof(struct adv_board_data));
	if (dev == NULL)
		return -ENOMEM;

	priv = netdev_priv(dev);
	board_data = priv->priv;

	board_data->reg_shift = reg_shift;

	priv->reg_base = pci_iomap(pdev, bar_no, 128);

	priv->read_reg = adv_pci_read_reg;
	priv->write_reg = adv_pci_write_reg;

	priv->can.clock.freq = ADV_PCI_CAN_CLOCK;

	priv->ocr = ADV_PCI_OCR;
	priv->cdr = ADV_PCI_CDR;

	priv->irq_flags = IRQF_SHARED;
	dev->irq = pdev->irq;

	board = pci_get_drvdata(pdev);
	board->pci_dev = pdev;
	board->slave_dev[bar_no] = dev;
    dev->dev_id = bar_no;

	adv_pci_reset(priv);

	dev_info(&pdev->dev, "reg_base=%lu irq=%d\n",
		priv->reg_base, dev->irq);

	SET_NETDEV_DEV(dev, &pdev->dev);

	/* Register SJA1000 device */
	err = register_sja1000dev(dev);
	if (err) {
		dev_err(&pdev->dev, "Registering device failed (err=%d)\n",
			err);
		goto failure;
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
	int no_channels;

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
	board->no_channels = no_channels;

	pci_set_drvdata(pdev, board);

	for (i = 0; i < no_channels; i++) {
		err = adv_pci_add_chan(pdev, i);
		if (err)
			goto failure_cleanup;
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
