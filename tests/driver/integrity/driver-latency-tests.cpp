/**
 * \file    driver-latency-tests.cpp
 * \brief   Driver micro-level integration tests definition file; basic latency
 *          functionality tests.
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
#include <tests/driver/common/test_devices.h>

extern "C" {
    #include <timer.h>
    #include <dev-can-linux/commands.h>
}

/* Timing tests are very unreliable during Profiling and Coverage so we will
   skip these */
#if PROFILING_BUILD != 1 && COVERAGE_BUILD != 1

TEST( Driver, LatencyFunctionality ) {
    int fd_tx = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd_tx, -1);

    int fd_rx = open(get_device0_rx0().c_str(), O_RDWR);

    EXPECT_NE(fd_rx, -1);

    // set latency limit
    int latency_ret = set_latency_limit_ms(fd_rx, 5);

    EXPECT_EQ(latency_ret, EOK);

    uint32_t start_ms = get_clock_time_us()/1000;

    struct can_msg canmsg_tx1 = {
        .dat = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 },
        .len = 8,
        .mid = 0xABC,
        .ext = {
            .timestamp = start_ms,
            .is_extended_mid = 1,
            .is_remote_frame = 0
        }
    };

    struct can_msg canmsg_tx2 = {
        .dat = { 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xAC },
        .len = 8,
        .mid = 0x123,
        .ext = {
            .timestamp = start_ms,
            .is_extended_mid = 1,
            .is_remote_frame = 0
        }
    };

    struct can_msg canmsg_rx;

    /*
     * Write and read 2 message quickly so the latency check doesn't drop any
     * messages
     */
    int write_ret = write_frame_raw(fd_tx, &canmsg_tx1);

    EXPECT_EQ(write_ret, EOK);

    write_ret = write_frame_raw(fd_tx, &canmsg_tx2);

    EXPECT_EQ(write_ret, EOK);

    int read_ret = read_frame_raw_block(fd_rx, &canmsg_rx);

    EXPECT_EQ(read_ret, EOK);

    EXPECT_EQ(canmsg_rx.dat[0], 0x11);
    EXPECT_EQ(canmsg_rx.dat[1], 0x22);
    EXPECT_EQ(canmsg_rx.dat[2], 0x33);
    EXPECT_EQ(canmsg_rx.dat[3], 0x44);
    EXPECT_EQ(canmsg_rx.dat[4], 0x55);
    EXPECT_EQ(canmsg_rx.dat[5], 0x66);
    EXPECT_EQ(canmsg_rx.dat[6], 0x77);
    EXPECT_EQ(canmsg_rx.dat[7], 0x88);
    EXPECT_EQ(canmsg_rx.len, 8);
    EXPECT_EQ(canmsg_rx.mid, 0xABC);
    EXPECT_GE(canmsg_rx.ext.timestamp - start_ms, 0);
    EXPECT_EQ(canmsg_rx.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg_rx.ext.is_remote_frame, 0);

    read_ret = read_frame_raw_block(fd_rx, &canmsg_rx);

    EXPECT_EQ(read_ret, EOK);

    EXPECT_EQ(canmsg_rx.dat[0], 0x99);
    EXPECT_EQ(canmsg_rx.dat[1], 0xAA);
    EXPECT_EQ(canmsg_rx.dat[2], 0xBB);
    EXPECT_EQ(canmsg_rx.dat[3], 0xCC);
    EXPECT_EQ(canmsg_rx.dat[4], 0xDD);
    EXPECT_EQ(canmsg_rx.dat[5], 0xEE);
    EXPECT_EQ(canmsg_rx.dat[6], 0xFF);
    EXPECT_EQ(canmsg_rx.dat[7], 0xAC);
    EXPECT_EQ(canmsg_rx.len, 8);
    EXPECT_EQ(canmsg_rx.mid, 0x123);
    EXPECT_GE(canmsg_rx.ext.timestamp - start_ms, 0);
    EXPECT_EQ(canmsg_rx.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg_rx.ext.is_remote_frame, 0);

    /*
     * Now write 2 messages again with some delays
     */
    write_ret = write_frame_raw(fd_tx, &canmsg_tx1);

    EXPECT_EQ(write_ret, EOK);

    usleep(6000); // sleep for 6ms when latency limit is 5ms

    write_ret = write_frame_raw(fd_tx, &canmsg_tx2);

    EXPECT_EQ(write_ret, EOK);

    read_ret = read_frame_raw_block(fd_rx, &canmsg_rx);

    EXPECT_EQ(read_ret, EOK);

    // second write message should be received

    EXPECT_EQ(canmsg_rx.dat[0], 0x99);
    EXPECT_EQ(canmsg_rx.dat[1], 0xAA);
    EXPECT_EQ(canmsg_rx.dat[2], 0xBB);
    EXPECT_EQ(canmsg_rx.dat[3], 0xCC);
    EXPECT_EQ(canmsg_rx.dat[4], 0xDD);
    EXPECT_EQ(canmsg_rx.dat[5], 0xEE);
    EXPECT_EQ(canmsg_rx.dat[6], 0xFF);
    EXPECT_EQ(canmsg_rx.dat[7], 0xAC);
    EXPECT_EQ(canmsg_rx.len, 8);
    EXPECT_EQ(canmsg_rx.mid, 0x123);
    EXPECT_GE(canmsg_rx.ext.timestamp - start_ms, 6);
    EXPECT_EQ(canmsg_rx.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg_rx.ext.is_remote_frame, 0);

    close(fd_tx);
    close(fd_rx);
}

#endif
