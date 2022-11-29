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

#include <new>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/neutrino.h>
#include "types.h"
#include "linux/irqreturn.h"
//#include <linux/can/dev.h>
#include "linux/can/platform/sja1000.h"
#include "uapi/linux/can.h"
#include "uapi/linux/can/error.h"

// New DE
/*
 * CAN device statistics
 */
struct can_device_stats {
    u32 bus_error;    /* Bus errors */
    u32 error_warning;    /* Changes to error warning state */
    u32 error_passive;    /* Changes to error passive state */
    u32 bus_off;      /* Changes to bus off state */
    u32 arbitration_lost; /* Arbitration lost errors */
    u32 restarts;     /* CAN controller re-starts */
};

/*
 * CAN operational and error states
 */
enum can_state {
    CAN_STATE_ERROR_ACTIVE = 0, /* RX/TX error count < 96 */
    CAN_STATE_ERROR_WARNING,    /* RX/TX error count < 128 */
    CAN_STATE_ERROR_PASSIVE,    /* RX/TX error count < 256 */
    CAN_STATE_BUS_OFF,      /* RX/TX error count >= 256 */
    CAN_STATE_STOPPED,      /* Device is stopped */
    CAN_STATE_SLEEPING,     /* Device is sleeping */
    CAN_STATE_MAX
};

/*
 * CAN mode
 */
enum can_mode {
    CAN_MODE_STOP = 0,
    CAN_MODE_START,
    CAN_MODE_SLEEP
};

/*
 * CAN bit-timing parameters
 *
 * For further information, please read chapter "8 BIT TIMING
 * REQUIREMENTS" of the "Bosch CAN Specification version 2.0"
 * at http://www.semiconductors.bosch.de/pdf/can2spec.pdf.
 */
struct can_bittiming {
    u32 bitrate;      /* Bit-rate in bits/second */
    u32 sample_point; /* Sample point in one-tenth of a percent */
    u32 tq;       /* Time quanta (TQ) in nanoseconds */
    u32 prop_seg;     /* Propagation segment in TQs */
    u32 phase_seg1;   /* Phase buffer segment 1 in TQs */
    u32 phase_seg2;   /* Phase buffer segment 2 in TQs */
    u32 sjw;      /* Synchronisation jump width in TQs */
    u32 brp;      /* Bit-rate prescaler */
};

/*
 * CAN harware-dependent bit-timing constant
 *
 * Used for calculating and checking bit-timing parameters
 */
struct can_bittiming_const {
    char name[16];      /* Name of the CAN controller hardware */
    u32 tseg1_min;    /* Time segement 1 = prop_seg + phase_seg1 */
    u32 tseg1_max;
    u32 tseg2_min;    /* Time segement 2 = phase_seg2 */
    u32 tseg2_max;
    u32 sjw_max;      /* Synchronisation jump width */
    u32 brp_min;      /* Bit-rate prescaler */
    u32 brp_max;
    u32 brp_inc;
};

/*
 * CAN bus error counters
 */
struct can_berr_counter {
    u16 txerr;
    u16 rxerr;
};

enum can_led_event {
    CAN_LED_EVENT_OPEN,
    CAN_LED_EVENT_STOP,
    CAN_LED_EVENT_TX,
    CAN_LED_EVENT_RX,
};

enum netdev_tx {
    __NETDEV_TX_MIN  = INT_MIN, /* make sure enum is signed */
    NETDEV_TX_OK     = 0x00,    /* driver took care of packet */
    NETDEV_TX_BUSY   = 0x10,    /* driver tx path was busy*/
    NETDEV_TX_LOCKED = 0x20,    /* driver tx lock was already taken */
};
typedef enum netdev_tx netdev_tx_t;

