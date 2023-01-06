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

#include <sys/syspage.h>
#include <linux/types.h>


/* Timer shutdown pulse */
#define _PULSE_CODE_SHUTDOWN_TIMER  (_PULSE_CODE_MINAVAIL+0)
#define _PULSE_CODE_TIMER_TRIGGER   (_PULSE_CODE_MINAVAIL+1)

/* some scaling factors */
#define MILLION	1e6 
#define BILLION 1e9

/*
 * SJA1000 Bus-off recovery timer
 */
#define HZ                  CONFIG_HZ // Internal kernel timer frequency
#define TIMER_INTERVAL_NS   ((BILLION) / (HZ)) // Timer interval nanoseconds
#define jiffies             0

typedef struct timer_record {
    timer_t id;

    int created, chid;

    void (*callback)(void*);
    void *data;

    struct sigevent event;

    pthread_attr_t attr;
} timer_record_t;

/*
 * Functions needed by the Linux Kernel source "drivers/net/can/dev.c" to
 * implement Bus-off recovery timer functionality.
 */
extern void setup_timer (timer_record_t* timer, void (*callback)(void*),
        void *priv);

extern void cancel_delayed_work_sync (timer_record_t* timer);
extern void schedule_delayed_work (timer_record_t* timer, int ticks);

/* accurate relative time function in us */
extern uint64_t get_clock_time_us();

/*
 * Behaviour of these two variables depends on the option '-t' (optt).
 * IF '-t' enabled, then user_timestamp supplied by the user via the devctl
 *      command CAN_DEVCTL_SET_TIMESTAMP is used as the message timestamp. Thus
 *      it is upto the user to ensure this command is called sufficiently
 *      regularly.
 * IF '-t' is not enabled, then the user_timestamp supplied by the user, is
 *      used to synchronize the internal timstamping using the point in time
 *      user_timestamp_time determined from get_clock_time_us()/10000. The
 *      timestamp is recorded in millisecond in this mode.
 */
extern uint32_t user_timestamp;
extern uint32_t user_timestamp_time;

#endif /* SRC_TIMER_H_ */
