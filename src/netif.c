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
#include <linux/can/skb.h>
#include <session.h>

#include "netif.h"


void* netif_tx (void* arg) {
    device_session_t* ds = (device_session_t*)arg;
    struct net_device* dev = ds->device;
    struct can_msg* canmsg;

    while (1) {
        if (ds->tx_queue.attr.size == 0) {
            log_trace("netif_tx exit: %s\n", dev->name);

            return NULL;
        }

        if ((canmsg = dequeue(&ds->tx_queue, 0)) == NULL) {
            log_trace("netif_tx exit: %s\n", dev->name);

            return NULL;
        }

        struct sk_buff *skb;
        struct can_frame *cf;

        /* create zero'ed CAN frame buffer */
        skb = alloc_can_skb(dev, &cf);

        if (skb == NULL) {
            log_err("netif_tx exit: alloc_can_skb error\n");

            return NULL;
        }

        skb->len = CAN_MTU;
        cf->can_id = canmsg->mid;
        cf->can_dlc = canmsg->len;

        if (canmsg->ext.is_extended_mid) {
            cf->can_id |= CAN_EFF_FLAG;
        }

        int i;
        for (i = 0; i < canmsg->len; ++i) {
            cf->data[i] = canmsg->dat[i];
        }

        dev->netdev_ops->ndo_start_xmit(skb, dev);
    }

    return 0;
}

/*
 * Test from Linux:
 *      cansend vcan0 1F334455#1122334455667788
 *      cansend vcan0 123#1122334455667788
 */
int netif_rx (struct sk_buff* skb) {
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

    canmsg.ext.timestamp = get_clock_time_us()/1000; // set TIMESTAMP
    canmsg.ext.is_remote_frame = (skb->is_echo ? 0 : 1);
    canmsg.len = msg->len; // set LEN

    int i;
    for (i = 0; i < CAN_MSG_DATA_MAX; ++i) {
        canmsg.dat[i] = msg->data[i]; // Set DAT
    }

    device_session_t* ds = device_sessions[skb->dev->dev_id];

    client_session_t** it = &ds->root_client_session;
    while (*it != NULL) {
        if (enqueue(&(*it)->rx_queue, &canmsg) != EOK) {
        }

        it = &(*it)->next;
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

void netif_wake_queue (struct net_device* dev) {
    log_trace("netif_wake_queue\n");

    device_session_t* ds = device_sessions[dev->dev_id];

    pthread_mutex_lock(&ds->mutex);
    ds->queue_stopped = 0;
    pthread_cond_signal(&ds->cond);
    pthread_mutex_unlock(&ds->mutex);
}

void netif_stop_queue (struct net_device* dev) {
    log_trace("netif_stop_queue\n");

    device_session_t* ds = device_sessions[dev->dev_id];

    if (ds->tx_queue.attr.size == 0) {
        return;
    }

    pthread_t self = pthread_self();
    if (self != ds->tx_thread) {
        return;
    }

    pthread_mutex_lock(&ds->mutex);
    while (ds->queue_stopped && ds->tx_queue.attr.size != 0) {
        pthread_cond_wait(&ds->cond, &ds->mutex);
    }

    ds->queue_stopped = 1;
    pthread_mutex_unlock(&ds->mutex);
}
