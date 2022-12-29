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
#include <linux/can/error.h>
#include <session.h>


void netif_wake_queue(struct net_device *dev)
{
    log_trace("netif_wake_queue\n");
}

/*
 * Test from Linux:
 *      cansend vcan0 1F334455#1122334455667788
 *      cansend vcan0 123#1122334455667788
 */
int netif_rx(struct sk_buff *skb)
{
    struct can_frame* msg = (struct can_frame*)skb->data;

    /* handle error message frame */
    if (msg->can_id & CAN_ERR_FLAG) {
        if (msg->can_id & CAN_ERR_TX_TIMEOUT) {
            log_warn("netif_rx: can%d: TX timeout (by netdevice driver)\n",
                    skb->dev->dev_id);
        }
        if (msg->can_id & CAN_ERR_LOSTARB) {
            log_warn("netif_rx: can%d: lost arbitration: %x\n",
                    skb->dev->dev_id, msg->data[0]);
        }
        if (msg->can_id & CAN_ERR_CRTL) {
            log_warn("netif_rx: can%d: controller problems: %x\n",
                    skb->dev->dev_id, msg->data[1]);
        }
        if (msg->can_id & CAN_ERR_PROT) {
            log_warn("netif_rx: can%d: protocol violations: %x, %x\n",
                    skb->dev->dev_id, msg->data[2], msg->data[3]);
        }
        if (msg->can_id & CAN_ERR_TRX) {
            log_warn("netif_rx: can%d: transceiver status: %x\n",
                    skb->dev->dev_id, msg->data[4]);
        }
        if (msg->can_id & CAN_ERR_ACK) {
            log_warn("netif_rx: can%d: received no ACK on transmission\n",
                    skb->dev->dev_id);
        }
        if (msg->can_id & CAN_ERR_BUSOFF) {
            log_warn("netif_rx: can%d: bus off\n", skb->dev->dev_id);
        }
        if (msg->can_id & CAN_ERR_BUSERROR) {
            log_warn("netif_rx: can%d: bus error (may flood!)\n",
                    skb->dev->dev_id);
        }
        if (msg->can_id & CAN_ERR_RESTARTED) {
            log_warn("netif_rx: can%d: controller restarted\n",
                    skb->dev->dev_id);
        }
        if (msg->can_id & CAN_ERR_CNT) {
            log_warn("netif_rx: can%d: TX error counter; tx:%x, rx:%x\n",
                    skb->dev->dev_id, msg->data[6], msg->data[7]);
        }

        return NET_RX_SUCCESS;
    }

    if (msg->can_id & CAN_RTR_FLAG) {
        log_trace("netif_rx; CAN_RTR_FLAG\n");

        return NET_RX_SUCCESS;
    }

    struct can_msg canmsg;

    /* set MID; omit EFF, RTR, ERR flags */
    canmsg.mid = (msg->can_id & CAN_ERR_MASK);

    if (msg->can_id & CAN_EFF_FLAG) {
        canmsg.ext.is_extended_mid = 1; // EFF
    }
    else {
        canmsg.ext.is_extended_mid = 0; // SFF
    }

    canmsg.ext.timestamp = 0x0; // set TIMESTAMP
    canmsg.len = msg->len; // set LEN

    int i;
    for (i = 0; i < CAN_MSG_DATA_MAX; ++i) {
        canmsg.dat[i] = msg->data[i]; // Set DAT
    }

    device_sessions_t* ds = &device_sessions[skb->dev->dev_id];

    for (i = 0; i < ds->num_sessions; ++i) {
        if (enqueue(&ds->sessions[i].rx, &canmsg) != EOK) {
        }
    }

    log_trace("netif_rx; can%d [%s] %X [%d] %2X %2X %2X %2X %2X %2X %2X %2X\n",
            skb->dev->dev_id,
            /* EFF/SFF is set in the MSB */
            (msg->can_id & CAN_EFF_FLAG) ? "EFF" : "SFF",
            msg->can_id & CAN_ERR_MASK, /* omit EFF, RTR, ERR flags */
            msg->len,
            msg->data[0],
            msg->data[1],
            msg->data[2],
            msg->data[3],
            msg->data[4],
            msg->data[5],
            msg->data[6],
            msg->data[7]);

    return NET_RX_SUCCESS;
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
