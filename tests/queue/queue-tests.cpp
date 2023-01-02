/**
 * \file    queue-tests.cpp
 * \brief   Queue implementation test definition file
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

#include <gtest/gtest.h>


extern "C" {
    #include <queue.h>
}

void* receive_loop (void* arg) {
    queue_t* queue = (queue_t*)arg;

    struct can_msg* dequeue_can_msg = dequeue(queue, 0);

    pthread_exit(dequeue_can_msg);
}

void* peek_receive_loop (void* arg) {
    queue_t* queue = (queue_t*)arg;

    struct can_msg* dequeue_can_msg = dequeue_peek(queue);

    pthread_exit(dequeue_can_msg);
}

TEST( Queue, ZeroQueueSize ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 0
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* OK to have null queue */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    can_msg msg;
    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EDOM /* Domain error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, &receive_loop, &queue);

    usleep(5000);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 1);

    msg.mid = 0x112233;
    msg.len = 0;

    enqueue_code = enqueue(&queue, &msg);

    void* value_ptr;
    pthread_join(thread, &value_ptr);

    EXPECT_EQ(enqueue_code, EDOM /* Domain error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg = (struct can_msg*)value_ptr;

    EXPECT_EQ(dequeue_can_msg, nullptr);

    destroy_queue(&queue);

    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 0);
    EXPECT_EQ(queue.dequeue_waiting, 0);
}

TEST( Queue, PeekZeroQueueSize ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 0
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* OK to have null queue */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    can_msg msg;
    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EDOM /* Domain error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, &peek_receive_loop, &queue);

    usleep(5000);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 1);

    msg.mid = 0x112233;
    msg.len = 0;

    enqueue_code = enqueue(&queue, &msg);

    void* value_ptr;
    pthread_join(thread, &value_ptr);

    EXPECT_EQ(enqueue_code, EDOM /* Domain error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg = (struct can_msg*)value_ptr;

    EXPECT_EQ(dequeue_can_msg, nullptr);

    destroy_queue(&queue);

    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 0);
    EXPECT_EQ(queue.dequeue_waiting, 0);
}

TEST( Queue, NonBlockZeroQueueSize ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 0
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* OK to have null queue */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    can_msg msg;
    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EDOM /* Domain error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg = dequeue_nonblock(&queue, 0);

    EXPECT_EQ(dequeue_can_msg, nullptr);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, NullInput ) {
    queue_t queue;
    queue_attr_t attr;

    int create_queue_code = create_queue(NULL, &attr);

    EXPECT_EQ(create_queue_code, EFAULT /* Bad address */);

    create_queue_code = create_queue(&queue, NULL);

    EXPECT_EQ(create_queue_code, EFAULT /* Bad address */);

    create_queue_code = create_queue(NULL, NULL);

    EXPECT_EQ(create_queue_code, EFAULT /* Bad address */);

    can_msg msg;
    int enqueue_code = enqueue(NULL, &msg);
    EXPECT_EQ(enqueue_code, EFAULT /* Bad address */);

    enqueue_code = enqueue(&queue, NULL);
    EXPECT_EQ(enqueue_code, EFAULT /* Bad address */);

    enqueue_code = enqueue(NULL, NULL);
    EXPECT_EQ(create_queue_code, EFAULT /* Bad address */);

    can_msg* dequeue_ret = dequeue(NULL, 0);
    EXPECT_EQ(dequeue_ret, nullptr);

    dequeue_ret = dequeue_nonblock(NULL, 0);
    EXPECT_EQ(dequeue_ret, nullptr);

    dequeue_ret = dequeue_peek(NULL);
    EXPECT_EQ(dequeue_ret, nullptr);
}

