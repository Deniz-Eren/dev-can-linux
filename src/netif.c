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

#include <linux/units.h>
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

        if (canmsg->ext.is_extended_mid) { // Extended MID
            cf->can_id |= CAN_EFF_FLAG;
        }
        else { // Standard MID
            /**
             * Message IDs or MIDs are slightly different on QNX compared to
             * Linux. The form of the ID depends on whether or not the driver
             * is using extended MIDs:
             *
             *      - In standard 11-bit MIDs, bits 18–28 define the MID.
             *      - In extended 29-bit MIDs, bits 0–28 define the MID.
             */

            cf->can_id >>= 18;
        }

        int i;
        for (i = 0; i < canmsg->len; ++i) {
            cf->data[i] = canmsg->dat[i];
        }

        pthread_mutex_lock(&ds->mutex);
        while (ds->queue_stopped) {
            struct timespec to;
            clock_gettime(CLOCK_MONOTONIC, &to);
            uint64_t nsec = timespec2nsec(&to);
            nsec += optb_restart_ms * NANO / MILLI;
            nsec2timespec(&to, nsec);

            int err = pthread_cond_timedwait(&ds->cond, &ds->mutex, &to);

            if (err == ETIMEDOUT) {
                ds->queue_stopped = 0;
            }
        }
        pthread_mutex_unlock(&ds->mutex);

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
            log_warn("netif_rx: %s: TX timeout (by netdevice driver)\n",
                    skb->dev->name);
        }
        if (msg->can_id & CAN_ERR_LOSTARB) {
            log_warn("netif_rx: %s: lost arbitration: %x\n",
                    skb->dev->name, msg->data[0]);
        }
        if (msg->can_id & CAN_ERR_CRTL) {
            log_warn("netif_rx: %s: controller problems: %x\n",
                    skb->dev->name, msg->data[1]);
        }
        if (msg->can_id & CAN_ERR_PROT) {
            log_warn("netif_rx: %s: protocol violations: %x, %x\n",
                    skb->dev->name, msg->data[2], msg->data[3]);
        }
        if (msg->can_id & CAN_ERR_TRX) {
            log_warn("netif_rx: %s: transceiver status: %x\n",
                    skb->dev->name, msg->data[4]);
        }
        if (msg->can_id & CAN_ERR_ACK) {
            log_warn("netif_rx: %s: received no ACK on transmission\n",
                    skb->dev->name);
        }
        if (msg->can_id & CAN_ERR_BUSOFF) {
            log_warn("netif_rx: %s: bus off\n", skb->dev->name);
        }
        if (msg->can_id & CAN_ERR_BUSERROR) {
            log_warn("netif_rx: %s: bus error (may flood!)\n",
                    skb->dev->name);
        }
        if (msg->can_id & CAN_ERR_RESTARTED) {
            log_warn("netif_rx: %s: controller restarted\n",
                    skb->dev->name);
        }
        if (msg->can_id & CAN_ERR_CNT) {
            log_warn("netif_rx: %s: TX error counter; tx:%x, rx:%x\n",
                    skb->dev->name, msg->data[6], msg->data[7]);
        }

        kfree_skb(skb);

        return NET_RX_SUCCESS;
    }

    if (msg->can_id & CAN_RTR_FLAG) {
        log_trace("netif_rx; CAN_RTR_FLAG\n");

        if (!skb->is_echo) {
            kfree_skb(skb);
        }

        return NET_RX_SUCCESS;
    }

    struct can_msg canmsg;

    /* set MID; omit EFF, RTR, ERR flags */
    canmsg.mid = (msg->can_id & CAN_ERR_MASK);

    if (msg->can_id & CAN_EFF_FLAG) { // Extended MID
        canmsg.ext.is_extended_mid = 1; // EFF
    }
    else { // Standard MID
        canmsg.ext.is_extended_mid = 0; // SFF

        /**
         * Message IDs or MIDs are slightly different on QNX compared to
         * Linux. The form of the ID depends on whether or not the driver
         * is using extended MIDs:
         *
         *      - In standard 11-bit MIDs, bits 18–28 define the MID.
         *      - In extended 29-bit MIDs, bits 0–28 define the MID.
         */

        canmsg.mid <<= 18;
    }

    // set TIMESTAMP
    if (optt) {
        canmsg.ext.timestamp = user_timestamp;
    }
    else if (user_timestamp_time != 0) {
        canmsg.ext.timestamp =
            user_timestamp + get_clock_time_us()/1000 - user_timestamp_time;
    }
    else {
        canmsg.ext.timestamp = get_clock_time_us()/1000;
    }

    canmsg.ext.is_remote_frame = (skb->is_echo ? 0 : 1);
    canmsg.len = msg->len; // set LEN

    int i;
    for (i = 0; i < CAN_MSG_DATA_MAX; ++i) {
        canmsg.dat[i] = msg->data[i]; // Set DAT
    }

    pthread_mutex_lock(&device_session_create_mutex);

    device_session_t* ds = skb->dev->device_session;

    client_session_t** it = &ds->root_client_session;
    while (it != NULL && *it != NULL) {
        if ((canmsg.mid & *(*it)->mfilter) == canmsg.mid) {
            if (enqueue(&(*it)->rx_queue, &canmsg) != EOK) {
            }
        }

        it = &(*it)->next;
    }

    pthread_mutex_unlock(&device_session_create_mutex);

    log_trace("netif_rx; %s [%s] %X [%d] %2X %2X %2X %2X %2X %2X %2X %2X\n",
            skb->dev->name,
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

    if (!skb->is_echo) {
        kfree_skb(skb);
    }

    return NET_RX_SUCCESS;
}

void netif_start_queue(struct net_device *dev)
{
    log_trace("netif_start_queue\n");
}

bool netif_carrier_ok(const struct net_device *dev) {
    if ((dev->state >> __LINK_STATE_NOCARRIER) & 1) {
        return false;
    }

    return true;
}

void netif_carrier_on(struct net_device *dev) {
    log_trace("netif_carrier_on\n");

    dev->state &= ~(1 << __LINK_STATE_NOCARRIER);
}

void netif_carrier_off(struct net_device *dev) {
    log_trace("netif_carrier_off\n");

    dev->state |= (1 << __LINK_STATE_NOCARRIER);
}

int netif_queue_stopped(const struct net_device *dev)
{
    log_trace("netif_queue_stopped\n");

    device_session_t* ds = dev->device_session;

    int result = 0;

    pthread_mutex_lock(&ds->mutex);
    result = ds->queue_stopped;
    pthread_mutex_unlock(&ds->mutex);

    return result;
}

void netif_wake_queue (struct net_device* dev) {
    log_trace("netif_wake_queue\n");

    device_session_t* ds = dev->device_session;

    pthread_mutex_lock(&ds->mutex);
    if (ds->queue_stopped == 1) {
        ds->queue_stopped = 0;
        pthread_cond_signal(&ds->cond);
    }
    pthread_mutex_unlock(&ds->mutex);
}

void netif_stop_queue (struct net_device* dev) {
    log_trace("netif_stop_queue\n");

    device_session_t* ds = dev->device_session;

    pthread_mutex_lock(&ds->mutex);
    ds->queue_stopped = 1;
    pthread_mutex_unlock(&ds->mutex);
}
