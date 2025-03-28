/*
 * \file    drivers/net/can/sja1000/sja1000.c
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Changes have been made to replace all spin_lock_irqsave functions
 *          with QNX InterruptLock functions. Removed the networking layer and
 *          replaced the memory allocation, error handling and logging, together
 *          with changes to some structure internals. For example net_device
 *          structure has been greatly simplified by removing networking
 *          specific functionality and retaining only CAN-bus critical features.
 *          PCI memory address types 'void __iomem *' have been changed to QNX
 *          equivalent type 'uintptr_t'.
 *          Added one custom function sja1000_set_btr() used to force btr0 and
 *          btr1 values.
 *
 * sja1000.c -  Philips SJA1000 network device driver
 *
 * Copyright (c) 2003 Matthias Brukner, Trajet Gmbh, Rebenring 33,
 * 38106 Braunschweig, GERMANY
 *
 * Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 * All rights reserved.
 *
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Volkswagen nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * Alternatively, provided that this notice is retained in full, this
 * software may be distributed under the terms of the GNU General
 * Public License ("GPL") version 2, in which case the provisions of the
 * GPL apply INSTEAD OF those given above.
 *
 * The provided data structures and external interfaces from this code
 * are not restricted to be used by modules with a GPL compatible license.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 */

#include <string.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/delay.h>

#include <linux/can/dev.h>

#include "sja1000.h"

#define DRV_NAME "sja1000"

MODULE_AUTHOR("Oliver Hartkopp <oliver.hartkopp@volkswagen.de>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION(DRV_NAME "CAN netdevice driver");

static const struct can_bittiming_const sja1000_bittiming_const = {
	.name = DRV_NAME,
	.tseg1_min = 1,
	.tseg1_max = 16,
	.tseg2_min = 1,
	.tseg2_max = 8,
	.sjw_max = 4,
	.brp_min = 1,
	.brp_max = 64,
	.brp_inc = 1,
};

static void sja1000_write_cmdreg(struct sja1000_priv *priv, u8 val)
{
#ifndef __QNX__
	unsigned long flags;
#endif

	/*
	 * The command register needs some locking and time to settle
	 * the write_reg() operation - especially on SMP systems.
	 */
#ifndef __QNX__
	spin_lock_irqsave(&priv->cmdreg_lock, flags);
#endif
	priv->write_reg(priv, SJA1000_CMR, val);
	priv->read_reg(priv, SJA1000_SR);
#ifndef __QNX__
	spin_unlock_irqrestore(&priv->cmdreg_lock, flags);
#endif
}

static int sja1000_is_absent(struct sja1000_priv *priv)
{
	return (priv->read_reg(priv, SJA1000_MOD) == 0xFF);
}

static int sja1000_probe_chip(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);

	if (priv->reg_base && sja1000_is_absent(priv)) {
		netdev_err(dev, "probing failed\n");
		return 0;
	}
	return -1;
}

static void set_reset_mode(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	unsigned char status = priv->read_reg(priv, SJA1000_MOD);
	int i;

	/* disable interrupts */
	priv->write_reg(priv, SJA1000_IER, IRQ_OFF);

	for (i = 0; i < 100; i++) {
		/* check reset bit */
		if (status & MOD_RM) {
			priv->can.state = CAN_STATE_STOPPED;
			return;
		}

		/* reset chip */
		priv->write_reg(priv, SJA1000_MOD, MOD_RM);
		udelay(10);
		status = priv->read_reg(priv, SJA1000_MOD);
	}

	netdev_err(dev, "setting SJA1000 into reset mode failed!\n");
}