#define CAN_CTRLMODE_LOOPBACK       0x01    /* Loopback mode */
#define CAN_CTRLMODE_LISTENONLY     0x02    /* Listen-only mode */
#define CAN_CTRLMODE_3_SAMPLES      0x04    /* Triple sampling mode */
#define CAN_CTRLMODE_ONE_SHOT       0x08    /* One-Shot mode */
#define CAN_CTRLMODE_BERR_REPORTING 0x10    /* Bus-error reporting */
#define CAN_CTRLMODE_FD         0x20    /* CAN FD mode */
#define CAN_CTRLMODE_PRESUME_ACK    0x40    /* Ignore missing CAN ACKs */
#define CAN_CTRLMODE_FD_NON_ISO     0x80    /* CAN FD in non-ISO mode */

/*
 *  Old network device statistics. Fields are native words
 *  (unsigned long) so they can be read and written atomically.
 */

struct net_device_stats {
    unsigned long   rx_packets;
    unsigned long   tx_packets;
    unsigned long   rx_bytes;
    unsigned long   tx_bytes;
    unsigned long   rx_errors;
    unsigned long   tx_errors;
    unsigned long   rx_dropped;
    unsigned long   tx_dropped;
    unsigned long   multicast;
    unsigned long   collisions;
    unsigned long   rx_length_errors;
    unsigned long   rx_over_errors;
    unsigned long   rx_crc_errors;
    unsigned long   rx_frame_errors;
    unsigned long   rx_fifo_errors;
    unsigned long   rx_missed_errors;
    unsigned long   tx_aborted_errors;
    unsigned long   tx_carrier_errors;
    unsigned long   tx_fifo_errors;
    unsigned long   tx_heartbeat_errors;
    unsigned long   tx_window_errors;
    unsigned long   rx_compressed;
    unsigned long   tx_compressed;
};

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

/*
 * get_can_dlc(value) - helper macro to cast a given data length code (dlc)
 * to __u8 and ensure the dlc value to be max. 8 bytes.
 *
 * To be used in the CAN netdriver receive path to ensure conformance with
 * ISO 11898-1 Chapter 8.4.2.3 (DLC field)
 */
#define get_can_dlc(i)      (min_t(u8, (i), CAN_MAX_DLC))
#define get_canfd_dlc(i)    (min_t(u8, (i), CANFD_MAX_DLC))

struct can_priv {
    struct can_device_stats can_stats;
    struct can_bittiming bittiming;
    const struct can_bittiming_const *bittiming_const;

    enum can_state state;

    /* CAN controller features - see include/uapi/linux/can/netlink.h */
    u32 ctrlmode;       /* current options setting */
    u32 ctrlmode_supported; /* options that can be modified by netlink */
    u32 ctrlmode_static;    /* static enabled options for driver/hardware */

    int (*do_set_bittiming)(struct net_device *dev);
    int (*do_set_mode)(struct net_device *dev, enum can_mode mode);
    int (*do_get_berr_counter)(struct net_device *dev,
               	   struct can_berr_counter *bec);
};

struct net_device_ops {
	int         (*ndo_open)(struct net_device *dev);
	int         (*ndo_stop)(struct net_device *dev);
    netdev_tx_t	(*ndo_start_xmit) (struct sk_buff *skb,
                      	  struct net_device *dev);
    int         (*ndo_change_mtu)(struct net_device *dev,
                          int new_mtu);
};

#define IFNAMSIZ    16

struct net_device {
    char      	name[IFNAMSIZ];
    int         irq;
	struct net_device_stats stats;
    unsigned int        flags;
    unsigned int        mtu;
    const struct net_device_ops *netdev_ops;
	void* priv;
};

struct sja1000_priv;

sja1000_priv* netdev_priv (struct net_device * dev) {
	return (sja1000_priv*)dev->priv;
}

struct sk_buff {
	unsigned char* data;
};

