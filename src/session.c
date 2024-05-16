/*
 * \file    session.c
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

#include <linux/netdevice.h>

#include "session.h"
#include "netif.h"
#include "interrupt.h"

device_session_t* root_device_session = NULL;

/* must ensure session create, destroy and handling are atomic */
pthread_mutex_t device_session_create_mutex = PTHREAD_MUTEX_INITIALIZER;


void increment_dropped_packet (void* arg) {
    if (arg == NULL) {
        return;
    }

    unsigned long* count = (unsigned long*)arg;

    ++(*count);
}

device_session_t*
create_device_session (struct net_device* dev, const queue_attr_t* tx_attr) {
    pthread_mutex_lock(&device_session_create_mutex);

    device_session_t* new_device = malloc(sizeof(device_session_t));

    device_session_t* last = get_last_device_session();

    if (last == NULL) {
        root_device_session = new_device;
        new_device->prev = new_device->next = NULL;
    }
    else {
        last->next = new_device;
        new_device->prev = last;
        new_device->next = NULL;
    }

    new_device->device = dev;
    new_device->root_client_session = NULL;
    new_device->queue_stopped = 0;

    int err;
    if ((err = create_queue(&new_device->tx_queue, tx_attr)) != EOK) {
        log_err("create_device_session fail: create_queue err: %d\n", err);

        destroy_device_session(new_device);
        pthread_mutex_unlock(&device_session_create_mutex);
        return NULL;
    }

    new_device->tx_queue.dropped_packet_arg = &dev->stats.tx_dropped;
    new_device->tx_queue.dropped_packet = increment_dropped_packet;

    int policy;
    struct sched_param param;

    err = pthread_getschedparam(pthread_self(), &policy, &param);

    if (err != EOK) {
        log_err("error pthread_getschedparam: %s\n", strerror(err));

        return NULL;
    }

    pthread_attr_init(&new_device->tx_thread_attr);
    pthread_attr_setdetachstate(
            &new_device->tx_thread_attr, PTHREAD_CREATE_DETACHED );

    pthread_attr_setinheritsched( &new_device->tx_thread_attr,
            PTHREAD_EXPLICIT_SCHED );

    param.sched_priority += CONFIG_IRQ_SCHED_PRIORITY_BOOST;
    pthread_attr_setschedparam(&new_device->tx_thread_attr, &param);

    pthread_create( &new_device->tx_thread, &new_device->tx_thread_attr,
            &netif_tx, new_device );

    pthread_mutex_unlock(&device_session_create_mutex);
    return new_device;
}

void destroy_device_session (device_session_t* D) {
    if (D == NULL) {
        return;
    }

    pthread_mutex_lock(&device_session_create_mutex);

    if (D->prev && D->next) {
        D->prev->next = D->next;
        D->next->prev = D->prev;
    }
    else if (D->prev) {
        D->prev->next = NULL;
    }
    else if (D->next) {
        D->next->prev = NULL;

        /* Since this node does not have a previous node, it must be the root
         * node. Therefore when it is destroyed the new root node must then be
         * the next node. */
        root_device_session = D->next;
    }
    else {
        /* Since this node does not have a previous or a next node, it must be
         * the root and last remaining node. Therefore when it is destroyed the
         * root node must be set to NULL. */

        root_device_session = NULL;
    }

    destroy_queue(&D->tx_queue);
    D->queue_stopped = 0;

    free(D);
}

client_session_t*
create_client_session (struct net_device* dev, const queue_attr_t* rx_attr,
        uint32_t* mid, uint32_t* mfilter, uint32_t* prio)
{
    if (dev == NULL) {
        return NULL;
    }

    device_session_t* ds = get_device_session(dev);

    if (ds == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&device_session_create_mutex);
    client_session_t* new_client = malloc(sizeof(client_session_t));

    client_session_t* last = get_last_client_session(ds);

    if (last == NULL) {
        ds->root_client_session = new_client;
        new_client->prev = new_client->next = NULL;
    }
    else {
        last->next = new_client;
        new_client->prev = last;
        new_client->next = NULL;
    }

    new_client->device_session = ds;
    new_client->mid = mid;          /* CAN message identifier */
    new_client->mfilter = mfilter;  /* CAN message filter */
    new_client->prio = prio;        /* CAN priority - not used */

    int err;
    if ((err = create_queue(&new_client->rx_queue, rx_attr)) != EOK) {
        log_err("create_client_session fail: create_queue err: %d\n", err);

        destroy_client_session(new_client);
        pthread_mutex_unlock(&device_session_create_mutex);
        return NULL;
    }

    new_client->rx_queue.dropped_packet_arg = &dev->stats.rx_dropped;
    new_client->rx_queue.dropped_packet = increment_dropped_packet;

    pthread_mutex_unlock(&device_session_create_mutex);
    return new_client;
}

void destroy_client_session (client_session_t* S) {
    if (S == NULL) {
        return;
    }

    device_session_t* ds = S->device_session;

    if (ds == NULL) {
        return;
    }

    pthread_mutex_lock(&device_session_create_mutex);

    if (S->prev && S->next) {
        S->prev->next = S->next;
        S->next->prev = S->prev;
    }
    else if (S->prev) {
        S->prev->next = NULL;
    }
    else if (S->next) {
        S->next->prev = NULL;

        /* Since this node does not have a previous node, it must be the root
         * node. Therefore when it is destroyed the new root node must then be
         * the next node. */
        ds->root_client_session = S->next;
    }
    else {
        /* Since this node does not have a previous or a next node, it must be
         * the root and last remaining node. Therefore when it is destroyed the
         * root node must be set to NULL. */

        ds->root_client_session = NULL;
    }

    destroy_queue(&S->rx_queue);
    free(S);

    pthread_mutex_unlock(&device_session_create_mutex);
}
