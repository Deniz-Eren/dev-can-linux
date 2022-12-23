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


static struct timer_record_t {
    timer_t* id;

    void (*callback)(void*);
    void *data;
    int num_callbacks;

    struct sigevent event;
    int chid;

    int created;

    pthread_attr_t attr;
} timer[MAX_DEVICES];

static int n_timers = 0;

static void* timer_loop (void* arg);


void create (int i) {
    struct sched_param scheduling_params;
    int prio;

    if (timer[i].created) {
        return;
    }

    timer[i].chid = ChannelCreate(0);

    if (timer[i].chid == -1) {
        log_err( "create(%d) ChannelCreate error; %s\n",
                i, strerror(errno) );
    }

    /* Get our priority. */
    if (SchedGet(0, 0, &scheduling_params) != -1) {
       prio = scheduling_params.sched_priority;
    }
    else {
       prio = 10;
    }

    timer[i].event.sigev_notify = SIGEV_PULSE;
    timer[i].event.sigev_coid =
        ConnectAttach(ND_LOCAL_NODE, 0, timer[i].chid, _NTO_SIDE_CHANNEL, 0);

    if (timer[i].event.sigev_coid == -1) {
        log_err( "create(%d) ConnectAttach error; %s\n",
                i, strerror(errno) );
    }

    timer[i].event.sigev_priority = prio;
    timer[i].event.sigev_code = _PULSE_CODE_TIMER_TRIGGER;

    if (timer_create(CLOCK_MONOTONIC, &timer[i].event, timer[i].id) == -1) {
        log_err( "create(%d) timer_create error; %s\n",
                i, strerror(errno) );
    }

    /* Start the timer thread */
    pthread_attr_init(&timer[i].attr);
    pthread_attr_setdetachstate(&timer[i].attr, PTHREAD_CREATE_DETACHED);
    pthread_create(NULL, &timer[i].attr, &timer_loop, &n_timers);

    timer[i].created = 1;
}

void destroy (int i) {
    if (!timer[i].created) {
        return;
    }

    if (MsgSendPulse(
        timer[i].event.sigev_coid,
        -1, _PULSE_CODE_SHUTDOWN_TIMER, 0 ) == -1)
    {
        log_err( "destroy(%d) MsgSendPulse error; %s\n",
                i, strerror(errno) );
    }

    if (timer_delete(*timer[i].id) == -1) {
        log_err( "destroy(%d) timer_delete error; %s\n",
                i, strerror(errno) );
    }

    if (ConnectDetach(timer[i].event.sigev_coid) == -1) {
        log_err( "destroy(%d) ConnectDetach error; %s\n",
                i, strerror(errno) );
    }

    if (ChannelDestroy(timer[i].chid) == -1) {
        log_err( "destroy(%d) ChannelDestroy error; %s\n",
                i, strerror(errno) );
    }

    timer[i].created = 0;
}

void setup_timer (timer_t* timer_id, void (*callback)(void*), void *data) {
    int i = n_timers;

    if (i >= MAX_DEVICES) {
        log_err( "setup_timer (%d:%x%x) fail; max devices reached\n", i,
                (u32)(~(u32)0 & ((u64)timer_id >> 32)),
                (u32)(~(u32)0 & (u64)timer_id) );
        return;
    }

    log_trace( "setup_timer (%d:%x%x)\n", i,
            (u32)(~(u32)0 & ((u64)timer_id >> 32)),
            (u32)(~(u32)0 & (u64)timer_id) );

    timer[i].created = 0;
    timer[i].id = timer_id;
    timer[i].callback = callback;
    timer[i].data = data;

    create(i);

    ++n_timers;
}

static void* timer_loop (void* arg) {
    int i = *(int*)arg;

    int             rcvid;
    struct _pulse   pulse;

    for (;;) {
        rcvid = MsgReceive(timer[i].chid, &pulse, sizeof(struct _pulse), NULL);
        if (rcvid == 0) { /* we got a pulse */
            if (pulse.code == _PULSE_CODE_TIMER_TRIGGER) {
                timer[i].callback(timer[i].data);
            }
	        else if (pulse.code == _PULSE_CODE_SHUTDOWN_TIMER) {
	            log_trace("timer_loop (%d) shutdow\n", i);

	            pthread_exit(NULL);
            } /* else other pulses ... */
        } /* else other messages ... */
    }

    return NULL;
}

void cancel_delayed_work_sync (timer_t* timer_id) {
    int i = -1, j;

    for (j = 0; j < n_timers; ++j) {
        if (timer[j].id == timer_id) {
            i = j;
            break;
        }
    }

    log_trace( "cancel_delayed_work_sync (%d:%x%x)\n", i,
            (u32)(~(u32)0 & ((u64)timer_id >> 32)),
            (u32)(~(u32)0 & (u64)timer_id) );

    if (i == -1) {
        log_err("cancel_delayed_work_sync unknown timer\n");
        return;
    }

    destroy(i);

/*
 * Important!
 * Linux CAN-bus driver calls the following 3 functions to handle scheduled timer
 * workloads:
 *      setup( callback function, data )
 *      schedule_delayed_work()
 *      cancel_delayed_work_sync()
 *
 * It does this by calling setup() in the begining and then
 * schedule_delayed_work() everytime works needs to be scheduled. At the very end
 * it calls cancel_delayed_work_sync() to shutdown. However, there seems to be
 * code that is open to the possibility of calling schedule_delayed_work() after
 * cancel_delayed_work_sync() was called.
 * For this reason, and since setup() is only called in the begining, we cannot
 * lose our records of the "callback function" and "data".
 * Also keeping in mind this timer based workload scheduler isn't a completely
 * dynamic one, and that a fixed number of timers are expected per executable
 * application.
 * For these reasons, we will not perform the following "maintain timer
 * inventory" section, however it is left here in commented out form for this
 * illustrate this explanation.
 */
//    /* maintain timer inventory */
//    for (j = i; j < n_timers; ++j) {
//        timer[j] = timer[j+1];
//    }
//
//    --n_timers;
}

void schedule_delayed_work (timer_t* timer_id, int ticks) {
    int i = -1, j;

    for (j = 0; j < n_timers; ++j) {
        if (timer[j].id == timer_id) {
            i = j;
            break;
        }
    }

    log_trace( "schedule_delayed_work (%d:%x%x)\n", i,
            (u32)(~(u32)0 & ((u64)timer_id >> 32)),
            (u32)(~(u32)0 & (u64)timer_id) );

    if (i == -1) {
        log_err("mod_timer unknown timer\n");
        return;
    }

    create(i);

    struct itimerspec       itime;

    // first trigger, seconds:
    itime.it_value.tv_sec = 0;
    // first trigger, 0.1s in nanoseconds:
    itime.it_value.tv_nsec = TIMER_INTERVAL_NS * ticks;

    // interval between triggers, seconds:
    itime.it_interval.tv_sec = 0;
    // interval between triggers, nanoseconds:
    itime.it_interval.tv_nsec = 0;

    if (timer_settime(*timer[i].id, 0, &itime, NULL) == -1) {
        log_err( "timer_loop (%d) timer_settime error; %s\n",
                i, strerror(errno) );
    }

    timer[i].created = 1;
}

uint64_t get_clock_time_us () {
    static uint64_t cycles_per_us = 0;

    if (cycles_per_us == 0) {
        cycles_per_us = SYSPAGE_ENTRY(qtime)->cycles_per_sec / MILLION;
    }

    return ClockCycles() / cycles_per_us;
}
