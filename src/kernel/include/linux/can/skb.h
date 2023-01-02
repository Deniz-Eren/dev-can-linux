/* SPDX-License-Identifier: (GPL-2.0 OR BSD-3-Clause) */
/*
 * \file    linux/can/skb.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Currently not handling canfd and canxl
 *
 * Definitions for the CAN network socket buffer
 *
 * Copyright (C) 2012 Oliver Hartkopp <socketcan@hartkopp.net>
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

#ifndef _CAN_SKB_H
#define _CAN_SKB_H

#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/can.h>
#ifndef __QNX__
#include <net/sock.h>
#endif

void can_flush_echo_skb(struct net_device *dev);
int can_put_echo_skb(struct sk_buff *skb, struct net_device *dev,
		     unsigned int idx, unsigned int frame_len);
struct sk_buff *__can_get_echo_skb(struct net_device *dev, unsigned int idx,
				   unsigned int *len_ptr,
				   unsigned int *frame_len_ptr);
unsigned int __must_check can_get_echo_skb(struct net_device *dev,
					   unsigned int idx,
					   unsigned int *frame_len_ptr);
void can_free_echo_skb(struct net_device *dev, unsigned int idx,
		       unsigned int *frame_len_ptr);
struct sk_buff *alloc_can_skb(struct net_device *dev, struct can_frame **cf);
struct sk_buff *alloc_can_err_skb(struct net_device *dev,
				  struct can_frame **cf);
bool can_dropped_invalid_skb(struct net_device *dev, struct sk_buff *skb);

/*
 * The struct can_skb_priv is used to transport additional information along
 * with the stored struct can(fd)_frame that can not be contained in existing
 * struct sk_buff elements.
 * N.B. that this information must not be modified in cloned CAN sk_buffs.
 * To modify the CAN frame content or the struct can_skb_priv content
 * skb_copy() needs to be used instead of skb_clone().
 */

/**
 * struct can_skb_priv - private additional data inside CAN sk_buffs
 * @ifindex:	ifindex of the first interface the CAN frame appeared on
 * @skbcnt:	atomic counter to have an unique id together with skb pointer
 * @frame_len:	length of CAN frame in data link layer
 * @cf:		align to the following CAN frame at skb->data
 */
struct can_skb_priv {
	int ifindex;
	int skbcnt;
	unsigned int frame_len;
	struct can_frame cf[1]; // TODO: check if this can be cf[] and needed to be changed to make it happen
};

static inline struct can_skb_priv *can_skb_prv(struct sk_buff *skb)
{
	return (struct can_skb_priv *)(skb->head);
}

/*
 * returns an unshared skb owned by the original sock to be echo'ed back
 */
static inline struct sk_buff *can_create_echo_skb(struct sk_buff *skb)
{
    /* no sharing of skb in the QNX application thus we can just return back */
    skb->is_echo = 1;
    return skb;
}

static inline bool can_is_can_skb(const struct sk_buff *skb)
{
	struct can_frame *cf = (struct can_frame *)skb->data;

	/* the CAN specific type of skb is identified by its data length */
	return (skb->len == CAN_MTU && cf->len <= CAN_MAX_DLEN);
}

static inline bool can_is_canfd_skb(const struct sk_buff *skb)
{
	struct canfd_frame *cfd = (struct canfd_frame *)skb->data;

	/* the CAN specific type of skb is identified by its data length */
	return (skb->len == CANFD_MTU && cfd->len <= CANFD_MAX_DLEN);
}

static inline bool can_is_canxl_skb(const struct sk_buff *skb)
{
	const struct canxl_frame *cxl = (struct canxl_frame *)skb->data;

	if (skb->len < CANXL_HDR_SIZE + CANXL_MIN_DLEN || skb->len > CANXL_MTU)
		return false;

	/* this also checks valid CAN XL data length boundaries */
	if (skb->len != CANXL_HDR_SIZE + cxl->len)
		return false;

	return cxl->flags & CANXL_XLF;
}

/* get length element value from can[|fd|xl]_frame structure */
static inline unsigned int can_skb_get_len_val(struct sk_buff *skb)
{
	const struct canxl_frame *cxl = (struct canxl_frame *)skb->data;
	const struct canfd_frame *cfd = (struct canfd_frame *)skb->data;

	if (can_is_canxl_skb(skb))
		return cxl->len;

	return cfd->len;
}

/* get needed data length inside CAN frame for all frame types (RTR aware) */
static inline unsigned int can_skb_get_data_len(struct sk_buff *skb)
{
	unsigned int len = can_skb_get_len_val(skb);
	const struct can_frame *cf = (struct can_frame *)skb->data;

	/* RTR frames have an actual length of zero */
	if (can_is_can_skb(skb) && (cf->can_id & CAN_RTR_FLAG))
		return 0;

	return len;
}

#endif /* !_CAN_SKB_H */
