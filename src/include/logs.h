/*
 * \file    logs.h
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

#ifndef SRC_LOGS_H_
#define SRC_LOGS_H_

#include <stdio.h>
#include <syslog.h>

#include "main.h"


/*
 * Used to disable logging during ISR call
 *
 * Both syslog and fprintf are not safe for interrupt handlers. Also it should
 * be safe to declare this volatile and not require any spinlocks, because
 * according to QNX documentation only a single CPU will ever be used for ISR
 * handling.
 * Quote: "On a multicore system, each interrupt is directed to one (and only
 *      one) CPU, although it doesn't matter which. How this happens is under
 *      control of the programmable interrupt controller (PIC) chip(s) on the
 *      board. When you initialize the PICs in your system's startup, you can
 *      program them to deliver the interrupts to whichever CPU you want to; on
 *      some PICs you can even get the interrupt to rotate between the CPUs each
 *      time it goes off."
 */
extern volatile unsigned log_enabled;

#define log_err(fmt, arg...) { \
        if (log_enabled && !optq) { \
            syslog(LOG_ERR, fmt, ##arg); \
            fprintf(stderr, fmt, ##arg); \
        } \
    }

#define log_warn(fmt, arg...) { \
        if (log_enabled && !optq) { \
            syslog(LOG_WARNING, fmt, ##arg); \
            fprintf(stderr, fmt, ##arg); \
        } \
    }

#define log_info(fmt, arg...) { \
        if (log_enabled && !optq) { \
            if (optv >= 1) { \
                syslog(LOG_INFO, fmt, ##arg); \
            } \
            if (!optq && optv >= 3) { \
                printf(fmt, ##arg); \
            } \
        } \
    }

#define log_dbg(fmt, arg...) { \
        if (log_enabled && !optq) { \
            if (optv >= 2) { \
                syslog(LOG_DEBUG, fmt, ##arg); \
            } \
            if (!optq && optv >= 4) { \
                printf(fmt, ##arg); \
            } \
        } \
    }

#define log_trace(fmt, arg...) { \
        if (log_enabled && !optq) { \
            if (optv >= 5) { \
                syslog(LOG_DEBUG, fmt, ##arg); \
            } \
            if (!optq && optv >= 6) { \
                printf(fmt, ##arg); \
            } \
        } \
    }

/* Mapping of dev_*() calls to log_(*) */
#define dev_err(dev, fmt, arg...) log_err(fmt, ##arg)
#define dev_warn(dev, fmt, arg...) log_warn(fmt, ##arg)
#define dev_info(dev, fmt, arg...) log_info(fmt, ##arg)
#define dev_dbg(dev, fmt, arg...) log_dbg(fmt, ##arg)

/* Mapping of netdev_*() calls to log_(*) */
#define netdev_err(dev, fmt, arg...) log_err(fmt, ##arg)
#define netdev_warn(dev, fmt, arg...) log_warn(fmt, ##arg)
#define netdev_info(dev, fmt, arg...) log_info(fmt, ##arg)
#define netdev_dbg(dev, fmt, arg...) log_dbg(fmt, ##arg)

// TODO: check what the *_once() variant is meant to do
#define netdev_info_once(dev, fmt, arg...) netdev_dbg(dev, fmt, ##arg)

#endif /* SRC_LOGS_H_ */
