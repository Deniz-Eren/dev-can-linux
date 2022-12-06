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


static struct timer_record_t {
    struct net_device *dev;

    timer_t* id;

    void (*callback)(unsigned long);
    unsigned long data;
    int num_callbacks;

    struct sigevent event;
    int chid;

    int created;

    pthread_attr_t attr;
} timer[MAX_DEVICES];

static int n_timers = 0;

static void* timer_loop (void* arg);


void setup_timer (timer_t* timer_id, void (*callback)(unsigned long),
        unsigned long data)
{
    int i = n_timers;

    timer[i].dev = (struct net_device *)data;
    timer[i].id = timer_id;

    timer[i].callback = callback;
    timer[i].data = data;

    timer_delete(*timer_id);

    struct sched_param scheduling_params;
    int prio;

    timer[i].chid = ChannelCreate(0);

    /* Get our priority. */
    if (SchedGet( 0, 0, &scheduling_params) != -1)
    {
       prio = scheduling_params.sched_priority;
    }
    else
    {
       prio = 10;
    }

    timer[i].created = 0;
    timer[i].event.sigev_notify = SIGEV_PULSE;
    timer[i].event.sigev_coid = ConnectAttach(ND_LOCAL_NODE, 0,
                                     timer[i].chid,
                                     _NTO_SIDE_CHANNEL, 0);
    timer[i].event.sigev_priority = prio;
    timer[i].event.sigev_code = TIMER_PULSE_CODE;
    timer_create(CLOCK_MONOTONIC, &timer[i].event, timer_id);

    /* Start the timer thread */
    pthread_attr_init(&timer[i].attr);
    pthread_attr_setdetachstate(&timer[i].attr, PTHREAD_CREATE_DETACHED);
    pthread_create(NULL, &timer[i].attr, &timer_loop, &n_timers);

    ++n_timers;
}

static void* timer_loop (void*  arg) {
    int i = *(int*)arg;

    int             rcvid;
    struct _pulse   pulse;

    for (;;) {
        rcvid = MsgReceive(timer[i].chid, &pulse, sizeof(struct _pulse), NULL);
        if (rcvid == 0) { /* we got a pulse */
             if (pulse.code == TIMER_PULSE_CODE) {
                 timer[i].callback(timer[i].data);
             } /* else other pulses ... */
        } /* else other messages ... */
    }

    return EXIT_SUCCESS;
}

void del_timer_sync (timer_t* timer_id)
{
    timer_delete(*timer_id);
}

void mod_timer (timer_t* timer_id, int ticks)
{
    int i = -1, j;

    for (j = 0; j < n_timers; ++j) {
        if (timer[j].id == timer_id) {
            i = j;
            break;
        }
    }

    if (i == -1) {
        log_err("mod_timer unknown timer\n");
        return;
    }

    if (timer[i].created) {
        timer_delete(*timer[i].id);
    }

    struct itimerspec       itime;

    // first trigger, seconds:
    itime.it_value.tv_sec = 0;
    // first trigger, 0.1s in nanoseconds:
    itime.it_value.tv_nsec = TIMER_INTERVAL_NS * ticks;

    // interval between triggers, seconds:
    itime.it_interval.tv_sec = 0;
    // interval between triggers, nanoseconds:
    itime.it_interval.tv_nsec = 0;
    timer_settime(*timer[i].id, 0, &itime, NULL);

    timer[i].created = 1;
}