static void set_normal_mode(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	unsigned char status = priv->read_reg(priv, SJA1000_MOD);
	u8 mod_reg_val = 0x00;
	int i;

	for (i = 0; i < 100; i++) {
		/* check reset bit */
		if ((status & MOD_RM) == 0) {
			priv->can.state = CAN_STATE_ERROR_ACTIVE;
			/* enable interrupts */
			if (priv->can.ctrlmode & CAN_CTRLMODE_BERR_REPORTING)
				priv->write_reg(priv, SJA1000_IER, IRQ_ALL);
			else
				priv->write_reg(priv, SJA1000_IER,
						IRQ_ALL & ~IRQ_BEI);
			return;
		}

		/* set chip to normal mode */
		if (priv->can.ctrlmode & CAN_CTRLMODE_LISTENONLY)
			mod_reg_val |= MOD_LOM;
		if (priv->can.ctrlmode & CAN_CTRLMODE_PRESUME_ACK)
			mod_reg_val |= MOD_STM;
		priv->write_reg(priv, SJA1000_MOD, mod_reg_val);

		udelay(10);

		status = priv->read_reg(priv, SJA1000_MOD);
	}

	netdev_err(dev, "setting SJA1000 into normal mode failed!\n");
}

/*
 * initialize SJA1000 chip:
 *   - reset chip
 *   - set output mode
 *   - set baudrate
 *   - enable interrupts
 *   - start operating mode
 */
static void chipset_init(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);

	if (!(priv->flags & SJA1000_QUIRK_NO_CDR_REG))
		/* set clock divider and output control register */
		priv->write_reg(priv, SJA1000_CDR, priv->cdr | CDR_PELICAN);

	/* set acceptance filter (accept all) */
	priv->write_reg(priv, SJA1000_ACCC0, 0x00);
	priv->write_reg(priv, SJA1000_ACCC1, 0x00);
	priv->write_reg(priv, SJA1000_ACCC2, 0x00);
	priv->write_reg(priv, SJA1000_ACCC3, 0x00);

	priv->write_reg(priv, SJA1000_ACCM0, 0xFF);
	priv->write_reg(priv, SJA1000_ACCM1, 0xFF);
	priv->write_reg(priv, SJA1000_ACCM2, 0xFF);
	priv->write_reg(priv, SJA1000_ACCM3, 0xFF);

	priv->write_reg(priv, SJA1000_OCR, priv->ocr | OCR_MODE_NORMAL);
}

static void sja1000_start(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);

	/* enter reset mode */
	if (priv->can.state != CAN_STATE_STOPPED)
		set_reset_mode(dev);

	/* Initialize chip if uninitialized at this stage */
	if (!(priv->flags & SJA1000_QUIRK_NO_CDR_REG ||
	      priv->read_reg(priv, SJA1000_CDR) & CDR_PELICAN))
		chipset_init(dev);

	/* Clear error counters and error code capture */
	priv->write_reg(priv, SJA1000_TXERR, 0x0);
	priv->write_reg(priv, SJA1000_RXERR, 0x0);
	priv->read_reg(priv, SJA1000_ECC);

	/* clear interrupt flags */
	priv->read_reg(priv, SJA1000_IR);

	/* leave reset mode */
	set_normal_mode(dev);
}