sk_buff *alloc_can_skb(struct net_device *dev, struct can_frame **cf)
{
    sk_buff *skb = new sk_buff;

//    skb->protocol = htons(ETH_P_CAN);
//    skb->pkt_type = PACKET_BROADCAST;
//    skb->ip_summed = CHECKSUM_UNNECESSARY;

//    skb_reset_mac_header(skb);
//    skb_reset_network_header(skb);
//    skb_reset_transport_header(skb);

//    can_skb_reserve(skb);
//    can_skb_prv(skb)->ifindex = dev->ifindex;
//    can_skb_prv(skb)->skbcnt = 0;

    *cf = new (std::nothrow) can_frame;
    memset(*cf, 0, sizeof(can_frame));

    skb->data = (unsigned char*)(*cf);

    return skb;
}

struct sk_buff *alloc_can_err_skb(struct net_device *dev, struct can_frame **cf)
{
    sk_buff *skb;

    skb = alloc_can_skb(dev, cf);
	if (skb == NULL)
        return NULL;

    (*cf)->can_id = CAN_ERR_FLAG;
    (*cf)->can_dlc = CAN_ERR_DLC;

    return skb;
}

/*
 * Common open function when the device gets opened.
 *
 * This function should be called in the open function of the device
 * driver.
 */
int open_candev(struct net_device *dev)
{
//    struct can_priv *priv = netdev_priv(dev);
//
//    if (!priv->bittiming.bitrate) {
//        netdev_err(dev, "bit-timing not yet defined\n");
//        return -EINVAL;
//    }
//
//    /* For CAN FD the data bitrate has to be >= the arbitration bitrate */
//    if ((priv->ctrlmode & CAN_CTRLMODE_FD) &&
//        (!priv->data_bittiming.bitrate ||
//         (priv->data_bittiming.bitrate < priv->bittiming.bitrate))) {
//        netdev_err(dev, "incorrect/missing data bit-timing\n");
//        return -EINVAL;
//    }
//
//    /* Switch carrier on if device was stopped while in bus-off state */
//    if (!netif_carrier_ok(dev))
//        netif_carrier_on(dev);
//
//    setup_timer(&priv->restart_timer, can_restart, (unsigned long)dev);

    return 0;
}

/*
 * Common close function for cleanup before the device gets closed.
 *
 * This function should be called in the close function of the device
 * driver.
 */
void close_candev(struct net_device *dev)
{
//    struct can_priv *priv = netdev_priv(dev);
//
//    del_timer_sync(&priv->restart_timer);
//    can_flush_echo_skb(dev);
}

/* Drop a given socketbuffer if it does not contain a valid CAN frame. */
static inline bool can_dropped_invalid_skb(struct net_device *dev,
                      struct sk_buff *skb)
{
    return false;
}

typedef irqreturn_t (*irq_handler_t)(int, void *);

extern int
request_threaded_irq(unsigned int irq, irq_handler_t handler,
             irq_handler_t thread_fn,
             unsigned long flags, const char *name, void *dev)
{
	// TODO: install IRQ handler
	return 0;
}

static inline int
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
        const char *name, void *dev)
{
    return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}

/**
 *  free_irq - free an interrupt allocated with request_irq
 *  @irq: Interrupt line to free
 *  @dev_id: Device identity to free
 *
 *  Remove an interrupt handler. The handler is removed and if the
 *  interrupt line is no longer in use by any driver it is disabled.
 *  On a shared IRQ the caller must ensure the interrupt is disabled
 *  on the card it drives before calling this function. The function
 *  does not return until any executing interrupts for this IRQ
 *  have completed.
 *
 *  This function must not be called from interrupt context.
 */
void free_irq(unsigned int irq, void *dev_id)
{
}

// NETIF interface
//inline void netif_carrier_on (struct net_device *dev) {
//}
static inline void netif_carrier_off(struct net_device *dev)
{
}

/**
 *  netif_queue_stopped - test if transmit queue is flowblocked
 *  @dev: network device
 *
 *  Test if transmit queue on device is currently unable to send.
 */