TEST( Queue, SimpleUse ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    can_msg msg;
    msg.mid = 0x112233;
    msg.len = 0;

    struct can_msg* dequeue_can_msg = dequeue_nonblock(&queue, 0);

    EXPECT_EQ(dequeue_can_msg, nullptr);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    msg.mid = 0x445566;
    msg.len = 2;
    msg.dat[0] = 0x77;
    msg.dat[1] = 0x88;

    enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    enqueue_code = enqueue(&queue, &msg);
    dequeue_can_msg = dequeue_nonblock(&queue, 0);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(dequeue_can_msg->mid, 0x112233);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 1);
    EXPECT_EQ(queue.end, 3);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 0x445566);
    EXPECT_EQ(dequeue_can_msg->len, 2);
    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 3);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue_peek(&queue);

    EXPECT_EQ(dequeue_can_msg->mid, 0x445566);
    EXPECT_EQ(dequeue_can_msg->len, 2);
    EXPECT_EQ(dequeue_can_msg->dat[0], 0x77);
    EXPECT_EQ(dequeue_can_msg->dat[1], 0x88);
    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 3);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 0x445566);
    EXPECT_EQ(dequeue_can_msg->len, 2);
    EXPECT_EQ(dequeue_can_msg->dat[0], 0x77);
    EXPECT_EQ(dequeue_can_msg->dat[1], 0x88);
    EXPECT_EQ(queue.begin, 3);
    EXPECT_EQ(queue.end, 3);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, WrapBeforeAnyDequeue ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    for (int i = 0; i < 10; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, 0);
        EXPECT_EQ(queue.end, i+1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 3);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 103);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 4);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 104);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 5);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, PushBeginToWrap ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 5
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 5);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    int begin_vals[11] = {
        0, 0, 0, 0, 0,
        2, 3, 4, 0, 0,
        2 };

    for (int i = 0; i < 11; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, begin_vals[i]);
        EXPECT_EQ(queue.end, i%5+1);
        EXPECT_EQ(queue.attr.size, 5);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    struct can_msg* dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 107);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 3);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 5);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, WrapAfterSomeDequeue ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    enqueue(&queue, &msg);
    enqueue(&queue, &msg);
    dequeue(&queue, 0);
    dequeue(&queue, 0);

    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    for (int i = 0; i < 8; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, 2);
        EXPECT_EQ(queue.end, i+3);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 3);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 101);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 4);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 102);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 5);
    EXPECT_EQ(queue.end, 2);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, WrapEnqueueThenWrapDequeue ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    for (int i = 0; i < 10; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, 0);
        EXPECT_EQ(queue.end, i+1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    msg.mid = 1000;
    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg;

    for (int i = 0; i < 7; ++i) {
        dequeue_can_msg = dequeue(&queue, 0);

        EXPECT_EQ(dequeue_can_msg->mid, 102+i);
        EXPECT_EQ(dequeue_can_msg->len, 0);
        EXPECT_EQ(queue.begin, 3+i);
        EXPECT_EQ(queue.end, 1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 109);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 1000);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 1);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, WrapEnqueueThenWrapDequeueNonBlock ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    for (int i = 0; i < 10; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, 0);
        EXPECT_EQ(queue.end, i+1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    msg.mid = 1000;
    int enqueue_code = enqueue(&queue, &msg);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 2);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg;

    for (int i = 0; i < 7; ++i) {
        dequeue_can_msg = dequeue_nonblock(&queue, 0);

        EXPECT_EQ(dequeue_can_msg->mid, 102+i);
        EXPECT_EQ(dequeue_can_msg->len, 0);
        EXPECT_EQ(queue.begin, 3+i);
        EXPECT_EQ(queue.end, 1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    dequeue_can_msg = dequeue_nonblock(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 109);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    dequeue_can_msg = dequeue_nonblock(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 1000);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 1);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, DequeueAllFromFullQueue ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    for (int i = 0; i < 10; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, 0);
        EXPECT_EQ(queue.end, i+1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    for (int i = 0; i < 9; ++i) {
        struct can_msg* dequeue_can_msg = dequeue(&queue, 0);

        EXPECT_EQ(dequeue_can_msg->mid, 100+i);
        EXPECT_EQ(dequeue_can_msg->len, 0);
        EXPECT_EQ(queue.begin, i+1);
        EXPECT_EQ(queue.end, 10);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    struct can_msg* dequeue_can_msg = dequeue(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 109);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, DequeueNonBlockAllFromFullQueue ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    struct can_msg msg;

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    for (int i = 0; i < 10; ++i) {
        msg.mid = 100 + i;
        msg.len = 0;

        int enqueue_code = enqueue(&queue, &msg);

        EXPECT_EQ(enqueue_code, EOK /* No error */);
        EXPECT_EQ(queue.begin, 0);
        EXPECT_EQ(queue.end, i+1);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    for (int i = 0; i < 9; ++i) {
        struct can_msg* dequeue_can_msg = dequeue_nonblock(&queue, 0);

        EXPECT_EQ(dequeue_can_msg->mid, 100+i);
        EXPECT_EQ(dequeue_can_msg->len, 0);
        EXPECT_EQ(queue.begin, i+1);
        EXPECT_EQ(queue.end, 10);
        EXPECT_EQ(queue.attr.size, 10);
        EXPECT_EQ(queue.session_up, 1);
        EXPECT_EQ(queue.dequeue_waiting, 0);
    }

    struct can_msg* dequeue_can_msg = dequeue_nonblock(&queue, 0);

    EXPECT_EQ(dequeue_can_msg->mid, 109);
    EXPECT_EQ(dequeue_can_msg->len, 0);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    destroy_queue(&queue);
}

TEST( Queue, BlockingDequeue ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, &receive_loop, &queue);

    usleep(5000);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 1);

    can_msg msg;
    msg.mid = 0x112233;
    msg.len = 0;

    int enqueue_code = enqueue(&queue, &msg);

    void* value_ptr;
    pthread_join(thread, &value_ptr);

    EXPECT_EQ(enqueue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 1);
    EXPECT_EQ(queue.end, 1);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    struct can_msg* dequeue_can_msg = (struct can_msg*)value_ptr;

    EXPECT_EQ(dequeue_can_msg->mid, 0x112233);
    EXPECT_EQ(dequeue_can_msg->len, 0);

    destroy_queue(&queue);
}

TEST( Queue, DestroyWhileDequeueBlocks ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, &receive_loop, &queue);

    usleep(5000);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 1);

    destroy_queue(&queue);

    void* value_ptr;
    pthread_join(thread, &value_ptr);

    EXPECT_EQ(value_ptr, nullptr);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 0);
    EXPECT_EQ(queue.dequeue_waiting, 0);
}

TEST( Queue, DestroyWhilePeekDequeueBlocks ) {
    queue_t queue = {
        .begin = -1,
        .end = -1,
        .session_up = -1,
        .dequeue_waiting = -1
    };

    queue_attr_t attr = {
        .size = 10
    };

    int create_queue_code = create_queue(&queue, &attr);

    EXPECT_EQ(create_queue_code, EOK /* No error */);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 0);

    pthread_t thread;
    pthread_create(&thread, NULL, &peek_receive_loop, &queue);

    usleep(5000);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 10);
    EXPECT_EQ(queue.session_up, 1);
    EXPECT_EQ(queue.dequeue_waiting, 1);

    destroy_queue(&queue);

    void* value_ptr;
    pthread_join(thread, &value_ptr);

    EXPECT_EQ(value_ptr, nullptr);
    EXPECT_EQ(queue.begin, 0);
    EXPECT_EQ(queue.end, 0);
    EXPECT_EQ(queue.attr.size, 0);
    EXPECT_EQ(queue.session_up, 0);
    EXPECT_EQ(queue.dequeue_waiting, 0);
}