static int sja1000_set_mode(struct net_device *dev, enum can_mode mode)
{
	switch (mode) {
	case CAN_MODE_START:
		sja1000_start(dev);
		if (netif_queue_stopped(dev))
			netif_wake_queue(dev);
		break;

	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int sja1000_set_bittiming(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	struct can_bittiming *bt = &priv->can.bittiming;
	u8 btr0, btr1;

	btr0 = ((bt->brp - 1) & 0x3f) | (((bt->sjw - 1) & 0x3) << 6);
	btr1 = ((bt->prop_seg + bt->phase_seg1 - 1) & 0xf) |
		(((bt->phase_seg2 - 1) & 0x7) << 4);
	if (priv->can.ctrlmode & CAN_CTRLMODE_3_SAMPLES)
		btr1 |= 0x80;

	netdev_info(dev, "setting BTR0=0x%02x BTR1=0x%02x\n", btr0, btr1);

	priv->write_reg(priv, SJA1000_BTR0, btr0);
	priv->write_reg(priv, SJA1000_BTR1, btr1);

	return 0;
}

/*
 * Special feature to force btr0 and btr1 to specific values needed for
 * some applications.
 */
static int sja1000_set_btr(struct net_device *dev, u8 btr0, u8 btr1)
{
	struct sja1000_priv *priv = netdev_priv(dev);

	netdev_info(dev, "setting BTR0=0x%02x BTR1=0x%02x\n", btr0, btr1);

	priv->write_reg(priv, SJA1000_BTR0, btr0);
	priv->write_reg(priv, SJA1000_BTR1, btr1);

	return 0;
}

static int sja1000_get_berr_counter(const struct net_device *dev,
				    struct can_berr_counter *bec)
{
	struct sja1000_priv *priv = netdev_priv(dev);

	bec->txerr = priv->read_reg(priv, SJA1000_TXERR);
	bec->rxerr = priv->read_reg(priv, SJA1000_RXERR);

	return 0;
}

/*
 * transmit a CAN message
 * message layout in the sk_buff should be like this:
 * xx xx xx xx	 ff	 ll   00 11 22 33 44 55 66 77
 * [  can-id ] [flags] [len] [can data (up to 8 bytes]
 */
static netdev_tx_t sja1000_start_xmit(struct sk_buff *skb,
					    struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	struct can_frame *cf = (struct can_frame *)skb->data;
	uint8_t fi;
	canid_t id;
	uint8_t dreg;
	u8 cmd_reg_val = 0x00;
	int i;

	if (can_dev_dropped_skb(dev, skb))
		return NETDEV_TX_OK;

	netif_stop_queue(dev);

	fi = can_get_cc_dlc(cf, priv->can.ctrlmode);
	id = cf->can_id;

	if (id & CAN_RTR_FLAG)
		fi |= SJA1000_FI_RTR;

	if (id & CAN_EFF_FLAG) {
		fi |= SJA1000_FI_FF;
		dreg = SJA1000_EFF_BUF;
		priv->write_reg(priv, SJA1000_FI, fi);
		priv->write_reg(priv, SJA1000_ID1, (id & 0x1fe00000) >> 21);
		priv->write_reg(priv, SJA1000_ID2, (id & 0x001fe000) >> 13);
		priv->write_reg(priv, SJA1000_ID3, (id & 0x00001fe0) >> 5);
		priv->write_reg(priv, SJA1000_ID4, (id & 0x0000001f) << 3);
	} else {
		dreg = SJA1000_SFF_BUF;
		priv->write_reg(priv, SJA1000_FI, fi);
		priv->write_reg(priv, SJA1000_ID1, (id & 0x000007f8) >> 3);
		priv->write_reg(priv, SJA1000_ID2, (id & 0x00000007) << 5);
	}

	for (i = 0; i < cf->len; i++)
		priv->write_reg(priv, dreg++, cf->data[i]);

	can_put_echo_skb(skb, dev, 0, 0);

	if (priv->can.ctrlmode & CAN_CTRLMODE_ONE_SHOT)
		cmd_reg_val |= CMD_AT;

	if (priv->can.ctrlmode & CAN_CTRLMODE_LOOPBACK)
		cmd_reg_val |= CMD_SRR;
	else
		cmd_reg_val |= CMD_TR;

	sja1000_write_cmdreg(priv, cmd_reg_val);

	return NETDEV_TX_OK;
}

static void sja1000_rx(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	uint8_t fi;
	uint8_t dreg;
	canid_t id;
	int i;

	/* create zero'ed CAN frame buffer */
	skb = alloc_can_skb(dev, &cf);
	if (skb == NULL)
		return;

	fi = priv->read_reg(priv, SJA1000_FI);

	if (fi & SJA1000_FI_FF) {
		/* extended frame format (EFF) */
		dreg = SJA1000_EFF_BUF;
		id = (priv->read_reg(priv, SJA1000_ID1) << 21)
		    | (priv->read_reg(priv, SJA1000_ID2) << 13)
		    | (priv->read_reg(priv, SJA1000_ID3) << 5)
		    | (priv->read_reg(priv, SJA1000_ID4) >> 3);
		id |= CAN_EFF_FLAG;
	} else {
		/* standard frame format (SFF) */
		dreg = SJA1000_SFF_BUF;
		id = (priv->read_reg(priv, SJA1000_ID1) << 3)
		    | (priv->read_reg(priv, SJA1000_ID2) >> 5);
	}

	can_frame_set_cc_len(cf, fi & 0x0F, priv->can.ctrlmode);
	if (fi & SJA1000_FI_RTR) {
		id |= CAN_RTR_FLAG;
	} else {
		for (i = 0; i < cf->len; i++)
			cf->data[i] = priv->read_reg(priv, dreg++);

		stats->rx_bytes += cf->len;
	}
	stats->rx_packets++;

	cf->can_id = id;

	/* release receive buffer */
	sja1000_write_cmdreg(priv, CMD_RRB);

	netif_rx(skb);
}

static irqreturn_t sja1000_reset_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;

	netdev_dbg(dev, "performing a soft reset upon overrun\n");

	netif_tx_lock(dev);

	can_free_echo_skb(dev, 0, NULL);
	sja1000_set_mode(dev, CAN_MODE_START);

	netif_tx_unlock(dev);

	return IRQ_HANDLED;
}

static int sja1000_err(struct net_device *dev, uint8_t isrc, uint8_t status)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	struct can_frame *cf;
	struct sk_buff *skb;
	enum can_state state = priv->can.state;
	enum can_state rx_state, tx_state;
	unsigned int rxerr, txerr;
	uint8_t ecc, alc;
	int ret = 0;

	skb = alloc_can_err_skb(dev, &cf);

	txerr = priv->read_reg(priv, SJA1000_TXERR);
	rxerr = priv->read_reg(priv, SJA1000_RXERR);

	if (isrc & IRQ_DOI) {
		/* data overrun interrupt */
		netdev_dbg(dev, "data overrun interrupt\n");
		if (skb) {
			cf->can_id |= CAN_ERR_CRTL;
			cf->data[1] = CAN_ERR_CRTL_RX_OVERFLOW;
		}

		stats->rx_over_errors++;
		stats->rx_errors++;
		sja1000_write_cmdreg(priv, CMD_CDO);	/* clear bit */

		/* Some controllers needs additional handling upon overrun
		 * condition: the controller may sometimes be totally confused
		 * and refuse any new frame while its buffer is empty. The only
		 * way to re-sync the read vs. write buffer offsets is to
		 * stop any current handling and perform a reset.
		 */
		if (priv->flags & SJA1000_QUIRK_RESET_ON_OVERRUN)
			ret = IRQ_WAKE_THREAD;
	}

	if (isrc & IRQ_EI) {
		/* error warning interrupt */
		netdev_dbg(dev, "error warning interrupt\n");

		if (status & SR_BS)
			state = CAN_STATE_BUS_OFF;
		else if (status & SR_ES)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_ACTIVE;
	}
	if (state != CAN_STATE_BUS_OFF && skb) {
		cf->can_id |= CAN_ERR_CNT;
		cf->data[6] = txerr;
		cf->data[7] = rxerr;
	}
	if (isrc & IRQ_BEI) {
		/* bus error interrupt */
		priv->can.can_stats.bus_error++;

		ecc = priv->read_reg(priv, SJA1000_ECC);
		if (skb) {
			cf->can_id |= CAN_ERR_PROT | CAN_ERR_BUSERROR;

			/* set error type */
			switch (ecc & ECC_MASK) {
			case ECC_BIT:
				cf->data[2] |= CAN_ERR_PROT_BIT;
				break;
			case ECC_FORM:
				cf->data[2] |= CAN_ERR_PROT_FORM;
				break;
			case ECC_STUFF:
				cf->data[2] |= CAN_ERR_PROT_STUFF;
				break;
			default:
				break;
			}

			/* set error location */
			cf->data[3] = ecc & ECC_SEG;
		}

		/* Error occurred during transmission? */
		if ((ecc & ECC_DIR) == 0) {
			stats->tx_errors++;
			if (skb)
				cf->data[2] |= CAN_ERR_PROT_TX;
		} else {
			stats->rx_errors++;
		}
	}
	if (isrc & IRQ_EPI) {
		/* error passive interrupt */
		netdev_dbg(dev, "error passive interrupt\n");

		if (state == CAN_STATE_ERROR_PASSIVE)
			state = CAN_STATE_ERROR_WARNING;
		else
			state = CAN_STATE_ERROR_PASSIVE;
	}
	if (isrc & IRQ_ALI) {
		/* arbitration lost interrupt */
		netdev_dbg(dev, "arbitration lost interrupt\n");
		alc = priv->read_reg(priv, SJA1000_ALC);
		priv->can.can_stats.arbitration_lost++;
		if (skb) {
			cf->can_id |= CAN_ERR_LOSTARB;
			cf->data[0] = alc & 0x1f;
		}
	}

	if (state != priv->can.state) {
		tx_state = txerr >= rxerr ? state : 0;
		rx_state = txerr <= rxerr ? state : 0;

		can_change_state(dev, cf, tx_state, rx_state);

		if(state == CAN_STATE_BUS_OFF)
			can_bus_off(dev);
	}

	if (!skb)
		return -ENOMEM;

	netif_rx(skb);

	return ret;
}

