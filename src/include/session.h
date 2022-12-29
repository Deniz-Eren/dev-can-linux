/*
 * \file    session.h
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

#ifndef SRC_SESSION_H_
#define SRC_SESSION_H_

#include <config.h>
#include <queue.h>

#define MAX_SESSIONS 256

typedef struct session {
    struct net_device* device;

    queue_t rx, tx;
} session_t;

typedef struct device_sessions {
    session_t sessions[MAX_SESSIONS];

    int num_sessions;
} device_sessions_t;

extern device_sessions_t device_sessions[MAX_DEVICES];

extern int create_session (session_t* S, struct net_device* dev,
        const queue_attr_t* rx_attr,
        const queue_attr_t* tx_attr);

extern void destroy_session (session_t* S);

#endif /* SRC_SESSION_H_ */
