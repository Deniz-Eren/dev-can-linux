// SPDX-License-Identifier: GPL-2.0-only
/*
 * \file    driver/net/can/dev/skb.c
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Changes have been made to remove the networking layer and replace
 *          memory allocation, error handling and logging, together with changes
 *          to some structure internals. For example net_device and sk_buff
 *          structures have been greatly simplified by removing networking
 *          specific functionality and retaining only CAN-bus critical features.
 *
 * Copyright (C) 2005 Marc Kleine-Budde, Pengutronix
 * Copyright (C) 2006 Andrey Volkov, Varma Electronics
 * Copyright (C) 2008-2009 Wolfgang Grandegger <wg@grandegger.com>
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
 *
 * Please also check the "SPDX-License-Identifier" documentation from the Linux
 * Kernel source code repository: github.com/torvalds/linux.git for further
 * details.
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

#include <linux/can/dev.h>
#include <linux/module.h>

#define MOD_DESC "CAN device driver interface"

MODULE_DESCRIPTION(MOD_DESC);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Wolfgang Grandegger <wg@grandegger.com>");

/* Local echo of CAN messages
 *
 * CAN network devices *should* support a local echo functionality
 * (see Documentation/networking/can.rst). To test the handling of CAN
 * interfaces that do not support the local echo both driver types are
 * implemented. In the case that the driver does not support the echo
 * the IFF_ECHO remains clear in dev->flags. This causes the PF_CAN core
 * to perform the echo as a fallback solution.
 */
void can_flush_echo_skb(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	int i;

	for (i = 0; i < priv->echo_skb_max; i++) {
		if (priv->echo_skb[i]) {
			kfree_skb(priv->echo_skb[i]);
			priv->echo_skb[i] = NULL;
			stats->tx_dropped++;
			stats->tx_aborted_errors++;
		}
	}
}

/* Put the skb on the stack to be looped backed locally lateron
 *
 * The function is typically called in the start_xmit function
 * of the device driver. The driver must protect access to
 * priv->echo_skb, if necessary.
 */
