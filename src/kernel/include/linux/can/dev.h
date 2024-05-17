/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/can/dev.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Changes have been made to remove the networking layer and replace
 *          timers, memory allocation, error handling and logging, together with
 *          changes to some structure internals. For example net_device
 *          structure has been greatly simplified by removing networking
 *          specific functionality and retaining only CAN-bus critical features.
 *
 * Definitions for the CAN network device driver interface
 *
 * Copyright (C) 2006 Andrey Volkov <avolkov@varma-el.com>
 *               Varma Electronics Oy
 *
 * Copyright (C) 2008 Wolfgang Grandegger <wg@grandegger.com>
 *
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

#ifndef _CAN_DEV_H
#define _CAN_DEV_H

#include <linux/types.h>
#include <linux/can.h>
#include <linux/can/bittiming.h>
#include <linux/can/error.h>
#include <linux/can/length.h>
#include <linux/can/netlink.h>
#include <linux/can/skb.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/math64.h>

#include "timer.h"

/*
 * CAN mode
 */
enum can_mode {
	CAN_MODE_STOP = 0,
	CAN_MODE_START,
	CAN_MODE_SLEEP
};

enum can_termination_gpio {
	CAN_TERMINATION_GPIO_DISABLED = 0,
	CAN_TERMINATION_GPIO_ENABLED,
	CAN_TERMINATION_GPIO_MAX,
};

/*
 * CAN common private data
 */
struct can_priv {
	struct net_device *dev;
	struct can_device_stats can_stats;

	const struct can_bittiming_const *bittiming_const,
		*data_bittiming_const;
	struct can_bittiming bittiming, data_bittiming;
	const struct can_tdc_const *tdc_const;
	struct can_tdc tdc;

	unsigned int bitrate_const_cnt;
	const u32 *bitrate_const;
	const u32 *data_bitrate_const;
	unsigned int data_bitrate_const_cnt;
	u32 bitrate_max;
	struct can_clock clock;

	unsigned int termination_const_cnt;
	const u16 *termination_const;
	u16 termination;
#ifndef __QNX__
	struct gpio_desc *termination_gpio;
	u16 termination_gpio_ohms[CAN_TERMINATION_GPIO_MAX];
#endif

	unsigned int echo_skb_max;
	struct sk_buff **echo_skb;

	enum can_state state;

	/* CAN controller features - see include/uapi/linux/can/netlink.h */
	u32 ctrlmode;		/* current options setting */
	u32 ctrlmode_supported;	/* options that can be modified by netlink */

	int restart_ms;
	timer_record_t restart_work;

	int (*do_set_bittiming)(struct net_device *dev);
	int (*do_set_data_bittiming)(struct net_device *dev);
    /* Special feature to force btr0 and btr1 to specific values needed for some
       applications. */
    int (*do_set_btr)(struct net_device *dev, u8 btr0, u8 btr1);
	int (*do_set_mode)(struct net_device *dev, enum can_mode mode);
	int (*do_set_termination)(struct net_device *dev, u16 term);
	int (*do_get_state)(const struct net_device *dev,
			    enum can_state *state);
	int (*do_get_berr_counter)(const struct net_device *dev,
				   struct can_berr_counter *bec);
	int (*do_get_auto_tdcv)(const struct net_device *dev, u32 *tdcv);
};

static inline u32 can_get_static_ctrlmode(struct can_priv *priv)
{
	return priv->ctrlmode & ~priv->ctrlmode_supported;
}

/* drop skb if it does not contain a valid CAN frame for sending */
static inline bool can_dev_dropped_skb(struct net_device *dev, struct sk_buff *skb)
{
	struct can_priv *priv = netdev_priv(dev);

	if (priv->ctrlmode & CAN_CTRLMODE_LISTENONLY) {
		netdev_info_once(dev,
				 "interface in listen only mode, dropping skb\n");
		kfree_skb(skb);
		dev->stats.tx_dropped++;
		return true;
	}

	return can_dropped_invalid_skb(dev, skb);
}

void can_setup(struct net_device *dev);

struct net_device *alloc_candev_mqs(int sizeof_priv, unsigned int echo_skb_max,
				    unsigned int txqs, unsigned int rxqs);
#define alloc_candev(sizeof_priv, echo_skb_max) \
	alloc_candev_mqs(sizeof_priv, echo_skb_max, 1, 1)
#define alloc_candev_mq(sizeof_priv, echo_skb_max, count) \
	alloc_candev_mqs(sizeof_priv, echo_skb_max, count, count)
void free_candev(struct net_device *dev);

int open_candev(struct net_device *dev);
void close_candev(struct net_device *dev);
int can_change_mtu(struct net_device *dev, int new_mtu);

int register_candev(struct net_device *dev);
void unregister_candev(struct net_device *dev);

int can_restart_now(struct net_device *dev);
void can_bus_off(struct net_device *dev);

const char *can_get_state_str(const enum can_state state);
void can_change_state(struct net_device *dev, struct can_frame *cf,
		      enum can_state tx_state, enum can_state rx_state);

#ifdef CONFIG_OF
void of_can_transceiver(struct net_device *dev);
#else
static inline void of_can_transceiver(struct net_device *dev) { }
#endif

extern struct resmgr_ops can_link_ops;

#endif /* !_CAN_DEV_H */
