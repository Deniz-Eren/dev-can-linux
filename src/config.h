#ifndef _TYPES_H
#define _TYPES_H

/*
 * Linux Kernel config macros needed to be enabled
 */
#define CONFIG_CAN_CALC_BITTIMING
#define CONFIG_CAN_LEDS
#define CONFIG_HZ 1000
#define CONFIG_X86_32
#define CONFIG_X86_64

#define BITS_PER_LONG 64

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <syslog.h>
#include <sys/neutrino.h>
#include <pci/pci.h>

typedef int8_t __s8;
typedef int16_t __s16;
typedef int32_t __s32;
typedef int64_t __s64;

typedef uint8_t __u8;
typedef uint16_t __u16;
typedef uint32_t __u32;
typedef uint64_t __u64;

typedef __s8 s8;
typedef __s16 s16;
typedef __s32 s32;
typedef __s64 s64;

typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

#ifdef __CHECKER__
#define __bitwise__ __attribute__((bitwise))
#else
#define __bitwise__
#endif
#ifdef __CHECK_ENDIAN__
#define __bitwise __bitwise__
#else
#define __bitwise
#endif

typedef __u16 __bitwise __le16;
typedef __u16 __bitwise __be16;
typedef __u32 __bitwise __le32;
typedef __u32 __bitwise __be32;
typedef __u64 __bitwise __le64;
typedef __u64 __bitwise __be64;

typedef __u16 __bitwise __sum16;
typedef __u32 __bitwise __wsum;

typedef intrspin_t spinlock_t;

#define __KERNEL__
#define __UAPI_DEF_IF_IFNAMSIZ
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO
#define __read_mostly
#define __always_inline

/*
 * Program options
 */
extern int optv;
extern int optq;
extern int optd;
extern int opt_vid;
extern int opt_did;

/*
 * Logging macros
 */
#define log_err(fmt, arg...) { \
        if (!optq) { \
            syslog(LOG_ERR, fmt, ##arg); \
            fprintf(stderr, fmt, ##arg); \
        } \
    }

#define log_warn(fmt, arg...) { \
        if (!optq) { \
            syslog(LOG_WARNING, fmt, ##arg); \
            fprintf(stderr, fmt, ##arg); \
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

/* Define mapping of IRQ spin lock */
#define spin_lock_init(lock) memset(lock, 0, sizeof(intrspin_t))
#define spin_lock_irqsave(lock, flags) InterruptLock(lock)
#define spin_unlock_irqrestore(lock, flags) InterruptUnlock(lock)

extern struct pci_driver adv_pci_driver;
extern struct pci_driver kvaser_pci_driver;
extern struct pci_driver ems_pci_driver;
extern struct pci_driver peak_pci_driver;
extern struct pci_driver plx_pci_driver;

extern struct pci_driver *detected_driver;

/*
 * Timer
 */
#define HZ                  CONFIG_HZ // Internal kernel timer frequency
#define TIMER_INTERVAL_NS   (1000000000UL / (HZ)) // Timer interval nanoseconds
#define jiffies             0
#define TIMER_PULSE_CODE    _PULSE_CODE_MINAVAIL

// Convenient to replace restart_timer type in struct can_priv with QNX timer_t
// to get equivalent functionality
#define timer_list timer_t

typedef union {
    struct _pulse   pulse;
    // other message structures would go here
} timer_msg_t;

extern void setup_timer (timer_t* timer_id, void (*callback)(unsigned long), unsigned long data);
extern void del_timer_sync (timer_t* timer_id);
extern void mod_timer (timer_t* timer_id, int ticks);

/* Netlink layer is not used */
struct null_struct_t { };
#define nlattr null_struct_t
#define net  null_struct_t

#endif /* _TYPES_H */
