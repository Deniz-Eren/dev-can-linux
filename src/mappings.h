#ifndef _TYPES_H
#define _TYPES_H

#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <stdint.h>
#include <syslog.h>
#include <sys/neutrino.h>
#include <pci/pci.h>

typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;

typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;

typedef __s8 s8;
typedef __s16 s16;
typedef __s32 s32;

typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;

typedef intrspin_t spinlock_t;

#define __KERNEL__
#define __UAPI_DEF_IF_IFNAMSIZ
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO

/* Do nothing for the following macros */
#define MODULE_AUTHOR(str)
#define MODULE_DESCRIPTION(str)
#define MODULE_SUPPORTED_DEVICE(str)
#define MODULE_LICENSE(str)
#define MODULE_DEVICE_TABLE(pci, tbl)
#define module_pci_driver(pci_driver)
#define EXPORT_SYMBOL_GPL(func)

/* Define mapping to QNX IO */
#define readb(addr)			in8(addr)
#define readw(addr)			in16(addr)
#define readl(addr)			in32(addr)

#define ioread8(addr)       readb(addr)
#define ioread16(addr)      readw(addr)
#define ioread32(addr)      readl(addr)

#define writeb(v, addr)		out8((addr), (v))
#define writew(v, addr)		out16((addr), (v))
#define writel(v, addr)		out32((addr), (v))

#define iowrite8(v, addr)   writeb((v), (addr))
#define iowrite16(v, addr)  writew((v), (addr))
#define iowrite32(v, addr)  writel((v), (addr))

extern int optv;
extern int optq;
extern int optd;
extern int opt_vid;
extern int opt_did;

#define log_err(fmt, arg...) { \
		if (!optq) { \
			syslog(LOG_ERR, fmt, ##arg); \
			printf(fmt, ##arg); \
		} \
	}

#define log_warn(fmt, arg...) { \
		if (!optq) { \
			syslog(LOG_WARNING, fmt, ##arg); \
			printf(fmt, ##arg); \
		} \
	}

#define log_info(fmt, arg...) { \
		if (!optq) { \
			if (optv >= 1) { \
				syslog(LOG_INFO, fmt, ##arg); \
			} \
			if (!optq && optv >= 3) { \
				printf(fmt, ##arg); \
			} \
		} \
	}

#define log_dbg(fmt, arg...) { \
		if (!optq) { \
			if (optv >= 2) { \
				syslog(LOG_DEBUG, fmt, ##arg); \
			} \
			if (!optq && optv >= 4) { \
				printf(fmt, ##arg); \
			} \
		} \
	}

#define log_trace(fmt, arg...) { \
		if (!optq) { \
			if (optv >= 5) { \
				syslog(LOG_DEBUG, fmt, ##arg); \
			} \
			if (!optq && optv >= 6) { \
				printf(fmt, ##arg); \
			} \
		} \
	}

/* Define mapping of dev_*() calls to syslog(*) */
#define dev_err(dev, fmt, arg...) log_err(fmt, ##arg)
#define dev_warn(dev, fmt, arg...) log_warn(fmt, ##arg)
#define dev_info(dev, fmt, arg...) log_info(fmt, ##arg)
#define dev_dbg(dev, fmt, arg...) log_dbg(fmt, ##arg)

/* Define mapping of netdev_*() calls to syslog(*) */
#define netdev_err(dev, fmt, arg...) log_err(fmt, ##arg)
#define netdev_warn(dev, fmt, arg...) log_warn(fmt, ##arg)
#define netdev_info(dev, fmt, arg...) log_info(fmt, ##arg)
#define netdev_dbg(dev, fmt, arg...) log_dbg(fmt, ##arg)

/* Structure used for driver selection processing */
struct driver_selection_t {
	pci_vid_t vid;
    pci_did_t did;

	int driver_auto;
	int driver_pick;
	int driver_ignored;
	int driver_unsupported;
};

/* Define mapping of kzalloc to simply malloc */
#define kzalloc(size, gfp) malloc(size)
#define kfree(ptr) free(ptr)

/* Define mapping of IRQ spin lock */
#define spin_lock_init(lock) memset(lock, 0, sizeof(intrspin_t))
#define spin_lock_irqsave(lock, flags) InterruptLock(lock)
#define spin_unlock_irqrestore(lock, flags) InterruptUnlock(lock)

extern struct pci_driver adv_pci_driver;
extern struct pci_driver kvaser_pci_driver;
extern struct pci_driver ems_pci_driver;
extern struct pci_driver peak_pci_driver;
extern struct pci_driver plx_pci_driver;

#endif /* _TYPES_H */