int can_put_echo_skb(struct sk_buff *skb, struct net_device *dev,
		     unsigned int idx, unsigned int frame_len)
{
	struct can_priv *priv = netdev_priv(dev);

	if (idx >= priv->echo_skb_max) {
        netdev_err(dev, "%s: BUG! echo_skb %d >= %d !\n",
                __func__, idx, priv->echo_skb_max);

	    kfree_skb(skb);
        return -ENOMEM;
	}

	/* check flag whether this packet has to be looped back */
	if (!(dev->flags & IFF_ECHO)) {
		kfree_skb(skb);
		return 0;
	}

	if (!priv->echo_skb[idx]) {
		skb = can_create_echo_skb(skb);
		if (!skb)
			return -ENOMEM;

		/* make settings for echo to reduce code in irq context */
		skb->dev = dev;

		/* save frame_len to reuse it when transmission is completed */
		can_skb_prv(skb)->frame_len = frame_len;

		skb_tx_timestamp(skb);

		/* save this skb for tx interrupt echo handling */
		priv->echo_skb[idx] = skb;
	} else {
		/* locking problem with netif_stop_queue() ?? */
		netdev_err(dev, "%s: BUG! echo_skb %d is occupied!\n", __func__, idx);
		kfree_skb(skb);
		return -EBUSY;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(can_put_echo_skb);

struct sk_buff *
__can_get_echo_skb(struct net_device *dev, unsigned int idx,
		   unsigned int *len_ptr, unsigned int *frame_len_ptr)
{
	struct can_priv *priv = netdev_priv(dev);

	if (idx >= priv->echo_skb_max) {
		netdev_err(dev, "%s: BUG! Trying to access can_priv::echo_skb out of bounds (%u/max %u)\n",
			   __func__, idx, priv->echo_skb_max);
		return NULL;
	}

	if (priv->echo_skb[idx]) {
		/* Using "struct canfd_frame::len" for the frame
		 * length is supported on both CAN and CANFD frames.
		 */
		struct sk_buff *skb = priv->echo_skb[idx];
		struct can_skb_priv *can_skb_priv = can_skb_prv(skb);

		/* get the real payload length for netdev statistics */
		*len_ptr = can_skb_get_data_len(skb);

		if (frame_len_ptr)
			*frame_len_ptr = can_skb_priv->frame_len;

		priv->echo_skb[idx] = NULL;

		return skb;
	}

	return NULL;
}

/* Get the skb from the stack and loop it back locally
 *
 * The function is typically called when the TX done interrupt
 * is handled in the device driver. The driver must protect
 * access to priv->echo_skb, if necessary.
 */
unsigned int can_get_echo_skb(struct net_device *dev, unsigned int idx,
			      unsigned int *frame_len_ptr)
{
	struct sk_buff *skb;
	unsigned int len;

	skb = __can_get_echo_skb(dev, idx, &len, frame_len_ptr);
	if (!skb)
		return 0;

	netif_rx(skb);

    kfree_skb(skb);

	return len;
}
EXPORT_SYMBOL_GPL(can_get_echo_skb);

/* Remove the skb from the stack and free it.
 *
 * The function is typically called when TX failed.
 */
void can_free_echo_skb(struct net_device *dev, unsigned int idx,
		       unsigned int *frame_len_ptr)
{
	struct can_priv *priv = netdev_priv(dev);

	if (idx >= priv->echo_skb_max) {
		netdev_err(dev, "%s: BUG! Trying to access can_priv::echo_skb out of bounds (%u/max %u)\n",
			   __func__, idx, priv->echo_skb_max);
		return;
	}

	if (priv->echo_skb[idx]) {
		struct sk_buff *skb = priv->echo_skb[idx];
		struct can_skb_priv *can_skb_priv = can_skb_prv(skb);

		if (frame_len_ptr)
			*frame_len_ptr = can_skb_priv->frame_len;

		kfree_skb(skb);
		priv->echo_skb[idx] = NULL;
	}
}
EXPORT_SYMBOL_GPL(can_free_echo_skb);

struct sk_buff *alloc_can_skb(struct net_device *dev, struct can_frame **cf)
{
    struct sk_buff *skb;
    struct can_skb_priv *skb_priv;

    skb = (struct sk_buff*)fixed_malloc(sizeof(struct sk_buff));
    if (unlikely(!skb))
        return NULL;

    skb_priv = fixed_malloc(sizeof(struct can_skb_priv));
    memset(skb_priv, 0, sizeof(struct can_skb_priv));

    *cf = skb_priv->cf;

    skb->head = (unsigned char*)skb_priv;
    skb->data = (unsigned char*)(*cf);
    skb->dev = dev;

    return skb;
}
EXPORT_SYMBOL_GPL(alloc_can_skb);

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
EXPORT_SYMBOL_GPL(alloc_can_err_skb);

/* Check for outgoing skbs that have not been created by the CAN subsystem */
/* Drop a given socketbuffer if it does not contain a valid CAN frame. */
bool can_dropped_invalid_skb(struct net_device *dev, struct sk_buff *skb)
{
//	switch (ntohs(skb->protocol)) {
//	case ETH_P_CAN:
		if (!can_is_can_skb(skb))
			goto inval_skb;
//		break;
//
//	case ETH_P_CANFD:
//		if (!can_is_canfd_skb(skb))
//			goto inval_skb;
//		break;
//
//	case ETH_P_CANXL:
//		if (!can_is_canxl_skb(skb))
//			goto inval_skb;
//		break;
//
//	default:
//		goto inval_skb;
//	}
//
//	if (!can_skb_headroom_valid(dev, skb))
//		goto inval_skb;

	return false;

inval_skb:
	kfree_skb(skb);
	dev->stats.tx_dropped++;
	return true;
}
EXPORT_SYMBOL_GPL(can_dropped_invalid_skb);
