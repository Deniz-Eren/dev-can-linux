/*
 * Copyright (C) 2005 Marc Kleine-Budde, Pengutronix
 * Copyright (C) 2006 Andrey Volkov, Varma Electronics
 * Copyright (C) 2008-2009 Wolfgang Grandegger <wg@grandegger.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the version 2 of the GNU General Public License
 * as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/can.h>
#include <linux/can/dev.h>


struct sk_buff *alloc_can_skb(struct net_device *dev, struct can_frame **cf)
{
	struct sk_buff *skb;

	skb = (struct sk_buff*)malloc(sizeof(struct sk_buff));

    *cf = (struct can_frame*)malloc(sizeof(struct can_frame));
	memset(*cf, 0, sizeof(struct can_frame));

    skb->data = (unsigned char*)(*cf);
    skb->dev_id = dev->dev_id;

	return skb;
}

struct sk_buff *alloc_can_err_skb(struct net_device *dev, struct can_frame **cf)
{
	struct sk_buff *skb;

	skb = alloc_can_skb(dev, cf);
	if (unlikely(!skb))
		return NULL;

	(*cf)->can_id = CAN_ERR_FLAG;
	(*cf)->can_dlc = CAN_ERR_DLC;

	return skb;
}

/*
 * Allocate and setup space for the CAN network device
 */
struct net_device *alloc_candev(int sizeof_priv, unsigned int echo_skb_max)
{
	struct net_device *dev;
	struct can_priv *priv;

    dev = (struct net_device*)malloc(sizeof(struct net_device));
	if (!dev)
		return NULL;

    dev->priv = malloc(sizeof_priv);
    if (!dev->priv)
    	return NULL;

    priv = (struct can_priv*)netdev_priv(dev);

	// TODO: echo
//    if (echo_skb_max) {
//        priv->echo_skb_max = echo_skb_max;
//        priv->echo_skb = (void *)priv +
//            ALIGN(sizeof_priv, sizeof(struct sk_buff *));
//    }

    priv->state = CAN_STATE_STOPPED;

    return dev;
}

/*
 * Free space of the CAN network device
 */
void free_candev(struct net_device *dev)
{
	free(netdev_priv(dev));
	free(dev);
}

/*
 * changing MTU and control mode for CAN/CANFD devices
 */
int can_change_mtu(struct net_device *dev, int new_mtu)
{
	struct can_priv *priv = (struct can_priv *)netdev_priv(dev);

	/* Do not allow changing the MTU while running */
	if (dev->flags & IFF_UP)
		return -EBUSY;

	/* allow change of MTU according to the CANFD ability of the device */
	switch (new_mtu) {
	case CAN_MTU:
		/* 'CANFD-only' controllers can not switch to CAN_MTU */
		if (priv->ctrlmode_static & CAN_CTRLMODE_FD)
			return -EINVAL;

		priv->ctrlmode &= ~CAN_CTRLMODE_FD;
		break;

	case CANFD_MTU:
		/* check for potential CANFD ability */
		if (!(priv->ctrlmode_supported & CAN_CTRLMODE_FD) &&
		    !(priv->ctrlmode_static & CAN_CTRLMODE_FD))
			return -EINVAL;

		priv->ctrlmode |= CAN_CTRLMODE_FD;
		break;

	default:
		return -EINVAL;
	}

	dev->mtu = new_mtu;
	return 0;
}
