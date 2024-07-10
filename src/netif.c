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

#include <drivers/net/can/sja1000/sja1000.h>
#include <session.h>

#include "netif.h"
#include "interrupt.h"


void* netif_tx (void* arg) {
    device_session_t* ds = (device_session_t*)arg;
    struct net_device* dev = ds->device;
    struct can_msg* canmsg;

    queue_start(&ds->tx_queue);

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
        struct sja1000_priv *priv = netdev_priv(skb->dev);
        enum can_state state = priv->can.state;

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

        if (optR_error_count != 0) {
            bool restart_trigger = false;

            switch (state) {
            case CAN_STATE_ERROR_ACTIVE:    /* RX/TX error count < 96 */
                if (optR_error_count < 96) {
                    restart_trigger = true;

                    log_trace("netif_rx: can_restart_now (Error Active)\n");

                }
                break;
            case CAN_STATE_ERROR_WARNING:   /* RX/TX error count < 128 */
                if (optR_error_count < 128) {
                    restart_trigger = true;

                    log_trace("netif_rx: can_restart_now (Error Warning)\n");
                }
                break;
            case CAN_STATE_ERROR_PASSIVE:   /* RX/TX error count < 256 */
                if (optR_error_count < 256) {
                    restart_trigger = true;

                    log_trace("netif_rx: can_restart_now (Error Passive)\n");
                }
                break;
            case CAN_STATE_BUS_OFF:         /* RX/TX error count >= 256 */
                if (optR_error_count >= 256) {
                    restart_trigger = true;

                    log_trace("netif_rx: can_restart_now (Error Bus-Off)\n");
                }
                break;
            case CAN_STATE_STOPPED:         /* Device is stopped */
                restart_trigger = true;
                log_trace("netif_rx: can_restart_now (State Stopped)\n");
                break;
            case CAN_STATE_SLEEPING:        /* Device is sleeping */
                restart_trigger = true;
                log_trace("netif_rx: can_restart_now (State Sleeping)\n");
                break;
            default:
                break;
            }

            if (restart_trigger) {
                struct can_priv *priv = netdev_priv(skb->dev);

                // Save automatic restart
                int backup_restart_ms = priv->restart_ms;

                priv->restart_ms = 0;            // Force automatic restart off
                priv->state = CAN_STATE_BUS_OFF; // Force CAN-bus state Bus-off

                int err = can_restart_now(skb->dev);
                if (err) {
                    log_err("netif_rx: can_restart_now error (%d)\n", err);
                }

                // Restore automatic restart
                priv->restart_ms = backup_restart_ms;
            }
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

    if (!optE) {
        if (skb->is_echo) {
            return NET_RX_SUCCESS;
        }
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

    client_session_t* it = ds->root_client_session;
    while (it != NULL) {
        if ((canmsg.mid & *it->mfilter) == canmsg.mid) {
            if (enqueue(&it->rx_queue, &canmsg) != EOK) {
            }
        }

        it = it->next;
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

    return queue_is_stopped(&ds->tx_queue);
}

void netif_wake_queue (struct net_device* dev) {
    log_trace("netif_wake_queue\n");

    device_session_t* ds = dev->device_session;

    queue_wake_up(&ds->tx_queue);
}

void netif_stop_queue (struct net_device* dev) {
    log_trace("netif_stop_queue\n");

    device_session_t* ds = dev->device_session;

    queue_stop(&ds->tx_queue);
}

/*
 * Current design utilized a single thread to process IRQ requests, so the
 * following lock/unlock functions are not required at this stage.
 */
void netif_tx_lock (struct net_device *dev) {
}

void netif_tx_unlock (struct net_device *dev) {
}
