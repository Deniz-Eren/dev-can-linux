/*
 * \file    resmgr.h
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

#ifndef SRC_RESMGR_H_
#define SRC_RESMGR_H_

#include <pci/pci.h>

#include <drivers/net/can/sja1000/sja1000.h>

#define MAX_MAILBOXES 16
#define MAX_NAME_SIZE (IFNAMSIZ*2) // e.g. /dev/can1/rx0


/*
 * Instead of netlink interface we introduce this more specific and
 * simplified user settings. Note that the user in this context is not
 * necessarily the network layer. In the case of QNX dev-can-* drivers
 * the user is specifically a resource manager driver.
 */
struct user_dev_setup {
    struct can_bittiming bittiming;
    struct can_bittiming data_bittiming;
    struct can_ctrlmode ctrlmode;
    int restart_ms;
    struct can_tdc tdc;
    u16 termination;

    bool set_bittiming;
    bool set_data_bittiming;
    bool set_ctrlmode;
    bool set_restart_ms;
    bool set_restart;
    bool set_tdc;
    bool set_termination;
};

/*
 *  struct resmgr_ops - resource manager link operations
 *
 *  @changelink:      Function for changing parameters of an existing device
 *  @get_xstats_size: Function to calculate required room for dumping device
 *                    specific statistics
 *  @fill_xstats:     Function to dump device specific statistics
 */
struct resmgr_ops {
    int     (*changelink)(struct net_device* dev, struct user_dev_setup* user);

    size_t  (*get_xstats_size)(const struct net_device* dev);

    int     (*fill_xstats)(struct sk_buff* skb, const struct net_device* dev);
};

extern struct net_device* device[MAX_DEVICES];

#endif /* SRC_RESMGR_H_ */
