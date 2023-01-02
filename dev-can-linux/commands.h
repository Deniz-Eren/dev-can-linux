/*
 * \file    commands.h
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if DEVCANLINUX_SYSLOG == 1
#include <syslog.h>
#endif

#include <fcntl.h>
#include <sys/can_dcmd.h>

#if DEVCANLINUX_SYSLOG == 1
# define SYSLOG_ERR(fmt, arg...) syslog(LOG_ERR, fmt, ##arg)
#else
# define SYSLOG_ERR(fmt, arg...)
#endif

#if DEVCANLINUX_STDERR == 1
# define FPRINTF_ERR(fmt, arg...) fprintf(stderr, fmt, ##arg)
#else
# define FPRINTF_ERR(fmt, arg...)
#endif

#ifndef log_err
# define log_err(fmt, arg...) {   \
        SYSLOG_ERR(fmt, ##arg);  \
        FPRINTF_ERR(fmt, ##arg); \
    }
#endif

/*
 * Extended devctl() commands; these are in addition to the standard
 * QNX dev-can-* driver protocol commands
 */
#define EXT_CAN_CMD_CODE                    0x54
#define EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS __DIOT(_DCMD_MISC, EXT_CAN_CMD_CODE + 0,  uint32_t)


static inline int write_canmsg_ext (int filedes, struct can_msg* canmsg) {
    int ret;

    if (EOK != (ret = devctl(
            filedes, CAN_DEVCTL_WRITE_CANMSG_EXT,
            canmsg, sizeof(struct can_msg), NULL )))
    {
        log_err("devctl CAN_DEVCTL_WRITE_CANMSG_EXT: %s\n", strerror(ret));

        return -1;
    }

    return EOK;
}

static inline int read_canmsg_ext (int filedes, struct can_msg* canmsg) {
    int ret;

    if (EOK != (ret = devctl(
            filedes, CAN_DEVCTL_READ_CANMSG_EXT,
            canmsg, sizeof(struct can_msg), NULL )))
    {
        log_err("devctl CAN_DEVCTL_READ_CANMSG_EXT: %s\n", strerror(ret));

        return -1;
    }

    return EOK;
}

static inline int set_latency_limit_ms (int filedes, uint32_t value) {
    int ret;

    if (EOK != (ret = devctl(
            filedes, EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS,
            &value, sizeof(uint32_t), NULL )))
    {
        log_err("devctl EXT_CAN_DEVCTL_SET_LATENCY_LIMIT_MS: %s\n",
                strerror(ret));

        return -1;
    }

    return EOK;
}
