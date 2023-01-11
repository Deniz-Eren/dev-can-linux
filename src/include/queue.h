/*
 * \file    queue.h
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

#ifndef SRC_QUEUE_H_
#define SRC_QUEUE_H_

#include <pthread.h>
#include <sys/can_dcmd.h>


typedef struct queue_attr {
    int size;
} queue_attr_t;

typedef struct queue {
    queue_attr_t attr;

    struct can_msg* data;
    int begin, end;

    pthread_cond_t cond;
    pthread_mutex_t mutex;

    volatile int session_up;
    volatile int dequeue_waiting;
} queue_t;


extern int create_queue (queue_t* Q, const queue_attr_t* attr);
extern void destroy_queue (queue_t* Q);
extern int enqueue (queue_t* Q, struct can_msg* msg);
extern struct can_msg* dequeue (queue_t* Q, uint32_t latency_limit_ms);
extern struct can_msg* dequeue_noblock (queue_t* Q, uint32_t latency_limit_ms);
extern struct can_msg* dequeue_peek (queue_t* Q);
extern struct can_msg* dequeue_peek_noblock (queue_t* Q);


#endif /* SRC_QUEUE_H_ */
