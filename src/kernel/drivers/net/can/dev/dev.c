// SPDX-License-Identifier: GPL-2.0-only
/*
 * \file    driver/net/can/dev/dev.c
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Changes have been made to remove the networking layer and replace
 *          timers, memory allocation, error handling and logging, together with
 *          changes to some structure internals. For example net_device
 *          structure has been greatly simplified by removing networking
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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/netdevice.h>
#include <linux/if.h>
#include <linux/err.h>
#include <linux/can.h>
#include <linux/can/dev.h>
#include <linux/can/skb.h>
#include <linux/can/netlink.h>
#include <resmgr.h>

/*
 * Check the size of sk_buff and can_frame are within the block-size limit of
 * the fixed-block allocator
 */
static_assert( sizeof(struct sk_buff) <= FIXED_MAX_BLOCK_SIZE,
        "sk_buff exceeds fixed block-size" );

static_assert( sizeof(struct can_skb_priv) <= FIXED_MAX_BLOCK_SIZE,
        "can_frame exceeds fixed block-size" );


static void can_update_state_error_stats(struct net_device *dev,
					 enum can_state new_state)
{
	struct can_priv *priv = netdev_priv(dev);

	if (new_state <= priv->state)
		return;

	switch (new_state) {
	case CAN_STATE_ERROR_WARNING:
		priv->can_stats.error_warning++;
		break;
	case CAN_STATE_ERROR_PASSIVE:
		priv->can_stats.error_passive++;
		break;
	case CAN_STATE_BUS_OFF:
		priv->can_stats.bus_off++;
		break;
	default:
		break;
	}
}

static int can_tx_state_to_frame(struct net_device *dev, enum can_state state)
{
	switch (state) {
	case CAN_STATE_ERROR_ACTIVE:
		return CAN_ERR_CRTL_ACTIVE;
	case CAN_STATE_ERROR_WARNING:
		return CAN_ERR_CRTL_TX_WARNING;
	case CAN_STATE_ERROR_PASSIVE:
		return CAN_ERR_CRTL_TX_PASSIVE;
	default:
		return 0;
	}
}

static int can_rx_state_to_frame(struct net_device *dev, enum can_state state)
{
	switch (state) {
	case CAN_STATE_ERROR_ACTIVE:
		return CAN_ERR_CRTL_ACTIVE;
	case CAN_STATE_ERROR_WARNING:
		return CAN_ERR_CRTL_RX_WARNING;
	case CAN_STATE_ERROR_PASSIVE:
		return CAN_ERR_CRTL_RX_PASSIVE;
	default:
		return 0;
	}
}

const char *can_get_state_str(const enum can_state state)
{
	switch (state) {
	case CAN_STATE_ERROR_ACTIVE:
		return "Error Active";
	case CAN_STATE_ERROR_WARNING:
		return "Error Warning";
	case CAN_STATE_ERROR_PASSIVE:
		return "Error Passive";
	case CAN_STATE_BUS_OFF:
		return "Bus Off";
	case CAN_STATE_STOPPED:
		return "Stopped";
	case CAN_STATE_SLEEPING:
		return "Sleeping";
	default:
		return "<unknown>";
	}

	return "<unknown>";
}
EXPORT_SYMBOL_GPL(can_get_state_str);

static enum can_state can_state_err_to_state(u16 err)
{
	if (err < CAN_ERROR_WARNING_THRESHOLD)
		return CAN_STATE_ERROR_ACTIVE;
	if (err < CAN_ERROR_PASSIVE_THRESHOLD)
		return CAN_STATE_ERROR_WARNING;
	if (err < CAN_BUS_OFF_THRESHOLD)
		return CAN_STATE_ERROR_PASSIVE;

	return CAN_STATE_BUS_OFF;
}

void can_state_get_by_berr_counter(const struct net_device *dev,
				   const struct can_berr_counter *bec,
				   enum can_state *tx_state,
				   enum can_state *rx_state)
{
	*tx_state = can_state_err_to_state(bec->txerr);
	*rx_state = can_state_err_to_state(bec->rxerr);
}
EXPORT_SYMBOL_GPL(can_state_get_by_berr_counter);

void can_change_state(struct net_device *dev, struct can_frame *cf,
		      enum can_state tx_state, enum can_state rx_state)
{
	struct can_priv *priv = netdev_priv(dev);
	enum can_state new_state = max(tx_state, rx_state);

