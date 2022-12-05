/*
 * netif.c
 *
 *  Created on: Dec 2, 2022
 *      Author: Deniz Eren
 */

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
            skb->dev_id,
            msg->can_id,
            msg->data[0],
            msg->data[1],
            msg->data[2],
            msg->data[3],
            msg->data[4],
            msg->data[5],
            msg->data[6],
            msg->data[7]);

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
