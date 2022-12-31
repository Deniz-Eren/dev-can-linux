/*
 * \file    timer.c
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

#include <pthread.h>
#include <sys/netmgr.h>

#include "timer.h"

/* check custom timer pulse codes are within safe range */
#if _PULSE_CODE_SHUTDOWN_TIMER < _PULSE_CODE_MINAVAIL || \
    _PULSE_CODE_SHUTDOWN_TIMER >= _PULSE_CODE_MAXAVAIL
#error Invalid (_PULSE_CODE_SHUTDOWN_TIMER) safe range of pulse values is \
    _PULSE_CODE_MINAVAIL through _PULSE_CODE_MAXAVAIL
#endif

#if _PULSE_CODE_TIMER_TRIGGER < _PULSE_CODE_MINAVAIL || \
    _PULSE_CODE_TIMER_TRIGGER >= _PULSE_CODE_MAXAVAIL
#error Invalid (_PULSE_CODE_TIMER_TRIGGER) safe range of pulse values is \
    _PULSE_CODE_MINAVAIL through _PULSE_CODE_MAXAVAIL
#endif

/* check custom timer pulse codes are unique */
#if _PULSE_CODE_TIMER_TRIGGER == _PULSE_CODE_SHUTDOWN_TIMER
#error _PULSE_CODE_SHUTDOWN_TIMER and _PULSE_CODE_TIMER_TRIGGER must have \
    unique values
#endif


static void* timer_loop (void* arg);


static void create (timer_record_t* timer) {
    if (timer == NULL) {
        return;
    }

    struct sched_param scheduling_params;
    int prio;

    if (timer->created) {
        return;
    }

    timer->chid = ChannelCreate(0);

    if (timer->chid == -1) {
        log_err( "timer ChannelCreate error; %s\n",
                strerror(errno) );
    }

    /* Get our priority. */
    if (SchedGet(0, 0, &scheduling_params) != -1) {
       prio = scheduling_params.sched_priority;
    }
    else {
       prio = 10;
    }

    timer->event.sigev_notify = SIGEV_PULSE;
    timer->event.sigev_coid =
        ConnectAttach(ND_LOCAL_NODE, 0, timer->chid, _NTO_SIDE_CHANNEL, 0);

    if (timer->event.sigev_coid == -1) {
        log_err( "timer ConnectAttach error; %s\n",
                strerror(errno) );
    }

    timer->event.sigev_priority = prio;
    timer->event.sigev_code = _PULSE_CODE_TIMER_TRIGGER;

    if (timer_create(CLOCK_MONOTONIC, &timer->event, &timer->id) == -1) {
        log_err( "timer timer_create error; %s\n",
                strerror(errno) );
    }

    /* Start the timer thread */
    pthread_attr_init(&timer->attr);
    pthread_attr_setdetachstate(&timer->attr, PTHREAD_CREATE_DETACHED);
    pthread_create(NULL, &timer->attr, &timer_loop, timer);

    timer->created = 1;
}

void destroy (timer_record_t* timer) {
    if (timer == NULL) {
        return;
    }

    if (!timer->created) {
        return;
    }

    if (MsgSendPulse(
        timer->event.sigev_coid,
        -1, _PULSE_CODE_SHUTDOWN_TIMER, 0 ) == -1)
    {
        log_err( "destroy timer MsgSendPulse error; %s\n",
                strerror(errno) );
    }

    if (timer_delete(timer->id) == -1) {
        log_err( "destroy timer timer_delete error; %s\n",
                strerror(errno) );
    }

    if (ConnectDetach(timer->event.sigev_coid) == -1) {
        log_err( "destroy timer ConnectDetach error; %s\n",
                strerror(errno) );
    }

    if (ChannelDestroy(timer->chid) == -1) {
        log_err( "destroy timer ChannelDestroy error; %s\n",
                strerror(errno) );
    }

    timer->created = 0;
}

void setup_timer (timer_record_t* timer, void (*callback)(void*), void *data) {
    if (timer == NULL) {
        log_err("setup_timer: invalid pointer\n");

        return;
    }

    log_trace( "setup_timer (%x%x)\n",
            (u32)(~(u32)0 & ((u64)timer >> 32)),
            (u32)(~(u32)0 & (u64)timer) );

    timer->created = 0;
    timer->callback = callback;
    timer->data = data;

    create(timer);
}

static void* timer_loop (void* arg) {
    timer_record_t* timer = (timer_record_t*)arg;

    int             rcvid;
    struct _pulse   pulse;

    for (;;) {
        rcvid = MsgReceive(timer->chid, &pulse, sizeof(struct _pulse), NULL);
        if (rcvid == 0) { /* we got a pulse */
            if (pulse.code == _PULSE_CODE_TIMER_TRIGGER) {
                timer->callback(timer->data);
            }
	        else if (pulse.code == _PULSE_CODE_SHUTDOWN_TIMER) {
	            log_trace("timer_loop shutdown\n");

	            pthread_exit(NULL);
            } /* else other pulses ... */
        } /* else other messages ... */
    }

    return NULL;
}

void cancel_delayed_work_sync (timer_record_t* timer) {
    if (timer == NULL) {
        log_err("cancel_delayed_work_sync: invalid pointer\n");

        return;
    }

    log_trace( "cancel_delayed_work_sync (%x%x)\n",
            (u32)(~(u32)0 & ((u64)timer >> 32)),
            (u32)(~(u32)0 & (u64)timer) );

    destroy(timer);
}

void schedule_delayed_work (timer_record_t* timer, int ticks) {
    if (timer == NULL) {
        log_err("schedule_delayed_work: invalid pointer\n");

        return;
    }

    log_trace( "schedule_delayed_work (%x%x)\n",
            (u32)(~(u32)0 & ((u64)timer >> 32)),
            (u32)(~(u32)0 & (u64)timer) );

    create(timer);

    struct itimerspec itime;

    // first trigger, seconds:
    itime.it_value.tv_sec = 0;
    // first trigger, 0.1s in nanoseconds:
    itime.it_value.tv_nsec = TIMER_INTERVAL_NS * ticks;

    // interval between triggers, seconds:
    itime.it_interval.tv_sec = 0;
    // interval between triggers, nanoseconds:
    itime.it_interval.tv_nsec = 0;

    if (timer_settime(timer->id, 0, &itime, NULL) == -1) {
        log_err( "schedule_delayed_work timer_settime error; %s\n",
                strerror(errno) );
    }

    timer->created = 1;
}

uint64_t get_clock_time_us() {
    static uint64_t cycles_per_us = 0;

    if (cycles_per_us == 0) {
        cycles_per_us = SYSPAGE_ENTRY(qtime)->cycles_per_sec / MILLION;
    }

    return ClockCycles() / cycles_per_us;
}