	if (unlikely(new_state == priv->state)) {
		netdev_warn(dev, "%s: oops, state did not change", __func__);
		return;
	}

	netdev_dbg(dev, "Controller changed from %s State (%d) into %s State (%d).\n",
		   can_get_state_str(priv->state), priv->state,
		   can_get_state_str(new_state), new_state);

	can_update_state_error_stats(dev, new_state);
	priv->state = new_state;

	if (!cf)
		return;

	if (unlikely(new_state == CAN_STATE_BUS_OFF)) {
		cf->can_id |= CAN_ERR_BUSOFF;
		return;
	}

	cf->can_id |= CAN_ERR_CRTL;
	cf->data[1] |= tx_state >= rx_state ?
		       can_tx_state_to_frame(dev, tx_state) : 0;
	cf->data[1] |= tx_state <= rx_state ?
		       can_rx_state_to_frame(dev, rx_state) : 0;
}
EXPORT_SYMBOL_GPL(can_change_state);

/* CAN device restart for bus-off recovery */
static void can_restart(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);
	struct sk_buff *skb;
	struct can_frame *cf;
	int err;

	if (netif_carrier_ok(dev))
		netdev_err(dev, "Attempt to restart for bus-off recovery, but carrier is OK?\n");

	/* No synchronization needed because the device is bus-off and
	 * no messages can come in or go out.
	 */
	can_flush_echo_skb(dev);

	/* send restart message upstream */
	skb = alloc_can_err_skb(dev, &cf);
	if (skb) {
		cf->can_id |= CAN_ERR_RESTARTED;
		netif_rx(skb);
	}

	/* Now restart the device */
	netif_carrier_on(dev);
	err = priv->do_set_mode(dev, CAN_MODE_START);
	if (err) {
		netdev_err(dev, "Restart failed, error %pe\n", ERR_PTR(err));
		netif_carrier_off(dev);
	} else {
		netdev_dbg(dev, "Restarted\n");
		priv->can_stats.restarts++;
	}
}

static void can_restart_work(void *data)
{
    struct can_priv *priv = data;

	can_restart(priv->dev);
}

int can_restart_now(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);

	/* A manual restart is only permitted if automatic restart is
	 * disabled and the device is in the bus-off state
	 */
	if (priv->restart_ms)
		return -EINVAL;
	if (priv->state != CAN_STATE_BUS_OFF)
		return -EBUSY;

	cancel_delayed_work_sync(&priv->restart_work);
	can_restart(dev);

	return 0;
}

/* CAN bus-off
 *
 * This functions should be called when the device goes bus-off to
 * tell the netif layer that no more packets can be sent or received.
 * If enabled, a timer is started to trigger bus-off recovery.
 */
void can_bus_off(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);

	if (priv->restart_ms) {
		netdev_info(dev, "bus-off, scheduling restart in %d ms\n",
			    priv->restart_ms);
	}
	else {
		netdev_info(dev, "bus-off\n");
	}

	netif_carrier_off(dev);

	if (priv->restart_ms)
		schedule_delayed_work(&priv->restart_work, priv->restart_ms);
}
EXPORT_SYMBOL_GPL(can_bus_off);

void can_setup(struct net_device *dev)
{
	dev->mtu = CAN_MTU;
	dev->tx_queue_len = 10;

	/* New-style flags. */
	dev->flags = IFF_NOARP;
}

/* Allocate and setup space for the CAN network device */
struct net_device *alloc_candev_mqs(int sizeof_priv, unsigned int echo_skb_max,
				    unsigned int txqs, unsigned int rxqs)
{
	struct net_device *dev;
	struct can_priv *priv;

    dev = (struct net_device*)malloc(sizeof(struct net_device));
	if (!dev)
		return NULL;

	memset(dev, 0, sizeof(struct net_device));

    dev->priv = malloc(sizeof_priv);
    if (!dev->priv)
    	return NULL;

    memset(dev->priv, 0, sizeof_priv);

	priv = netdev_priv(dev);
	priv->dev = dev;

	if (echo_skb_max) {
        int i;

        priv->echo_skb_max = echo_skb_max;
        priv->echo_skb = malloc(echo_skb_max*sizeof(struct sk_buff *));

        for (i = 0; i < priv->echo_skb_max; i++) {
            priv->echo_skb[i] = NULL;
        }
	}

	priv->state = CAN_STATE_STOPPED;

	setup_timer(&priv->restart_work, &can_restart_work, priv);

	return dev;
}
EXPORT_SYMBOL_GPL(alloc_candev_mqs);

