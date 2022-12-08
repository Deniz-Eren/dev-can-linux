/*
 *	Definitions for the 'struct sk_buff' memory handlers.
 *
 *	Authors:
 *		Alan Cox, <gw4pts@gw4pts.ampr.org>
 *		Florian La Roche, <rzsfl@rz.uni-sb.de>
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#ifndef _LINUX_SKBUFF_H
#define _LINUX_SKBUFF_H

#include <linux/kernel.h>
#include <linux/compiler.h>


/** 
 *	struct sk_buff - socket buffer
 *	@data:			Data head pointer
 * 	@dev_id:		Used to differentiate devices that share
 * 				the same link layer address
 */

struct sk_buff {
	unsigned char* data;
	struct net_device* dev;
};

void kfree_skb (struct sk_buff* skb);

#endif	/* _LINUX_SKBUFF_H */