irqreturn_t sja1000_interrupt(int irq, void *dev_id)
{
	struct net_device *dev = (struct net_device *)dev_id;
	struct sja1000_priv *priv = netdev_priv(dev);
	struct net_device_stats *stats = &dev->stats;
	uint8_t isrc, status;
	irqreturn_t ret = 0;
	int n = 0, err;

	if (priv->pre_irq)
		priv->pre_irq(priv);

	/* Shared interrupts and IRQ off? */
	if (priv->read_reg(priv, SJA1000_IER) == IRQ_OFF)
		goto out;

	while ((isrc = priv->read_reg(priv, SJA1000_IR)) &&
	       (n < SJA1000_MAX_IRQ)) {

		status = priv->read_reg(priv, SJA1000_SR);
		/* check for absent controller due to hw unplug */
		if (status == 0xFF && sja1000_is_absent(priv))
			goto out;

		if (isrc & IRQ_WUI)
			netdev_warn(dev, "wakeup interrupt\n");

		if (isrc & IRQ_TI) {
			/* transmission buffer released */
			if (priv->can.ctrlmode & CAN_CTRLMODE_ONE_SHOT &&
			    !(status & SR_TCS)) {
				stats->tx_errors++;
				can_free_echo_skb(dev, 0, NULL);
			} else {
				/* transmission complete */
				stats->tx_bytes += can_get_echo_skb(dev, 0, NULL);
				stats->tx_packets++;
			}
			netif_wake_queue(dev);
		}
		if (isrc & IRQ_RI) {
			/* receive interrupt */
			while (status & SR_RBS) {
				sja1000_rx(dev);
				status = priv->read_reg(priv, SJA1000_SR);
				/* check for absent controller */
				if (status == 0xFF && sja1000_is_absent(priv))
					goto out;
			}
		}
		if (isrc & (IRQ_DOI | IRQ_EI | IRQ_BEI | IRQ_EPI | IRQ_ALI)) {
			/* error interrupt */
			err = sja1000_err(dev, isrc, status);
			if (err == IRQ_WAKE_THREAD)
				ret = err;
			if (err)
				break;
		}
		n++;
	}
out:
	if (!ret)
		ret = (n) ? IRQ_HANDLED : IRQ_NONE;

	if (priv->post_irq)
		priv->post_irq(priv);

	if (n >= SJA1000_MAX_IRQ)
		netdev_dbg(dev, "%d messages handled in ISR", n);

	return ret;
}
EXPORT_SYMBOL_GPL(sja1000_interrupt);