/* Free space of the CAN network device */
void free_candev(struct net_device *dev)
{
	free(netdev_priv(dev));
	free(dev);
}
EXPORT_SYMBOL_GPL(free_candev);

/* changing MTU and control mode for CAN/CANFD devices */
int can_change_mtu(struct net_device *dev, int new_mtu)
{
	struct can_priv *priv = netdev_priv(dev);
	u32 ctrlmode_static = can_get_static_ctrlmode(priv);

	/* Do not allow changing the MTU while running */
	if (dev->flags & IFF_UP)
		return -EBUSY;

	/* allow change of MTU according to the CANFD ability of the device */
	switch (new_mtu) {
	case CAN_MTU:
		/* 'CANFD-only' controllers can not switch to CAN_MTU */
		if (ctrlmode_static & CAN_CTRLMODE_FD)
			return -EINVAL;

		priv->ctrlmode &= ~CAN_CTRLMODE_FD;
		break;

	case CANFD_MTU:
		/* check for potential CANFD ability */
		if (!(priv->ctrlmode_supported & CAN_CTRLMODE_FD) &&
		    !(ctrlmode_static & CAN_CTRLMODE_FD))
			return -EINVAL;

		priv->ctrlmode |= CAN_CTRLMODE_FD;
		break;

	default:
		return -EINVAL;
	}

	dev->mtu = new_mtu;
	return 0;
}
EXPORT_SYMBOL_GPL(can_change_mtu);

/* Common open function when the device gets opened.
 *
 * This function should be called in the open function of the device
 * driver.
 */
int open_candev(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);

	if (!priv->bittiming.bitrate) {
		netdev_err(dev, "bit-timing not yet defined\n");
		return -EINVAL;
	}

	/* For CAN FD the data bitrate has to be >= the arbitration bitrate */
	if ((priv->ctrlmode & CAN_CTRLMODE_FD) &&
	    (!priv->data_bittiming.bitrate ||
	     priv->data_bittiming.bitrate < priv->bittiming.bitrate)) {
		netdev_err(dev, "incorrect/missing data bit-timing\n");
		return -EINVAL;
	}

	/* Switch carrier on if device was stopped while in bus-off state */
	if (!netif_carrier_ok(dev))
		netif_carrier_on(dev);

	return 0;
}
EXPORT_SYMBOL_GPL(open_candev);

#ifdef CONFIG_OF
/* Common function that can be used to understand the limitation of
 * a transceiver when it provides no means to determine these limitations
 * at runtime.
 */
void of_can_transceiver(struct net_device *dev)
{
	struct device_node *dn;
	struct can_priv *priv = netdev_priv(dev);
	struct device_node *np = dev->dev.parent->of_node;
	int ret;

	dn = of_get_child_by_name(np, "can-transceiver");
	if (!dn)
		return;

	ret = of_property_read_u32(dn, "max-bitrate", &priv->bitrate_max);
	of_node_put(dn);
	if ((ret && ret != -EINVAL) || (!ret && !priv->bitrate_max))
		netdev_warn(dev, "Invalid value for transceiver max bitrate. Ignoring bitrate limit.\n");
}
EXPORT_SYMBOL_GPL(of_can_transceiver);
#endif

/* Common close function for cleanup before the device gets closed.
 *
 * This function should be called in the close function of the device
 * driver.
 */
void close_candev(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);

	cancel_delayed_work_sync(&priv->restart_work);
	can_flush_echo_skb(dev);
}
EXPORT_SYMBOL_GPL(close_candev);

/* Register the CAN network device */
int register_candev(struct net_device *dev)
{
	struct can_priv *priv = netdev_priv(dev);

	if (!priv->bitrate_const != !priv->bitrate_const_cnt)
		return -EINVAL;

	if (!priv->data_bitrate_const != !priv->data_bitrate_const_cnt)
		return -EINVAL;

	dev->resmgr_ops = &can_link_ops;
	netif_carrier_off(dev);

	return register_netdev(dev);
}
EXPORT_SYMBOL_GPL(register_candev);

/* Unregister the CAN network device */
void unregister_candev(struct net_device *dev)
{
	unregister_netdev(dev);
}
EXPORT_SYMBOL_GPL(unregister_candev);
