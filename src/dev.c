/*
 * can.c
 *
 *  Created on: Dec 2, 2022
 *      Author: Deniz Eren
 */

#include <stdio.h>
#include <drivers/net/can/sja1000/sja1000.h>

extern struct net_device_ops sja1000_netdev_ops;

struct net_device* device[16];
netdev_tx_t (*dev_xmit[16]) (struct sk_buff *skb, struct net_device *dev);


int register_candev(struct net_device *dev) {
	snprintf(dev->name, IFNAMSIZ, "can%d", dev->dev_id);

	if (dev->dev_id >= 16) {
		syslog(LOG_ERR, "device id (%d) exceeds max (%d)", dev->dev_id, 16);
	}

	dev_xmit[dev->dev_id] = dev->netdev_ops->ndo_start_xmit;
	device[dev->dev_id] = dev;

	syslog(LOG_DEBUG, "register_candev: %s", dev->name);

	if (sja1000_netdev_ops.ndo_open(dev)) {
		return -1;
	}

	return 0;
}

void unregister_candev(struct net_device *dev) {
	syslog(LOG_DEBUG, "unregister_candev: %s", dev->name);

	if (sja1000_netdev_ops.ndo_stop(dev)) {
		syslog(LOG_ERR, "internal error; ndo_stop failure");
	}
}

int open_candev(struct net_device *dev)
{
	syslog(LOG_DEBUG, "open_candev: %s", dev->name);
	return 0;
}

void close_candev(struct net_device *dev)
{
	syslog(LOG_DEBUG, "close_candev: %s", dev->name);
}

void can_bus_off(struct net_device *dev) {
	syslog(LOG_DEBUG, "can_bus_off");
}

unsigned int can_get_echo_skb(struct net_device *dev, unsigned int idx) {
	syslog(LOG_DEBUG, "can_get_echo_skb");

	return 0;
}

void can_put_echo_skb(struct sk_buff *skb, struct net_device *dev,
		      unsigned int idx)
{
	syslog(LOG_DEBUG, "can_put_echo_skb");
}

void can_free_echo_skb(struct net_device *dev, unsigned int idx) {
	syslog(LOG_DEBUG, "can_free_echo_skb");
}

void can_change_state(struct net_device *dev, struct can_frame *cf,
		      enum can_state tx_state, enum can_state rx_state)
{
	syslog(LOG_DEBUG, "can_change_state");
}
