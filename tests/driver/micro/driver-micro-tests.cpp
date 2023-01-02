/**
 * \file    driver-micro-tests.cpp
 * \brief   Driver micro-level integration tests definition file; basic single
 *          send and receive testing.
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
#include <gtest/gtest.h>

extern "C" {
    #include <timer.h>
    #include <dev-can-linux/commands.h>
}

void* receive_loop0 (void* arg) {
    struct can_msg* canmsg = (struct can_msg*)arg;

    int fd = open("/dev/can0/rx0", O_RDWR);

    if (read_canmsg_ext(fd, canmsg) == EOK) {
        close(fd);
        pthread_exit(canmsg);
    }

    pthread_exit(NULL);
}

void* receive_loop1 (void* arg) {
    struct can_msg* canmsg = (struct can_msg*)arg;

    int fd = open("/dev/can1/rx0", O_RDWR);

    if (read_canmsg_ext(fd, canmsg) == EOK) {
        close(fd);
        pthread_exit(canmsg);
    }

    pthread_exit(NULL);
}

TEST( Driver, SingleSendReceive ) {
    int fd0 = open("/dev/can0/tx0", O_RDWR);

    EXPECT_NE(fd0, -1);

    int fd1 = open("/dev/can1/tx0", O_RDWR);

    EXPECT_NE(fd1, -1);

    uint32_t start_ms = get_clock_time_us()/1000;

    struct can_msg canmsg = {
        .dat = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
        .len = 8,
        .mid = 0xABC,
        .ext = {
            .timestamp = start_ms,
            .is_extended_mid = 1,
            .is_remote_frame = 0
        }
    };

    struct can_msg canmsg0, canmsg1;

    pthread_t thread0;
    pthread_create(&thread0, NULL, &receive_loop0, &canmsg0);

    pthread_t thread1;
    pthread_create(&thread1, NULL, &receive_loop1, &canmsg1);

    usleep(3000);
    int write_ret = write_canmsg_ext(fd0, &canmsg);

    EXPECT_EQ(write_ret, EOK);

    void* exit_ptr0;
    pthread_join(thread0, &exit_ptr0);

    EXPECT_EQ(exit_ptr0, &canmsg0);
    EXPECT_EQ(canmsg0.dat[0], 0x11);
    EXPECT_EQ(canmsg0.dat[1], 0x22);
    EXPECT_EQ(canmsg0.dat[2], 0x33);
    EXPECT_EQ(canmsg0.dat[3], 0x44);
    EXPECT_EQ(canmsg0.dat[4], 0x55);
    EXPECT_EQ(canmsg0.dat[5], 0x66);
    EXPECT_EQ(canmsg0.dat[6], 0x77);
    EXPECT_EQ(canmsg0.dat[7], 0x88);
    EXPECT_EQ(canmsg0.len, 8);
    EXPECT_EQ(canmsg0.mid, 0xABC);
    EXPECT_GE(canmsg0.ext.timestamp - start_ms, 3);
    EXPECT_LE(canmsg0.ext.timestamp - start_ms, 5);
    EXPECT_EQ(canmsg0.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg0.ext.is_remote_frame, 0);

    usleep(3000);
    write_ret = write_canmsg_ext(fd1, &canmsg);

    EXPECT_EQ(write_ret, EOK);

    void* exit_ptr1;
    pthread_join(thread1, &exit_ptr1);

    EXPECT_EQ(exit_ptr1, &canmsg1);
    EXPECT_EQ(canmsg1.dat[0], 0x11);
    EXPECT_EQ(canmsg1.dat[1], 0x22);
    EXPECT_EQ(canmsg1.dat[2], 0x33);
    EXPECT_EQ(canmsg1.dat[3], 0x44);
    EXPECT_EQ(canmsg1.dat[4], 0x55);
    EXPECT_EQ(canmsg1.dat[5], 0x66);
    EXPECT_EQ(canmsg1.dat[6], 0x77);
    EXPECT_EQ(canmsg1.dat[7], 0x88);
    EXPECT_EQ(canmsg1.len, 8);
    EXPECT_EQ(canmsg1.mid, 0xABC);
    EXPECT_GE(canmsg1.ext.timestamp - start_ms, 6);
    EXPECT_LE(canmsg1.ext.timestamp - start_ms, 10);
    EXPECT_EQ(canmsg1.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg1.ext.is_remote_frame, 0);

    close(fd0);
    close(fd1);
}

TEST( Driver, SingleSendMultiReceive ) {
    int fd = open("/dev/can0/tx0", O_RDWR);

    EXPECT_NE(fd, -1);

    uint32_t start_ms = get_clock_time_us()/1000;

    struct can_msg canmsg = {
        .dat = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
        .len = 8,
        .mid = 0xABC,
        .ext = {
            .timestamp = start_ms,
            .is_extended_mid = 1,
            .is_remote_frame = 0
        }
    };

    struct can_msg canmsg0, canmsg1;

    pthread_attr_t attr0;
    pthread_t thread0;
    pthread_attr_init(&attr0);
    pthread_attr_setdetachstate(&attr0, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread0, NULL, &receive_loop0, &canmsg0);

    pthread_attr_t attr1;
    pthread_t thread1;
    pthread_attr_init(&attr1);
    pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
    pthread_create(&thread1, NULL, &receive_loop0, &canmsg1);
    usleep(3000);

    int write_ret = write_canmsg_ext(fd, &canmsg);

    EXPECT_EQ(write_ret, EOK);

    void *exit_ptr0, *exit_ptr1;
    pthread_join(thread0, &exit_ptr0);
    pthread_join(thread1, &exit_ptr1);

    EXPECT_EQ(exit_ptr0, &canmsg0);
    EXPECT_EQ(canmsg0.dat[0], 0x11);
    EXPECT_EQ(canmsg0.dat[1], 0x22);
    EXPECT_EQ(canmsg0.dat[2], 0x33);
    EXPECT_EQ(canmsg0.dat[3], 0x44);
    EXPECT_EQ(canmsg0.dat[4], 0x55);
    EXPECT_EQ(canmsg0.dat[5], 0x66);
    EXPECT_EQ(canmsg0.dat[6], 0x77);
    EXPECT_EQ(canmsg0.dat[7], 0x88);
    EXPECT_EQ(canmsg0.len, 8);
    EXPECT_EQ(canmsg0.mid, 0xABC);
    EXPECT_GE(canmsg0.ext.timestamp - start_ms, 3);
    EXPECT_LE(canmsg0.ext.timestamp - start_ms, 5);
    EXPECT_EQ(canmsg0.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg0.ext.is_remote_frame, 0);

    EXPECT_EQ(exit_ptr1, &canmsg1);
    EXPECT_EQ(canmsg1.dat[0], 0x11);
    EXPECT_EQ(canmsg1.dat[1], 0x22);
    EXPECT_EQ(canmsg1.dat[2], 0x33);
    EXPECT_EQ(canmsg1.dat[3], 0x44);
    EXPECT_EQ(canmsg1.dat[4], 0x55);
    EXPECT_EQ(canmsg1.dat[5], 0x66);
    EXPECT_EQ(canmsg1.dat[6], 0x77);
    EXPECT_EQ(canmsg1.dat[7], 0x88);
    EXPECT_EQ(canmsg1.len, 8);
    EXPECT_EQ(canmsg1.mid, 0xABC);
    EXPECT_GE(canmsg1.ext.timestamp - start_ms, 3);
    EXPECT_LE(canmsg1.ext.timestamp - start_ms, 5);
    EXPECT_EQ(canmsg1.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg1.ext.is_remote_frame, 0);

    close(fd);
}
