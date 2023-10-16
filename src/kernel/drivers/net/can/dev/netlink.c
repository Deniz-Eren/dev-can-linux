// SPDX-License-Identifier: GPL-2.0-only
/*
 * \file    driver/net/can/dev/netlink.c
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Changes have been made to remove the networking layer functionality.
 *          Only the modified versions of the changelink and xstats functions
 *          remain because they are relevant to CAN-bus setup and monitoring.
 *
 * Copyright (C) 2005 Marc Kleine-Budde, Pengutronix
 * Copyright (C) 2006 Andrey Volkov, Varma Electronics
 * Copyright (C) 2008-2009 Wolfgang Grandegger <wg@grandegger.com>
 * Copyright (C) 2021 Vincent Mailhol <mailhol.vincent@wanadoo.fr>
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

#include <resmgr.h>


static int can_changelink(struct net_device *dev,
        struct user_dev_setup *user/*,			struct netlink_ext_ack *extack*/)
{
	struct can_priv *priv = netdev_priv(dev);
	u32 tdc_mask = 0;
	int err;

#ifndef __QNX__
    /* We need synchronization with dev->stop() */
    ASSERT_RTNL();
#endif

    if (user->set_bittiming) {
        struct can_bittiming bt;

		/* Do not allow changing bittiming while running */
		if (dev->flags & IFF_UP)
			return -EBUSY;

		/* Calculate bittiming parameters based on
		 * bittiming_const if set, otherwise pass bitrate
		 * directly via do_set_bitrate(). Bail out if neither
		 * is given.
		 */
		if (!priv->bittiming_const && !priv->do_set_bittiming &&
		    !priv->bitrate_const)
			return -EOPNOTSUPP;

        memcpy(&bt, &user->bittiming, sizeof(bt));
		err = can_get_bittiming(dev, &bt,
					priv->bittiming_const,
					priv->bitrate_const,
					priv->bitrate_const_cnt);
		if (err)
			return err;

		if (priv->bitrate_max && bt.bitrate > priv->bitrate_max) {
			netdev_err(dev, "arbitration bitrate surpasses transceiver capabilities of %d bps\n",
				   priv->bitrate_max);
			return -EINVAL;
		}

		memcpy(&priv->bittiming, &bt, sizeof(bt));

		if (priv->do_set_bittiming) {
			/* Finally, set the bit-timing registers */
			err = priv->do_set_bittiming(dev);
			if (err)
				return err;
		}
	}

    if (user->set_ctrlmode) {
		struct can_ctrlmode *cm;
		u32 ctrlstatic;
		u32 maskedflags;

		/* Do not allow changing controller mode while running */
		if (dev->flags & IFF_UP)
			return -EBUSY;
        cm = &user->ctrlmode;
		ctrlstatic = can_get_static_ctrlmode(priv);
		maskedflags = cm->flags & cm->mask;

		/* check whether provided bits are allowed to be passed */
		if (maskedflags & ~(priv->ctrlmode_supported | ctrlstatic))
			return -EOPNOTSUPP;

		/* do not check for static fd-non-iso if 'fd' is disabled */
		if (!(maskedflags & CAN_CTRLMODE_FD))
			ctrlstatic &= ~CAN_CTRLMODE_FD_NON_ISO;

		/* make sure static options are provided by configuration */
		if ((maskedflags & ctrlstatic) != ctrlstatic)
			return -EOPNOTSUPP;

		/* clear bits to be modified and copy the flag values */
		priv->ctrlmode &= ~cm->mask;
		priv->ctrlmode |= maskedflags;

		/* CAN_CTRLMODE_FD can only be set when driver supports FD */
		if (priv->ctrlmode & CAN_CTRLMODE_FD) {
			dev->mtu = CANFD_MTU;
		} else {
			dev->mtu = CAN_MTU;
			memset(&priv->data_bittiming, 0,
			       sizeof(priv->data_bittiming));
			priv->ctrlmode &= ~CAN_CTRLMODE_TDC_MASK;
			memset(&priv->tdc, 0, sizeof(priv->tdc));
		}

		tdc_mask = cm->mask & CAN_CTRLMODE_TDC_MASK;
		/* CAN_CTRLMODE_TDC_{AUTO,MANUAL} are mutually
		 * exclusive: make sure to turn the other one off
		 */
		if (tdc_mask)
			priv->ctrlmode &= cm->flags | ~CAN_CTRLMODE_TDC_MASK;
	}

    if (user->set_restart_ms) {
		/* Do not allow changing restart delay while running */
		if (dev->flags & IFF_UP)
			return -EBUSY;
        priv->restart_ms = user->restart_ms;
	}

    if (user->set_restart) {
		/* Do not allow a restart while not running */
		if (!(dev->flags & IFF_UP))
			return -EINVAL;
		err = can_restart_now(dev);
		if (err)
			return err;
	}

