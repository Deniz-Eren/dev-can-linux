/*
 * \file    queue.c
 *
 * \details Circular queue for handling CAN messages of QNX type can_msg.
 *          Multi-thread safe, blocks on dequeue and implements nice shutdown on
 *          destroy_queue() call.
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

#include <stdlib.h>

#include "queue.h"


int create_queue (queue_t* Q, const queue_attr_t* attr) {
    int result;

    if (Q == NULL || attr == NULL) {
        return EFAULT; // Bad address
    }

    Q->session_up = 0;
    Q->dequeue_waiting = 0;

    if (attr->size == 0) {
	return EINVAL; // Invalid argument
    }

    if ((result = pthread_mutex_init(&Q->mutex, NULL)) != EOK) {
        return result;
    }

    if ((result = pthread_cond_init(&Q->cond, NULL)) != EOK) {
        pthread_mutex_destroy(&Q->mutex);

        return result;
    }

    Q->attr = *attr;
    Q->begin = Q->end = 0;

    if ((Q->data = malloc(Q->attr.size*sizeof(struct can_msg))) == NULL) {
        pthread_mutex_destroy(&Q->mutex);
        pthread_cond_destroy(&Q->cond);

        return ENOMEM; // Not enough memory
    }

    Q->session_up = 1;
    Q->dequeue_waiting = 0;

    return EOK;
}

void destroy_queue (queue_t* Q) {
    if (Q == NULL) {
        return;
    }

    pthread_mutex_lock(&Q->mutex);

    Q->session_up = 0;

    pthread_cond_signal(&Q->cond);
    pthread_mutex_unlock(&Q->mutex);

    pthread_mutex_lock(&Q->mutex);
    while (Q->dequeue_waiting) {
        pthread_cond_wait(&Q->cond, &Q->mutex);
    }

    free(Q->data);
    Q->attr.size = 0;
    Q->begin = Q->end = 0;

    pthread_mutex_destroy(&Q->mutex);
    pthread_cond_destroy(&Q->cond);

    // Notice we never unlocked the mutex, since we know the dequeue() is not
    // waiting and we are in the process of destroying the session.
    // No need: pthread_mutex_unlock(&Q->mutex);
}

int enqueue (queue_t* Q, struct can_msg* msg) {
    if (Q == NULL || msg == NULL) {
        return EFAULT; // Bad address
    }

    if (Q->session_up == 0) {
        return EPIPE; // Broken pipe
    }

    pthread_mutex_lock(&Q->mutex);

    // handle data insertion to queue here

    if (Q->end == Q->attr.size) {
        Q->end = 0;

        if (Q->begin == 0) {
            Q->begin = 2; // 2 messages lost if we use the entire queue size and
                          // wrap around. Since we don't track the wrap around
                          // when Q->begin == Q->end (our empty queue condition)
                          // would be the same as the wrapped around condition.
                          // Thus we must lose 2 of the oldest messages in this
                          // scenario.
        }
    }
    else if (Q->begin == Q->end+1) {
        if (Q->begin+1 == Q->attr.size) {
            Q->begin = 0;
        }
        else {
            ++Q->begin; // 1 message lost if we use the entire queue size but did
                        // not wrap around; oldest message.
        }
    }

    Q->data[Q->end] = *msg;
    ++Q->end;

    pthread_cond_signal(&Q->cond);
    pthread_mutex_unlock(&Q->mutex);

    return EOK;
}

struct can_msg* dequeue (queue_t* Q) {
    if (Q == NULL) {
        return NULL;
    }

    if (Q->session_up == 0) {
        return NULL;
    }

    struct can_msg* result = NULL;

    pthread_mutex_lock(&Q->mutex);

    Q->dequeue_waiting = 1;
    while (Q->session_up == 1 && Q->begin == Q->end) {
        pthread_cond_wait(&Q->cond, &Q->mutex);
    }
    Q->dequeue_waiting = 0;

    if (Q->session_up == 0) {
        pthread_cond_signal(&Q->cond);
        pthread_mutex_unlock(&Q->mutex);

        return NULL;
    }

    // handle data in queue here, i.e. when Q->begin != Q-end
    result = &Q->data[Q->begin];

    ++Q->begin;

    if (Q->begin == Q->attr.size) {
        Q->begin = 0;

        if (Q->end == Q->attr.size) {
            Q->end = 0;
        }
    }

    pthread_mutex_unlock(&Q->mutex);

    return result;
}

