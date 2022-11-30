/*
* Copyright (C) 2016 Deniz Eren <deniz.eren@icloud.com>
*
* Parts of this software are based on (derived) the following:
*
* - Driver for CAN cards (PCIE-1680/PCI-1680/PCM-3680/PCL-841/MIC-3680)
*   Copyright (C) 2011, ADVANTECH Co, Ltd.
*
* - git://git.toiit.sgu.ru/people/samarkinpa/public/adv_pci.git
*   Copyright (C) 2011 Pavel Samarkin <pavel.samarkin@gmail.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the version 2 of the GNU General Public License
* as published by the Free Software Foundation
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdlib>
#include <hw/inout.h>
//#include <linux/kernel.h>
//#include <linux/module.h>
#include "linux/interrupt.h"
#include "linux/netdevice.h"
#include "linux/pci.h"
//#include <linux/io.h>

#include "sja1000.h"

#define DRV_NAME  "adv_pci"

//MODULE_AUTHOR("Deniz Eren (deniz.eren@icloud.com)");
//MODULE_DESCRIPTION("Socket-CAN driver for Advantech PCI cards");
//MODULE_SUPPORTED_DEVICE("Advantech PCI cards");
//MODULE_LICENSE("GPL v2");

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
#define ADV_PCI_DEVICE_ID01       0x1680
#define ADV_PCI_DEVICE_ID02       0x3680
#define ADV_PCI_DEVICE_ID03       0x2052
#define ADV_PCI_DEVICE_ID04       0x1681
#define ADV_PCI_DEVICE_ID05       0xc001
#define ADV_PCI_DEVICE_ID06       0xc002
#define ADV_PCI_DEVICE_ID07       0xc004
#define ADV_PCI_DEVICE_ID08       0xc101
#define ADV_PCI_DEVICE_ID09       0xc102
#define ADV_PCI_DEVICE_ID10       0xc104
#define ADV_PCI_DEVICE_ID11       0xc201
#define ADV_PCI_DEVICE_ID12       0xc202
#define ADV_PCI_DEVICE_ID13       0xc204
#define ADV_PCI_DEVICE_ID14       0xc301
#define ADV_PCI_DEVICE_ID15       0xc302
#define ADV_PCI_DEVICE_ID16       0xc304

static const struct pci_device_id adv_pci_tbl[] = {
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID01, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID02, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID03, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID04, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID05, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID06, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID07, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID08, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID09, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID10, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID11, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID12, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID13, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID14, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID15, PCI_ANY_ID, PCI_ANY_ID,},
	{ADV_PCI_VENDOR_ID, ADV_PCI_DEVICE_ID16, PCI_ANY_ID, PCI_ANY_ID,},
	{0,}
};

//MODULE_DEVICE_TABLE(pci, adv_pci_tbl);

static u8 adv_pci_read_reg(const struct sja1000_priv *priv, int port)
{
	struct adv_board_data *board_data = (adv_board_data*)priv->priv;

//	return ioread8(priv->reg_base + (port << board_data->reg_shift));
	return in8(priv->reg_base + (port << board_data->reg_shift));
}

static void adv_pci_write_reg(const struct sja1000_priv *priv,
				int port, u8 val)
{
	struct adv_board_data *board_data = (adv_board_data*)priv->priv;

//	iowrite8(val, priv->reg_base + (port << board_data->reg_shift));
	out8(priv->reg_base + (port << board_data->reg_shift), val);
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
	case ADV_PCI_DEVICE_ID01:
	case ADV_PCI_DEVICE_ID03:
		no_of_chips = 2;
		break;
	case ADV_PCI_DEVICE_ID04:
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
	case ADV_PCI_DEVICE_ID01:
	case ADV_PCI_DEVICE_ID03:
	case ADV_PCI_DEVICE_ID04:
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
	case ADV_PCI_DEVICE_ID11:
	case ADV_PCI_DEVICE_ID12:
	case ADV_PCI_DEVICE_ID13:
	case ADV_PCI_DEVICE_ID14:
	case ADV_PCI_DEVICE_ID15:
	case ADV_PCI_DEVICE_ID16:
		bar_offset = 0x400;
		break;
	case ADV_PCI_DEVICE_ID01:
	case ADV_PCI_DEVICE_ID03:
	case ADV_PCI_DEVICE_ID04:
		bar_offset = 0x0;
		break;
	default:
		break;
	}

	return bar_offset;
}

static int adv_pci_is_multi_bar(struct pci_dev *pdev)
{
	int is_multi_bar = 0;

	switch (pdev->device) {
	case ADV_PCI_DEVICE_ID01:
	case ADV_PCI_DEVICE_ID03:
	case ADV_PCI_DEVICE_ID04:
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
	case ADV_PCI_DEVICE_ID11:
	case ADV_PCI_DEVICE_ID12:
	case ADV_PCI_DEVICE_ID13:
	case ADV_PCI_DEVICE_ID14:
	case ADV_PCI_DEVICE_ID15:
	case ADV_PCI_DEVICE_ID16:
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

	board = (adv_pci*)pci_get_drvdata(pdev);
	if (!board)
		return;

	for (i = 0; i < board->no_channels; i++) {
		if (!board->slave_dev[i])
			continue;

		dev = board->slave_dev[i];
		if (!dev)
			continue;

//		dev_info(&board->pci_dev->dev, "Removing device %s\n",
//			dev->name);
		syslog(LOG_INFO, "Removing device %s\n", dev->name);

		priv = (sja1000_priv*)netdev_priv(dev);

		unregister_sja1000dev(dev);

		pci_iounmap(board->pci_dev, priv->reg_base);

		free_sja1000dev(dev);
	}
}

static int adv_pci_add_chan(struct pci_dev *pdev, int channel, int bar_no)
{
	struct net_device *dev;
	struct sja1000_priv *priv;
	struct adv_pci *board;
	struct adv_board_data *board_data;
	int err;
	int bar_offset, reg_shift;

	/* The following function calls assume device is supported */
	bar_offset = adv_pci_bar_offset(pdev);
	reg_shift = adv_pci_reg_shift(pdev);

	dev = alloc_sja1000dev(sizeof(struct adv_board_data));
	if (dev == NULL)
		return -ENOMEM;

	priv = (sja1000_priv*)netdev_priv(dev);
	board_data = (adv_board_data*)priv->priv;

	board_data->reg_shift = reg_shift;

	priv->reg_base = pci_iomap(pdev, bar_no, 128) + bar_offset * channel;

	priv->read_reg = adv_pci_read_reg;
	priv->write_reg = adv_pci_write_reg;

	priv->can.clock.freq = ADV_PCI_CAN_CLOCK;

	priv->ocr = ADV_PCI_OCR;
	priv->cdr = ADV_PCI_CDR;

	priv->irq_flags = IRQF_SHARED;
	dev->irq = pdev->irq;

	board = (adv_pci*)pci_get_drvdata(pdev);
	board->pci_dev = pdev;
	board->slave_dev[channel] = dev;

	adv_pci_reset(priv);