static int sja1000_open(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);
	int err;

	/* set chip into reset mode */
	set_reset_mode(dev);

	/* common open */
	err = open_candev(dev);
	if (err)
		return err;

	/* register interrupt handler, if not done by the device driver */
	if (!(priv->flags & SJA1000_CUSTOM_IRQ_HANDLER)) {
		err = request_threaded_irq(dev->irq, sja1000_interrupt,
					   sja1000_reset_interrupt,
					   priv->irq_flags, dev->name, (void *)dev);
		if (err) {
			close_candev(dev);
			return -EAGAIN;
		}
	}

	/* init and start chi */
	sja1000_start(dev);

	netif_start_queue(dev);

	return 0;
}

static int sja1000_close(struct net_device *dev)
{
	struct sja1000_priv *priv = netdev_priv(dev);

	netif_stop_queue(dev);
	set_reset_mode(dev);

	if (!(priv->flags & SJA1000_CUSTOM_IRQ_HANDLER))
		free_irq(dev->irq, (void *)dev);

	close_candev(dev);

	return 0;
}

struct net_device *alloc_sja1000dev(int sizeof_priv)
{
	struct net_device *dev;
	struct sja1000_priv *priv;

	dev = alloc_candev(sizeof(struct sja1000_priv),
		SJA1000_ECHO_SKB_MAX);
	if (!dev)
		return NULL;

