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
#include <stdarg.h>
#include <syslog.h>

#include "config.h"


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

extern int optl;
extern int optq;
extern int optv;

/**
 * @brief Logs debug messages to syslog and/or stdout based on configuration.
 *
 * @param priority The syslog priority (e.g., LOG_DEBUG, LOG_INFO, LOG_WARNING, etc.).
 * @param v The minimum verbosity level required for logging to occur.
 * @param fmt The format string, followed by variable arguments.
 */
static inline void log_generic (int priority, int v, const char *fmt, ...) {
    if (log_enabled) {
        va_list args;

        if (optl >= v) {
            va_start(args, fmt);
            vsyslog(priority, fmt, args);
            va_end(args);
        }

        if (!optq && optv >= v) {
            va_start(args, fmt); // Re-initialize args
            vfprintf(stdout, fmt, args);
            va_end(args);
        }
    }
}

#define log_cus(fmt, arg...)    log_generic(LOG_INFO, -1, fmt, ##arg)
#define log_err(fmt, arg...)    log_generic(LOG_ERR, -1, fmt, ##arg)
#define log_warn(fmt, arg...)   log_generic(LOG_WARNING, -1, fmt, ##arg)
#define log_info(fmt, arg...)   log_generic(LOG_INFO, 1, fmt, ##arg)
#define log_dbg(fmt, arg...)    log_generic(LOG_DEBUG, 2, fmt, ##arg)
#define log_trace(fmt, arg...)  log_generic(LOG_DEBUG, 3, fmt, ##arg)

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

#define NL_SET_ERR_MSG_FMT(extack, fmt, arg...) log_err(fmt, ##arg)

#endif /* SRC_LOGS_H_ */
