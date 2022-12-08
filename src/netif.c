/*
 * \file    netif.c
 *
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
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

#include <pci/pci.h>

#include <linux/netdevice.h>
#include <linux/can.h>


void netif_wake_queue(struct net_device *dev)
{
    log_trace("netif_wake_queue\n");
}

int netif_rx(struct sk_buff *skb)
{
    struct can_frame* msg = (struct can_frame*)skb->data;

    log_trace("netif_rx; can%d: %x#%2x%2x%2x%2x%2x%2x%2x%2x\n",
            skb->dev->dev_id,
            msg->can_id,
            msg->data[0],
            msg->data[1],
            msg->data[2],
            msg->data[3],
            msg->data[4],
            msg->data[5],
            msg->data[6],
            msg->data[7]);

    kfree_skb(skb);
    return 0;
}

void netif_start_queue(struct net_device *dev)
{
    log_trace("netif_start_queue\n");
}

bool netif_carrier_ok(const struct net_device *dev) {
    return true;
}

void netif_carrier_on(struct net_device *dev)
{
    log_trace("netif_carrier_on\n");
    // Not called from SJA1000 driver
}

void netif_carrier_off(struct net_device *dev)
{
    log_trace("netif_carrier_off\n");
}

int netif_queue_stopped(const struct net_device *dev)
{
    log_trace("netif_queue_stopped\n");

    return 0;
}

void netif_stop_queue(struct net_device *dev)
{
    log_trace("netif_stop_queue\n");
}
