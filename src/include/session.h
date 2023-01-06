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

/* must ensure session create, destroy and handling are atomic */
extern pthread_mutex_t device_session_create_mutex;

typedef struct client_session {
    struct client_session *prev, *next;
    struct device_session* device_session;

    queue_t rx_queue;
} client_session_t;

typedef struct device_session {
    struct device_session *prev, *next;
    struct net_device* device;
    struct client_session* root_client_session;

    pthread_attr_t tx_thread_attr;
    pthread_t tx_thread;

    pthread_cond_t cond;
    pthread_mutex_t mutex;

    queue_t tx_queue;

    int queue_stopped;
} device_session_t;

extern device_session_t* root_device_session;
extern device_session_t** device_sessions; // Fast lookup array using dev_id

extern device_session_t* create_device_session (
        struct net_device* dev, const queue_attr_t* tx_attr);

extern void destroy_device_session (device_session_t* D);

extern client_session_t* create_client_session (
        struct net_device* dev, const queue_attr_t* rx_attr);

extern void destroy_client_session (client_session_t* S);

static inline device_session_t* get_last_device_session() {
    device_session_t** last = &root_device_session;

    if (*last == NULL) {
        return NULL;
    }

    while ((*last)->next != NULL) {
        last = &(*last)->next;
    }

    return *last;
}

static inline client_session_t* get_last_client_session (device_session_t* ds) {
    if (ds == NULL) {
        return NULL;
    }

    client_session_t** last = &ds->root_client_session;

    if (*last == NULL) {
        return NULL;
    }

    while ((*last)->next != NULL) {
        last = &(*last)->next;
    }

    return *last;
}

static inline device_session_t* get_device_session (struct net_device* dev) {
    device_session_t** location = &root_device_session;

    while (*location != NULL) {
        if ((*location)->device == dev) {
            return *location;
        }

        location = &(*location)->next;
    }

    return NULL;
}

#endif /* SRC_SESSION_H_ */
