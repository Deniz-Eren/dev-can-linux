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
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/neutrino.h> /* QNX header */

#include "timer.h"


/*
 * Program options
 */

extern int optv;
extern int optl;
extern int opti;
extern int optq;
extern int optd;
extern int opt_vid;
extern int opt_did;
extern int opts;
extern int optt;
extern int optu;

#define DEFAULT_NUM_RX_CHANNELS 1
#define DEFAULT_NUM_TX_CHANNELS 1

typedef struct channel_config {
    int id;
    int num_rx_channels;
    int num_tx_channels;
} channel_config_t;

extern size_t num_optu_configs;
extern channel_config_t* optu_config;


/*
 * Linux Kernel platform and architecture macros
 */

#define __KERNEL__
#define __iomem volatile

#define BITS_PER_LONG (64)

/* Defines the need for macro IFNAMSIZ */
#define __UAPI_DEF_IF_IFNAMSIZ (1)

/* Defines the need for macros
 *  - IFF_UP        interface is up
 *  - IFF_NOARP     no address resolution protocol */
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS (1)

/* Defines the need for macro IFF_ECHO; this is a unique network device flags in
 * that no equivalent exists in QNX (e.g. /usr/include/net/if.h). */
#define __UAPI_DEF_IF_NET_DEVICE_FLAGS_LOWER_UP_DORMANT_ECHO (1)

/*
 * Linux Kernel compiler definition macros
 */

#define __always_inline     inline // Impact: reduce binary size
#define __must_check
#define __attribute_const__

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

#endif /* SRC_CONFIG_H_ */
