/*
 * sja1000.h -  Philips SJA1000 network device driver
 *
 * Copyright (c) 2003 Matthias Brukner, Trajet Gmbh, Rebenring 33,
 * 38106 Braunschweig, GERMANY
 *
 * Copyright (c) 2002-2007 Volkswagen Group Electronic Research
 * All rights reserved.
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

#ifndef SJA1000_DEV_H
#define SJA1000_DEV_H

#include <syslog.h>
#include <sys/neutrino.h>
#include "linux/irqreturn.h"
#include "linux/can/dev.h"
#include "linux/can/platform/sja1000.h"
#include "uapi/linux/can.h"
#include "uapi/linux/can/error.h"

// New DE
enum can_led_event {
    CAN_LED_EVENT_OPEN,
    CAN_LED_EVENT_STOP,
    CAN_LED_EVENT_TX,
    CAN_LED_EVENT_RX,
};

#define CAN_CTRLMODE_LOOPBACK       0x01    /* Loopback mode */
#define CAN_CTRLMODE_LISTENONLY     0x02    /* Listen-only mode */
#define CAN_CTRLMODE_3_SAMPLES      0x04    /* Triple sampling mode */
#define CAN_CTRLMODE_ONE_SHOT       0x08    /* One-Shot mode */
#define CAN_CTRLMODE_BERR_REPORTING 0x10    /* Bus-error reporting */
#define CAN_CTRLMODE_FD         0x20    /* CAN FD mode */
#define CAN_CTRLMODE_PRESUME_ACK    0x40    /* Ignore missing CAN ACKs */
#define CAN_CTRLMODE_FD_NON_ISO     0x80    /* CAN FD in non-ISO mode */

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */
#define min_t(type, x, y) ({            \
    type __min1 = (x);          \
    type __min2 = (y);          \
    __min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({            \
    type __max1 = (x);          \
    type __max2 = (y);          \
    __max1 > __max2 ? __max1: __max2; })

struct sja1000_priv;

//sja1000_priv* netdev_priv (struct net_device * dev) {
//	return (sja1000_priv*)dev->priv;
//}
//
//const sja1000_priv* netdev_priv (const struct net_device * dev) {
//	return (const sja1000_priv*)dev->priv;
//}

// NETIF interface
// New DE


#define SJA1000_ECHO_SKB_MAX	1 /* the SJA1000 has one TX buffer object */

#define SJA1000_MAX_IRQ 20	/* max. number of interrupts handled in ISR */

/* SJA1000 registers - manual section 6.4 (Pelican Mode) */
#define SJA1000_MOD		0x00
#define SJA1000_CMR		0x01
#define SJA1000_SR		0x02
#define SJA1000_IR		0x03
#define SJA1000_IER		0x04
#define SJA1000_ALC		0x0B
#define SJA1000_ECC		0x0C
#define SJA1000_EWL		0x0D
#define SJA1000_RXERR		0x0E
#define SJA1000_TXERR		0x0F
#define SJA1000_ACCC0		0x10
#define SJA1000_ACCC1		0x11
#define SJA1000_ACCC2		0x12
#define SJA1000_ACCC3		0x13
#define SJA1000_ACCM0		0x14
#define SJA1000_ACCM1		0x15
#define SJA1000_ACCM2		0x16
#define SJA1000_ACCM3		0x17
#define SJA1000_RMC		0x1D
#define SJA1000_RBSA		0x1E

/* Common registers - manual section 6.5 */
#define SJA1000_BTR0		0x06
#define SJA1000_BTR1		0x07
#define SJA1000_OCR		0x08
#define SJA1000_CDR		0x1F

#define SJA1000_FI		0x10
#define SJA1000_SFF_BUF		0x13
#define SJA1000_EFF_BUF		0x15

#define SJA1000_FI_FF		0x80
#define SJA1000_FI_RTR		0x40

#define SJA1000_ID1		0x11
#define SJA1000_ID2		0x12
#define SJA1000_ID3		0x13
#define SJA1000_ID4		0x14

#define SJA1000_CAN_RAM		0x20

/* mode register */
#define MOD_RM		0x01
#define MOD_LOM		0x02
#define MOD_STM		0x04
#define MOD_AFM		0x08
#define MOD_SM		0x10

/* commands */
#define CMD_SRR		0x10
#define CMD_CDO		0x08
#define CMD_RRB		0x04
#define CMD_AT		0x02
#define CMD_TR		0x01

/* interrupt sources */
#define IRQ_BEI		0x80
#define IRQ_ALI		0x40
#define IRQ_EPI		0x20
#define IRQ_WUI		0x10
#define IRQ_DOI		0x08
#define IRQ_EI		0x04
#define IRQ_TI		0x02
#define IRQ_RI		0x01
#define IRQ_ALL		0xFF
#define IRQ_OFF		0x00

/* status register content */
#define SR_BS		0x80
#define SR_ES		0x40
#define SR_TS		0x20
#define SR_RS		0x10
#define SR_TCS		0x08
#define SR_TBS		0x04
#define SR_DOS		0x02
#define SR_RBS		0x01

#define SR_CRIT (SR_BS|SR_ES)

/* ECC register */
#define ECC_SEG		0x1F
#define ECC_DIR		0x20
#define ECC_ERR		6
#define ECC_BIT		0x00
#define ECC_FORM	0x40
#define ECC_STUFF	0x80
#define ECC_MASK	0xc0

/*
 * Flags for sja1000priv.flags
 */
#define SJA1000_CUSTOM_IRQ_HANDLER 0x1

/*
 * SJA1000 private data structure
 */
struct sja1000_priv {
	struct can_priv can;	/* must be the first member */
	struct sk_buff *echo_skb;

	/* the lower-layer is responsible for appropriate locking */
	u8 (*read_reg) (const struct sja1000_priv *priv, int reg);
	void (*write_reg) (const struct sja1000_priv *priv, int reg, u8 val);
	void (*pre_irq) (const struct sja1000_priv *priv);
	void (*post_irq) (const struct sja1000_priv *priv);

	void *priv;		/* for board-specific data */
	struct net_device *dev;

//	void __iomem *reg_base;	 /* ioremap'ed address to registers */
	uintptr_t reg_base;	 /* ioremap'ed address to registers */
	unsigned long irq_flags; /* for request_irq() */
//	spinlock_t cmdreg_lock;  /* lock for concurrent cmd register writes */
	intrspin_t cmdreg_lock;  /* lock for concurrent cmd register writes */

	u16 flags;		/* custom mode flags */
	u8 ocr;			/* output control register */
	u8 cdr;			/* clock divider register */
};

struct net_device *alloc_sja1000dev(int sizeof_priv);
void free_sja1000dev(struct net_device *dev);
int register_sja1000dev(struct net_device *dev);
void unregister_sja1000dev(struct net_device *dev);

irqreturn_t sja1000_interrupt(int irq, void *dev_id);

// New DE

/*
 * Free space of the CAN network device
 */

#define __UAPI_DEF_IF_NET_DEVICE_FLAGS
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO

// New DE

#endif /* SJA1000_DEV_H */