	priv = netdev_priv(dev);

	priv->dev = dev;
	priv->can.bittiming_const = &sja1000_bittiming_const;
	priv->can.do_set_bittiming = sja1000_set_bittiming;
    priv->can.do_set_btr = sja1000_set_btr; /* Special feature to force btr0 and
                                             * btr1 to specific values needed
                                             * for some applications. */
	priv->can.do_set_mode = sja1000_set_mode;
	priv->can.do_get_berr_counter = sja1000_get_berr_counter;
	priv->can.ctrlmode_supported = CAN_CTRLMODE_LOOPBACK |
				       CAN_CTRLMODE_LISTENONLY |
				       CAN_CTRLMODE_3_SAMPLES |
				       CAN_CTRLMODE_ONE_SHOT |
				       CAN_CTRLMODE_BERR_REPORTING |
				       CAN_CTRLMODE_PRESUME_ACK |
				       CAN_CTRLMODE_CC_LEN8_DLC;

#ifndef __QNX__
	spin_lock_init(&priv->cmdreg_lock);
#endif

	if (sizeof_priv)
		priv->priv = (void *)malloc(sizeof_priv);

	return dev;
}
EXPORT_SYMBOL_GPL(alloc_sja1000dev);

void free_sja1000dev(struct net_device *dev)
{
	free_candev(dev);
}
EXPORT_SYMBOL_GPL(free_sja1000dev);

static const struct net_device_ops sja1000_netdev_ops = {
	.ndo_open	= sja1000_open,
	.ndo_stop	= sja1000_close,
	.ndo_start_xmit	= sja1000_start_xmit,
	.ndo_change_mtu	= can_change_mtu,
};

int register_sja1000dev(struct net_device *dev)
{
	if (!sja1000_probe_chip(dev))
		return -ENODEV;

	dev->flags |= IFF_ECHO;	/* we support local echo */
	dev->netdev_ops = &sja1000_netdev_ops;

	set_reset_mode(dev);
	chipset_init(dev);

	return register_candev(dev);
}
EXPORT_SYMBOL_GPL(register_sja1000dev);

void unregister_sja1000dev(struct net_device *dev)
{
	set_reset_mode(dev);
	unregister_candev(dev);
}
EXPORT_SYMBOL_GPL(unregister_sja1000dev);

