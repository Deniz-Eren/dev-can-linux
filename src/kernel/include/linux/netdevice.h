/*
 * INET		An implementation of the TCP/IP protocol suite for the LINUX
 *		operating system.  INET is implemented using the  BSD Socket
 *		interface as the means of communication with the user level.
 *
 *		Definitions for the Interfaces handler.
 *
 * Version:	@(#)dev.h	1.0.10	08/12/93
 *
 * Authors:	Ross Biro
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 *		Corey Minyard <wf-rch!minyard@relay.EU.net>
 *		Donald J. Becker, <becker@cesdis.gsfc.nasa.gov>
 *		Alan Cox, <alan@lxorguk.ukuu.org.uk>
 *		Bjorn Ekwall. <bj0rn@blox.se>
 *              Pekka Riikonen <priikone@poseidon.pspt.fi>
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 *		Moved to /usr/include/linux for NET3
 */
#ifndef _LINUX_NETDEVICE_H
#define _LINUX_NETDEVICE_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/skbuff.h>
#include <uapi/linux/if.h>
#include <limits.h>


enum netdev_tx {
	__NETDEV_TX_MIN	 = INT_MIN,	/* make sure enum is signed */
	NETDEV_TX_OK	 = 0x00,	/* driver took care of packet */
	NETDEV_TX_BUSY	 = 0x10,	/* driver tx path was busy*/
	NETDEV_TX_LOCKED = 0x20,	/* driver tx lock was already taken */
};
typedef enum netdev_tx netdev_tx_t;

/*
 *	Old network device statistics. Fields are native words
 *	(unsigned long) so they can be read and written atomically.
 */

struct net_device_stats {
	unsigned long	rx_packets;
	unsigned long	tx_packets;
	unsigned long	rx_bytes;
	unsigned long	tx_bytes;
	unsigned long	rx_errors;
	unsigned long	tx_errors;
	unsigned long	rx_dropped;
	unsigned long	tx_dropped;
	unsigned long	multicast;
	unsigned long	collisions;
	unsigned long	rx_length_errors;
	unsigned long	rx_over_errors;
	unsigned long	rx_crc_errors;
	unsigned long	rx_frame_errors;
	unsigned long	rx_fifo_errors;
	unsigned long	rx_missed_errors;
	unsigned long	tx_aborted_errors;
	unsigned long	tx_carrier_errors;
	unsigned long	tx_fifo_errors;
	unsigned long	tx_heartbeat_errors;
	unsigned long	tx_window_errors;
	unsigned long	rx_compressed;
	unsigned long	tx_compressed;
};

struct net_device;

/*
 * This structure defines the management hooks for network devices.
 * The following hooks can be defined; unless noted otherwise, they are
 * optional and can be filled with a null pointer.
 *
 * int (*ndo_open)(struct net_device *dev);
 *     This function is called when network device transistions to the up
 *     state.
 *
 * int (*ndo_stop)(struct net_device *dev);
 *     This function is called when network device transistions to the down
 *     state.
 *
 * netdev_tx_t (*ndo_start_xmit)(struct sk_buff *skb,
 *                               struct net_device *dev);
 *	Called when a packet needs to be transmitted.
 *	Returns NETDEV_TX_OK.  Can return NETDEV_TX_BUSY, but you should stop
 *	the queue before that can happen; it's for obsolete devices and weird
 *	corner cases, but the stack really does a non-trivial amount
 *	of useless work if you return NETDEV_TX_BUSY.
 *        (can also return NETDEV_TX_LOCKED iff NETIF_F_LLTX)
 *	Required can not be NULL.
 *
 * int (*ndo_change_mtu)(struct net_device *dev, int new_mtu);
 *	Called when a user wants to change the Maximum Transfer Unit
 *	of a device. If not defined, any request to change MTU will
 *	will return an error.
 *
 */
struct net_device_ops {
	int			(*ndo_open)(struct net_device *dev);
	int			(*ndo_stop)(struct net_device *dev);
	netdev_tx_t		(*ndo_start_xmit) (struct sk_buff *skb,
						   struct net_device *dev);
	int			(*ndo_change_mtu)(struct net_device *dev,
						  int new_mtu);
};

/**
 *	struct net_device - The DEVICE structure.
 *		Actually, this whole structure is a big mistake.  It mixes I/O
 *		data with strictly "high-level" data, and it has to know about
 *		almost every data structure used in the INET module.
 *
 *	@name:	This is the first field of the "visible" part of this structure
 *		(i.e. as seen by users in the "Space.c" file).  It is the name
 *	 	of the interface.
 *
 *	@irq:		Device IRQ number
 *
 *	@stats:		Statistics struct, which was left as a legacy, use
 *			rtnl_link_stats64 instead
 *
 *	@netdev_ops:	Includes several pointers to callbacks,
 *			if one wants to override the ndo_*() functions
 *
 *	@flags:		Interface flags (a la BSD)
 *
 *	@mtu:		Interface MTU value
 *
 * 	@dev_id:		Used to differentiate devices that share
 * 				the same link layer address
 *
 *  @priv:		Private data
 */

struct net_device {
    char      	name[IFNAMSIZ];
    unsigned int         irq;
	struct net_device_stats stats;
    unsigned int        flags;
    unsigned int        mtu;
    const struct net_device_ops *netdev_ops;
    unsigned short          dev_id;
	void* priv;
};

/**
 *	netdev_priv - access network device private data
 *	@dev: network device
 *
 * Get network device private data
 */
static inline void *netdev_priv(const struct net_device *dev)
{
	return dev->priv;
}

/**
 *	netif_start_queue - allow transmit
 *	@dev: network device
 *
 *	Allow upper layers to call the device hard_start_xmit routine.
 */
void netif_start_queue(struct net_device *dev);

/**
 *	netif_wake_queue - restart transmit
 *	@dev: network device
 *
 *	Allow upper layers to call the device hard_start_xmit routine.
 *	Used for flow control when transmit resources are available.
 */
void netif_wake_queue(struct net_device *dev);

/**
 *	netif_stop_queue - stop transmitted packets
 *	@dev: network device
 *
 *	Stop upper layers calling the device hard_start_xmit routine.
 *	Used for flow control when transmit resources are unavailable.
 */
void netif_stop_queue(struct net_device *dev);

/**
 *	netif_queue_stopped - test if transmit queue is flowblocked
 *	@dev: network device
 *
 *	Test if transmit queue on device is currently unable to send.
 */
int netif_queue_stopped(const struct net_device *dev);

int netif_rx(struct sk_buff *skb);

void netif_carrier_on(struct net_device *dev);

void netif_carrier_off(struct net_device *dev);

/* Set the sysfs physical device reference for the network logical device
 * if set prior to registration will cause a symlink during initialization.
 */
//#define SET_NETDEV_DEV(net, pdev)   ((net)->dev.parent = (pdev))
#define SET_NETDEV_DEV(net, pdev) // Undefine this since we don't use it

#endif	/* _LINUX_NETDEVICE_H */