static inline bool netif_queue_stopped(const struct net_device *dev)
{
	return true;
}

/**
 *  netif_wake_queue - restart transmit
 *  @dev: network device
 *
 *  Allow upper layers to call the device hard_start_xmit routine.
 *  Used for flow control when transmit resources are available.
 */
static inline void netif_wake_queue(struct net_device *dev)
{
//    netif_tx_wake_queue(netdev_get_tx_queue(dev, 0));
}

/**
 *  netif_stop_queue - stop transmitted packets
 *  @dev: network device
 *
 *  Stop upper layers calling the device hard_start_xmit routine.
 *  Used for flow control when transmit resources are unavailable.
 */
static inline void netif_stop_queue(struct net_device *dev)
{
//    netif_tx_stop_queue(netdev_get_tx_queue(dev, 0));
}

int netif_rx(struct sk_buff *skb) {
	return 0;
}

/**
 *  netif_start_queue - allow transmit
 *  @dev: network device
 *
 *  Allow upper layers to call the device hard_start_xmit routine.
 */
static inline void netif_start_queue(struct net_device *dev)
{
//    netif_tx_start_queue(netdev_get_tx_queue(dev, 0));
}
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
 * Allocate and setup space for the CAN network device
 */
struct net_device *alloc_candev(int sizeof_priv, unsigned int echo_skb_max)
{
    struct net_device *dev;
    struct can_priv *priv;
//    int size;

//    if (echo_skb_max)
//        size = ALIGN(sizeof_priv, sizeof(struct sk_buff *)) +
//            echo_skb_max * sizeof(struct sk_buff *);
//    else
//        size = sizeof_priv;

//    dev = alloc_netdev(size, "can%d", NET_NAME_UNKNOWN, can_setup);
    dev = new (std::nothrow) net_device;
    if (!dev)
        return NULL;

    dev->priv = (void*)new (std::nothrow) sja1000_priv;
    if (!dev->priv)
    	return NULL;

    priv = &netdev_priv(dev)->can;

	// TODO: echo
//    if (echo_skb_max) {
//        priv->echo_skb_max = echo_skb_max;
//        priv->echo_skb = (void *)priv +
//            ALIGN(sizeof_priv, sizeof(struct sk_buff *));
//    }

    priv->state = CAN_STATE_STOPPED;

//    init_timer(&priv->restart_timer);

    return dev;
}

/*
 * Free space of the CAN network device
 */
void free_candev(struct net_device *dev)
{
//    free_netdev(dev);
	sja1000_priv* priv = netdev_priv(dev);
	delete priv;
	delete dev;
}

#define __UAPI_DEF_IF_NET_DEVICE_FLAGS
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO
/**
 * enum net_device_flags - &struct net_device flags
 *
 * These are the &struct net_device flags, they can be set by drivers, the
 * kernel and some can be triggered by userspace. Userspace can query and
 * set these flags using userspace utilities but there is also a sysfs
 * entry available for all dev flags which can be queried and set. These flags
 * are shared for all types of net_devices. The sysfs entries are available
 * via /sys/class/net/<dev>/flags. Flags which can be toggled through sysfs
 * are annotated below, note that only a few flags can be toggled and some
 * other flags are always always preserved from the original net_device flags
 * even if you try to set them via sysfs. Flags which are always preserved
 * are kept under the flag grouping @IFF_VOLATILE. Flags which are volatile
 * are annotated below as such.
 *
 * You should have a pretty good reason to be extending these flags.
 *
 * @IFF_UP: interface is up. Can be toggled through sysfs.
 * @IFF_BROADCAST: broadcast address valid. Volatile.
 * @IFF_DEBUG: turn on debugging. Can be toggled through sysfs.
 * @IFF_LOOPBACK: is a loopback net. Volatile.
 * @IFF_POINTOPOINT: interface is has p-p link. Volatile.
 * @IFF_NOTRAILERS: avoid use of trailers. Can be toggled through sysfs.
 *  Volatile.
 * @IFF_RUNNING: interface RFC2863 OPER_UP. Volatile.
 * @IFF_NOARP: no ARP protocol. Can be toggled through sysfs. Volatile.
 * @IFF_PROMISC: receive all packets. Can be toggled through sysfs.
 * @IFF_ALLMULTI: receive all multicast packets. Can be toggled through
 *  sysfs.
 * @IFF_MASTER: master of a load balancer. Volatile.
 * @IFF_SLAVE: slave of a load balancer. Volatile.
 * @IFF_MULTICAST: Supports multicast. Can be toggled through sysfs.
 * @IFF_PORTSEL: can set media type. Can be toggled through sysfs.
 * @IFF_AUTOMEDIA: auto media select active. Can be toggled through sysfs.
 * @IFF_DYNAMIC: dialup device with changing addresses. Can be toggled
 *  through sysfs.
 * @IFF_LOWER_UP: driver signals L1 up. Volatile.
 * @IFF_DORMANT: driver signals dormant. Volatile.
 * @IFF_ECHO: echo sent packets. Volatile.
 */
