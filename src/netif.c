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
	syslog(LOG_DEBUG, "netif_wake_queue");
}

int netif_rx(struct sk_buff *skb)
{
	struct can_frame* msg = (struct can_frame*)skb->data;

	syslog(LOG_DEBUG, "netif_rx; can%d: %x#%2x%2x%2x%2x%2x%2x%2x%2x",
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
	syslog(LOG_DEBUG, "netif_start_queue");
}

void netif_carrier_on(struct net_device *dev)
{
	syslog(LOG_DEBUG, "netif_carrier_on");
	// Not called from SJA1000 driver
}

void netif_carrier_off(struct net_device *dev)
{
	syslog(LOG_DEBUG, "netif_carrier_off");
}

int netif_queue_stopped(const struct net_device *dev)
{
	syslog(LOG_DEBUG, "netif_queue_stopped");

	return 0;
}

void netif_stop_queue(struct net_device *dev)
{
	syslog(LOG_DEBUG, "netif_stop_queue");
}
