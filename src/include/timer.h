/*
 * \file    timer.h
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

#ifndef SRC_TIMER_H_
#define SRC_TIMER_H_

#include <time.h>
#include <linux/types.h>


/* Timer shutdown pulse */
#define _PULSE_CODE_SHUTDOWN_PROGRAM _PULSE_CODE_MINAVAIL

/*
 * SJA1000 Bus-off recovery timer
 */
#define HZ                  CONFIG_HZ // Internal kernel timer frequency
#define TIMER_INTERVAL_NS   (1000000000UL / (HZ)) // Timer interval nanoseconds
#define jiffies             0
#define TIMER_PULSE_CODE    _PULSE_CODE_MINAVAIL

/*
 * Functions needed by the Linux Kernel source "drivers/net/can/dev.c" to
 * implement Bus-off recovery timer functionality.
 */
extern void setup_timer (timer_t* timer_id, void (*callback)(void*),
        void *priv);

extern void cancel_delayed_work_sync (timer_t* timer_id);
extern void schedule_delayed_work (timer_t* timer_id, int ticks);

#endif /* SRC_TIMER_H_ */
