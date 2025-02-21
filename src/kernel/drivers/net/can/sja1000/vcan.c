/*
 * Copyright (C) 2025 Deniz Eren <deniz.eren@outlook.com>
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

#include "sja1000.h"

#define DRV_NAME  "vcan"

MODULE_AUTHOR("Deniz Eren (deniz.eren@outlook.com)");
MODULE_DESCRIPTION("Virtual CAN-bus module");
MODULE_LICENSE("GPL v2");

#define VCAN_PCI_CAN_CLOCK  (16000000 / 2)

struct vcan_pci {
    struct pci_dev *pci_dev;
    struct net_device *slave_dev[MAX_NO_OF_VCAN_CHANNELS];
    int no_channels;
    uint8_t *reg_base;
};

static const struct pci_device_id vcan_pci_tbl[] = {
    {0,}
};

MODULE_DEVICE_TABLE(pci, vcan_pci_tbl);

// SJA1000 simuation note:
// In netif_tx() (src/netif.c), for the vcan case, we just broadcast to all
// client sessions to avoid deeper hardware layers of the driver. However, if
// in the future an SJA1000 simulator is desired, then these
// vcan_read_reg()/vcan_write_reg() functions can be utilized and the netif_tx()
// bypass can be removed.
// Such an implementation should be considered as an additional option rather
// than modifying this existing vcan implementation.

static u8 vcan_read_reg(const struct sja1000_priv *priv, int port)
{
    return ((uint8_t *)priv->reg_base)[port];
}

static void vcan_write_reg(const struct sja1000_priv *priv,
                int port, u8 val)
{
    ((uint8_t *)priv->reg_base)[port] = val;
}

static void vcan_del_all_channels(struct pci_dev *pdev)
{
    struct net_device *dev;
    struct sja1000_priv *priv;
    struct vcan_pci *board;
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

        priv->reg_base = NULL;

        free_sja1000dev(dev);
        board->slave_dev[i] = NULL;
    }

    kfree(board->reg_base);
}

static int vcan_add_chan(struct pci_dev *pdev, int channel)
{
    struct net_device *dev;
    struct sja1000_priv *priv;
    struct vcan_pci *board;
    int err;

    dev = alloc_sja1000dev(0);
    if (dev == NULL)
        return -ENOMEM;

    priv = netdev_priv(dev);

    priv->read_reg = vcan_read_reg;
    priv->write_reg = vcan_write_reg;

    priv->irq_flags = 0;
    priv->flags = SJA1000_CUSTOM_IRQ_HANDLER;

    priv->can.clock.freq = VCAN_PCI_CAN_CLOCK;

    dev->irq = pdev->irq;

    board = pci_get_drvdata(pdev);
    board->pci_dev = pdev;
    board->slave_dev[channel] = dev;

    priv->reg_base = &board->reg_base[channel*0x100];

    dev_info(&pdev->dev, "reg_base=%p irq=%d\n",
            priv->reg_base, dev->irq);

    SET_NETDEV_DEV(dev, &pdev->dev);
    dev->dev_id = channel;

    /* Register SJA1000 device */
    err = register_sja1000dev(dev);
    if (err) {
        dev_err(&pdev->dev, "Registering device failed (err=%d)\n",
            err);
        goto failure;
    }

    return 0;

failure:
    vcan_del_all_channels(pdev);
    return err;
}

static void vcan_remove_one(struct pci_dev *pdev)
{
    struct vcan_pci *board = pci_get_drvdata(pdev);

    dev_info(&pdev->dev, "Removing card\n");

    vcan_del_all_channels(pdev);

    kfree(board);

    pci_set_drvdata(pdev, NULL);
}

static int vcan_init_one(struct pci_dev *pdev,
                const struct pci_device_id *ent)
{
    struct vcan_pci *board;

    int i, err;

    err = 0;

    dev_info(&pdev->dev, "initializing vcan\n");

    board = kzalloc(sizeof(struct vcan_pci), GFP_KERNEL);
    if (board == NULL)
        goto failure;

    board->reg_base = kzalloc(10*MAX_NO_OF_VCAN_CHANNELS*0x100, GFP_KERNEL);
    board->no_channels = optL_num;

    pci_set_drvdata(pdev, board);

    for (i = 0; i < board->no_channels; i++) {
        err = vcan_add_chan(pdev, i);
        if (err)
            goto failure_cleanup;
    }
    return 0;

failure_cleanup:
    vcan_remove_one(pdev);

failure:
    return err;
}

/*static */struct pci_driver vcan_driver = {
    .name = DRV_NAME,
    .id_table = vcan_pci_tbl,
    .probe = vcan_init_one,
    .remove = vcan_remove_one
};

module_pci_driver(vcan_driver);
