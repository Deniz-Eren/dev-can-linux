/*
 * can.c
 *
 *  Created on: Dec 2, 2022
 *      Author: Deniz Eren
 */

#include <stdio.h>
#include <drivers/net/can/sja1000/sja1000.h>

#include "config.h"


extern struct net_device_ops sja1000_netdev_ops;

struct net_device* device[16];
netdev_tx_t (*dev_xmit[16]) (struct sk_buff *skb, struct net_device *dev);


int register_candev(struct net_device *dev) {
    snprintf(dev->name, IFNAMSIZ, "can%d", dev->dev_id);

    if (dev->dev_id >= 16) {
        log_err("device id (%d) exceeds max (%d)\n", dev->dev_id, 16);
    }

    dev_xmit[dev->dev_id] = dev->netdev_ops->ndo_start_xmit;
    device[dev->dev_id] = dev;

    log_trace("register_candev: %s\n", dev->name);

    if (sja1000_netdev_ops.ndo_open(dev)) {
        return -1;
    }

    return 0;
}

void unregister_candev(struct net_device *dev) {
    log_trace("unregister_candev: %s\n", dev->name);

    if (sja1000_netdev_ops.ndo_stop(dev)) {
        log_err("internal error; ndo_stop failure\n");
    }
}

int open_candev(struct net_device *dev)
{
    log_trace("open_candev: %s\n", dev->name);
    return 0;
}

void close_candev(struct net_device *dev)
{
    log_trace("close_candev: %s\n", dev->name);
}

void can_bus_off(struct net_device *dev) {
    log_trace("can_bus_off\n");
}

unsigned int can_get_echo_skb(struct net_device *dev, unsigned int idx) {
    log_trace("can_get_echo_skb\n");

    return 0;
}

void can_put_echo_skb(struct sk_buff *skb, struct net_device *dev,
              unsigned int idx)
{
    log_trace("can_put_echo_skb\n");
}

void can_free_echo_skb(struct net_device *dev, unsigned int idx) {
    log_trace("can_free_echo_skb\n");
}

void can_change_state(struct net_device *dev, struct can_frame *cf,
              enum can_state tx_state, enum can_state rx_state)
{
    log_trace("can_change_state\n");
}