//	dev_info(&pdev->dev, "reg_base=%p irq=%d\n",
//		priv->reg_base, dev->irq);
	syslog(LOG_INFO, "reg_base=%p irq=%d\n", priv->reg_base, dev->irq);

//	SET_NETDEV_DEV(dev, &pdev->dev);
	dev->dev_id = channel;

	/* Register SJA1000 device */
	err = register_sja1000dev(dev);
	if (err) {
//		dev_err(&pdev->dev, "Registering device failed (err=%d)\n", err);
		syslog(LOG_ERR, "Registering device failed (err=%d)\n", err);

		goto failure;
	}

	return 0;

failure:
	adv_pci_del_all_channels(pdev);
	return err;
}

static void adv_pci_remove_one(struct pci_dev *pdev)
{
	struct adv_pci *board = (adv_pci*)pci_get_drvdata(pdev);

//	dev_info(&pdev->dev, "Removing card");
	syslog(LOG_INFO, "Removing card");

	adv_pci_del_all_channels(pdev);

//	kfree(board);
	std::free(board);

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

//	dev_info(&pdev->dev, "initializing device %04x:%04x\n",
//		pdev->vendor, pdev->device);
	syslog(LOG_INFO, "initializing device %04x:%04x\n", pdev->vendor, pdev->device);

//	board = kzalloc(sizeof(struct adv_pci), GFP_KERNEL);
	board = (adv_pci*)std::malloc(sizeof(struct adv_pci));
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

struct pci_driver adv_pci_driver = {
	.name = DRV_NAME,
	.id_table = adv_pci_tbl,
	.probe = adv_pci_init_one,
	.remove = adv_pci_remove_one
};

//module_pci_driver(adv_pci_driver);