enum net_device_flags {
/* for compatibility with glibc net/if.h */
#ifdef __UAPI_DEF_IF_NET_DEVICE_FLAGS
    IFF_UP              = 1<<0,  /* sysfs */
    IFF_BROADCAST           = 1<<1,  /* volatile */
    IFF_DEBUG           = 1<<2,  /* sysfs */
    IFF_LOOPBACK            = 1<<3,  /* volatile */
    IFF_POINTOPOINT         = 1<<4,  /* volatile */
    IFF_NOTRAILERS          = 1<<5,  /* sysfs */
    IFF_RUNNING         = 1<<6,  /* volatile */
    IFF_NOARP           = 1<<7,  /* sysfs */
    IFF_PROMISC         = 1<<8,  /* sysfs */
    IFF_ALLMULTI            = 1<<9,  /* sysfs */
    IFF_MASTER          = 1<<10, /* volatile */
    IFF_SLAVE           = 1<<11, /* volatile */
    IFF_MULTICAST           = 1<<12, /* sysfs */
    IFF_PORTSEL         = 1<<13, /* sysfs */
    IFF_AUTOMEDIA           = 1<<14, /* sysfs */
    IFF_DYNAMIC         = 1<<15, /* sysfs */
#endif /* __UAPI_DEF_IF_NET_DEVICE_FLAGS */
#ifdef __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO
    IFF_LOWER_UP            = 1<<16, /* volatile */
    IFF_DORMANT         = 1<<17, /* volatile */
    IFF_ECHO            = 1<<18, /* volatile */
#endif /* __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO */
};

/*
 * changing MTU and control mode for CAN/CANFD devices
 */
int can_change_mtu(struct net_device *dev, int new_mtu)
{
    struct can_priv *priv = &netdev_priv(dev)->can;

    /* Do not allow changing the MTU while running */
    if (dev->flags & IFF_UP)
        return -EBUSY;

    /* allow change of MTU according to the CANFD ability of the device */
    switch (new_mtu) {
    case CAN_MTU:
        /* 'CANFD-only' controllers can not switch to CAN_MTU */
        if (priv->ctrlmode_static & CAN_CTRLMODE_FD)
            return -EINVAL;

        priv->ctrlmode &= ~CAN_CTRLMODE_FD;
        break;

    case CANFD_MTU:
        /* check for potential CANFD ability */
        if (!(priv->ctrlmode_supported & CAN_CTRLMODE_FD) &&
            !(priv->ctrlmode_static & CAN_CTRLMODE_FD))
            return -EINVAL;

        priv->ctrlmode |= CAN_CTRLMODE_FD;
        break;

    default:
        return -EINVAL;
    }

    dev->mtu = new_mtu;
    return 0;
}

// New DE

#endif /* SJA1000_DEV_H */
