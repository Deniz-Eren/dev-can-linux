/*
 * \file    linux/skbuff.c
 * \brief   Defines simplified 'struct sk_buff' and function kfree_skb() only.
 *
 * \details This file has the exact same name and path as the one supplied by
 *          the Linux Kernel source-code, however the contents have been
 *          completely replaced. This has been done to keep the rest of the
 *          Linux Kernel source-code unchanged so that ongoing updates to the
 *          Linux Kernel can be propagated to this project easily.
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

#ifndef _LINUX_SKBUFF_H
#define _LINUX_SKBUFF_H

#include <linux/kernel.h>
#include <linux/compiler.h>


/** 
 * struct sk_buff
 * @data:   Data head pointer
 * @dev:    Used to access device data and callbacks
 */
struct sk_buff {
	unsigned int len, data_len, is_echo;
    unsigned char *head, *data;
	struct net_device* dev;
};

/**
 * kfree_skb - free an sk_buff
 * @skb: buffer to free
 */
static inline void kfree_skb (struct sk_buff* skb) {
    // TODO: Check if any of the Linux implementation house-keeping is needed
    fixed_free(skb->head);
    fixed_free(skb);
}

/**
 * skb_tx_timestamp() - Driver hook for transmit timestamping
 *
 * Ethernet MAC Drivers should call this function in their hard_xmit()
 * function immediately before giving the sk_buff to the MAC hardware.
 *
 * Specifically, one should make absolutely sure that this function is
 * called before TX completion of this packet can trigger.  Otherwise
 * the packet could potentially already be freed.
 *
 * @skb: A socket buffer.
 */
static inline void skb_tx_timestamp(struct sk_buff *skb)
{
    // TODO: perform timestamp functionality here if needed
}

#endif	/* _LINUX_SKBUFF_H */