    if (user->set_data_bittiming) {
		struct can_bittiming dbt;

		/* Do not allow changing bittiming while running */
		if (dev->flags & IFF_UP)
			return -EBUSY;

		/* Calculate bittiming parameters based on
		 * data_bittiming_const if set, otherwise pass bitrate
		 * directly via do_set_bitrate(). Bail out if neither
		 * is given.
		 */
		if (!priv->data_bittiming_const && !priv->do_set_data_bittiming &&
		    !priv->data_bitrate_const)
			return -EOPNOTSUPP;

        memcpy(&dbt, &user->data_bittiming, sizeof(dbt));
		err = can_get_bittiming(dev, &dbt,
					priv->data_bittiming_const,
					priv->data_bitrate_const,
					priv->data_bitrate_const_cnt);
		if (err)
			return err;

		if (priv->bitrate_max && dbt.bitrate > priv->bitrate_max) {
			netdev_err(dev, "canfd data bitrate surpasses transceiver capabilities of %d bps\n",
				   priv->bitrate_max);
			return -EINVAL;
		}

		memset(&priv->tdc, 0, sizeof(priv->tdc));
	    if (user->set_tdc) {
//			/* TDC parameters are provided: use them */
//			err = can_tdc_changelink(priv, user->tdc,
//						 extack);
//			if (err) {
//				priv->ctrlmode &= ~CAN_CTRLMODE_TDC_MASK;
//				return err;
//			}
		} else if (!tdc_mask) {
			/* Neither of TDC parameters nor TDC flags are
			 * provided: do calculation
			 */
			can_calc_tdco(&priv->tdc, priv->tdc_const, &priv->data_bittiming,
				      &priv->ctrlmode, priv->ctrlmode_supported);
		} /* else: both CAN_CTRLMODE_TDC_{AUTO,MANUAL} are explicitly
		   * turned off. TDC is disabled: do nothing
		   */

		memcpy(&priv->data_bittiming, &dbt, sizeof(dbt));

		if (priv->do_set_data_bittiming) {
			/* Finally, set the bit-timing registers */
			err = priv->do_set_data_bittiming(dev);
			if (err)
				return err;
		}
	}

    if (user->set_btr) {
        /* Special feature to force btr0 and btr1 to specific values needed for
         * some applications.
         */
		if (priv->do_set_btr) {
            u64 v64;
            u8 btr0 = user->can_btr.btr0;
            u8 btr1 = user->can_btr.btr1;
            u32 bitrate = user->bittiming.bitrate;

			/* Set the bit-timing registers */
			err = priv->do_set_btr(dev, btr0, btr1);
			if (err)
				return err;

            /* When special function is used to set btr0 and btr1 manually all
               bittiming data is extracted from these register values */
            priv->bittiming.bitrate = bitrate;
            priv->bittiming.brp = (btr0 & 0x3f) + 1;
            priv->bittiming.sjw = ((btr0 >> 6) & 0x3) + 1;
            priv->bittiming.phase_seg2 = ((btr1 >> 4) & 0x7) + 1;
            priv->bittiming.phase_seg1 = 0; // TODO: Not calculating phase_seg1
            // TODO: Once phase_seg1 is calculated prop_seg must be set to (btr1 & 0xf) + 1 - priv->bittiming.phase_seg1;
            priv->bittiming.prop_seg = 0;

            v64 = (u64)priv->bittiming.brp * 1000 * 1000 * 1000;
            do_div(v64, priv->clock.freq);

            priv->bittiming.tq = (u32)v64;

            /* Use CiA recommended sample points */
            if (bitrate > 800 * 1000 /* BPS */)
                priv->bittiming.sample_point = 750;
            else if (bitrate > 500 * 1000 /* BPS */)
                priv->bittiming.sample_point = 800;
            else
                priv->bittiming.sample_point = 875;

            if (btr1 & 0x80) {
                priv->ctrlmode |= CAN_CTRLMODE_3_SAMPLES;
            }
		}
    }

	if (user->set_termination) {
		const u16 termval = user->termination;
		const unsigned int num_term = priv->termination_const_cnt;
		unsigned int i;

		if (!priv->do_set_termination)
			return -EOPNOTSUPP;

		/* check whether given value is supported by the interface */
		for (i = 0; i < num_term; i++) {
			if (termval == priv->termination_const[i])
				break;
		}
		if (i >= num_term)
			return -EINVAL;

		/* Finally, set the termination value */
		err = priv->do_set_termination(dev, termval);
		if (err)
			return err;

		priv->termination = termval;
	}

	return 0;
}

static size_t can_get_xstats_size(const struct net_device *dev)
{
	return sizeof(struct can_device_stats);
}

static int can_fill_xstats(struct sk_buff *skb, const struct net_device *dev)
{
	return 0;
}

struct resmgr_ops can_link_ops = {
	.changelink	= can_changelink,
	.get_xstats_size = can_get_xstats_size,
	.fill_xstats	= can_fill_xstats,
};
