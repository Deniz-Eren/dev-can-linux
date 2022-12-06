/*
 * \file    config.h
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

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include <errno.h>
#include <syslog.h>
#include <sys/neutrino.h>
#include <pci/pci.h>

#include "timer.h"


#define program_version "1.0.0" /* Release version */


/*
 * Linux Kernel configuration macros
 */

#define CONFIG_HZ 1000
#define CONFIG_CAN_CALC_BITTIMING
#define CONFIG_X86_32
#define CONFIG_X86_64
//#define CONFIG_CAN_LEDS /* Currently LEDS are not supported */


/*
 * Linux Kernel platform and architecture macros
 */

#define BITS_PER_LONG 64

#define __UAPI_DEF_IF_IFNAMSIZ /* defines the need for macro IFNAMSIZ */

/* Defines the need for macros
 *  - IFF_UP        interface is up
 *  - IFF_NOARP     no address resolution protocol */
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS

/* Defines the need for macro IFF_ECHO; this is a unique network device flags in
 * that no equivalent exists in QNX (e.g. /usr/include/net/if.h). */
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO


/*
 * Linux Kernel compiler definition macros
 */

#define __always_inline     inline


/*
 * QNX Resource Manager macros
 */

#define MAX_DEVICES 16


/*
 * Mapping stdint.h types to Linux Kernel needed type names
 */

typedef int8_t      __s8;
typedef int16_t     __s16;
typedef int32_t     __s32;
typedef int64_t     __s64;

typedef uint8_t     __u8;
typedef uint16_t    __u16;
typedef uint32_t    __u32;
typedef uint64_t    __u64;

typedef __s8        s8;
typedef __s16       s16;
typedef __s32       s32;
typedef __s64       s64;

typedef __u8        u8;
typedef __u16       u16;
typedef __u32       u32;
typedef __u64       u64;


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

#endif /* SRC_CONFIG_H_ */
