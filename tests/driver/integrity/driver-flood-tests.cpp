/**
 * \file    driver-flood-tests.cpp
 * \brief   CMake listing file for driver flood TX tests; this tests the drivers
 *          ability to queue message and talk to the hardware while feeding TX
 *          messages under flood conditions without triggering hardware fault
 *          interrupts.
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


/* Flood tests are unreliable and time consuming during Profiling so we will
 * skip these */
#if PROFILING_BUILD != 1

#define FLOOD_TEST_SIZE 1024

static volatile bool receive_loop0_started = false, receive_loop1_started = false;
static volatile bool receive_loop0_stop = false, receive_loop1_stop = false;

static struct can_msg record0[FLOOD_TEST_SIZE];
static volatile size_t record0_size = 0;

static struct can_msg record1[FLOOD_TEST_SIZE];
static volatile size_t record1_size = 0;

void* receive_loop0 (void* arg) {
    struct can_msg canmsg;

    int fd = open(get_device0_rx0().c_str(), O_RDWR);

    // set latency limit to 0 (i.e. no limit)
    set_latency_limit_ms(fd, 0);

    receive_loop0_started = true;

    while (!receive_loop0_stop) {
        if (read_frame_raw_block(fd, &canmsg) == EOK) {
            record0[record0_size++] = canmsg;
        }
    }

    while (read_frame_raw_noblock(fd, &canmsg) != EAGAIN) {
        record0[record0_size++] = canmsg;
    }

    close(fd);
    pthread_exit(NULL);
}

void* receive_loop1 (void* arg) {
    struct can_msg canmsg;

    int fd = open(get_device1_rx0().c_str(), O_RDWR);

    // set latency limit to 0 (i.e. no limit)
    set_latency_limit_ms(fd, 0);

    receive_loop1_started = true;

    while (!receive_loop1_stop) {
        if (read_frame_raw_block(fd, &canmsg) == EOK) {
            record1[record1_size++] = canmsg;
        }
    }

    while (read_frame_raw_noblock(fd, &canmsg) != EAGAIN) {
        record1[record1_size++] = canmsg;
    }

    close(fd);
    pthread_exit(NULL);
}

TEST( Driver, FloodSend ) {
    int fd0_tx = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd0_tx, -1);

    int fd1_tx = -1;

    if (get_device1_tx0() != std::string("")) {
        fd1_tx = open(get_device1_tx0().c_str(), O_RDWR);

        EXPECT_NE(fd1_tx, -1);
    }

    // set latency limit to 0 (i.e. no limit)
    int latency_ret = set_latency_limit_ms(fd0_tx, 0);

    EXPECT_EQ(latency_ret, EOK);

    if (fd1_tx != -1) {
        latency_ret = set_latency_limit_ms(fd1_tx, 0);

        EXPECT_EQ(latency_ret, EOK);
    }

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

    receive_loop0_started = receive_loop1_started = false;

    pthread_t thread0;
    pthread_create(&thread0, NULL, &receive_loop0, NULL);

    pthread_t thread1;

    if (fd1_tx != -1) {
        pthread_create(&thread1, NULL, &receive_loop1, NULL);

        while (!receive_loop1_started) {
            usleep(1000);
        }
    }

    while (!receive_loop0_started) {
        usleep(1000);
    }

    struct can_devctl_stats stats0, stats1;
    int get_stats_ret = get_stats(fd0_tx, &stats0);

    EXPECT_EQ(get_stats_ret, EOK);

    uint32_t initial_tx_frames0 = stats0.transmitted_frames;
    uint32_t initial_rx_frames0 = stats0.received_frames;
    uint32_t initial_missing_ack0 = stats0.missing_ack;
    uint32_t initial_total_frame_errors0 = stats0.total_frame_errors;
    uint32_t initial_hw_receive_overflows0 = stats0.hw_receive_overflows;
    uint32_t initial_rx_interrupts0 = stats0.rx_interrupts;
    uint32_t initial_tx_interrupts0 = stats0.tx_interrupts;
    uint32_t initial_total_interrupts0 = stats0.total_interrupts;

    uint32_t initial_tx_frames1 = 0;
    uint32_t initial_rx_frames1 = 0;
    uint32_t initial_missing_ack1 = 0;
    uint32_t initial_total_frame_errors1 = 0;
    uint32_t initial_hw_receive_overflows1 = 0;
    uint32_t initial_rx_interrupts1 = 0;
    uint32_t initial_tx_interrupts1 = 0;
    uint32_t initial_total_interrupts1 = 0;

    if (fd1_tx != -1) {
        get_stats_ret = get_stats(fd1_tx, &stats1);

        EXPECT_EQ(get_stats_ret, EOK);

        initial_tx_frames1 = stats1.transmitted_frames;
        initial_rx_frames1 = stats1.received_frames;
        initial_missing_ack1 = stats1.missing_ack;
        initial_total_frame_errors1 = stats1.total_frame_errors;
        initial_hw_receive_overflows1 = stats1.hw_receive_overflows;
        initial_rx_interrupts1 = stats1.rx_interrupts;
        initial_tx_interrupts1 = stats1.tx_interrupts;
        initial_total_interrupts1 = stats1.total_interrupts;
    }

    // ensure all channels are empty
    int fd = open(get_device0_rx0().c_str(), O_RDWR);

    int err = read_frame_raw_noblock(fd, NULL);

    EXPECT_EQ(err, EAGAIN);

    close(fd);

    if (get_device1_rx0() != std::string("")) {
        fd = open(get_device1_rx0().c_str(), O_RDWR);

        err = read_frame_raw_noblock(fd, NULL);

        EXPECT_EQ(err, EAGAIN);

        close(fd);
    }

    // flood messages
    for (int i = 0; i < FLOOD_TEST_SIZE-1; ++i) {
        write_frame_raw(fd0_tx, &canmsg);
    }

    if (fd1_tx != -1) {
        for (int i = 0; i < FLOOD_TEST_SIZE-1; ++i) {
            write_frame_raw(fd1_tx, &canmsg);
        }
    }

    usleep(10000);
    receive_loop0_stop = true;
    receive_loop1_stop = true;

    write_frame_raw(fd0_tx, &canmsg);

    if (fd1_tx != -1) {
        write_frame_raw(fd1_tx, &canmsg);
    }

    err = pthread_join(thread0, NULL);

    EXPECT_EQ(err, EOK);

    if (fd1_tx != -1) {
        err = pthread_join(thread1, NULL);

        EXPECT_EQ(err, EOK);
    }

    // At least some of the data should have been received
    EXPECT_GE(record0_size, FLOOD_TEST_SIZE/50);
    EXPECT_LE(record0_size, FLOOD_TEST_SIZE);

    if (fd1_tx != -1) {
        EXPECT_GE(record1_size, FLOOD_TEST_SIZE/50);
        EXPECT_LE(record1_size, FLOOD_TEST_SIZE);
    }

    // It seems QEmu CAN interface doesn't emulate baud rate correctly, so the
    // following checks cannot be done.
    //
    // 2.0a (SFF) message is 47bits and 2.0b (EFF) message is 67bits
    //EXPECT_NEAR(1000*record0_size*67/(t2_ms - t1_ms), 250000, 50000);
    //EXPECT_NEAR(1000*record1_size*67/(t1_ms - t0_ms), 250000, 50000);

    uint32_t expected_missed = 800;

    get_stats_ret = get_stats(fd0_tx, &stats0);

    EXPECT_EQ(get_stats_ret, EOK);

    EXPECT_GE(stats0.transmitted_frames - initial_tx_frames0, record0_size);
    EXPECT_GE(stats0.received_frames - initial_rx_frames0, 0);
    EXPECT_LE(stats0.missing_ack - initial_missing_ack0, expected_missed);
    EXPECT_EQ(stats0.total_frame_errors - initial_total_frame_errors0, 0);
    EXPECT_EQ(stats0.hw_receive_overflows - initial_hw_receive_overflows0, 0);
    EXPECT_EQ(stats0.rx_interrupts - initial_rx_interrupts0, 0);
    EXPECT_EQ(stats0.tx_interrupts - initial_tx_interrupts0, 0);
    EXPECT_EQ(stats0.total_interrupts - initial_total_interrupts0, 0);

    EXPECT_EQ(stats0.stuff_errors, 0);
    EXPECT_EQ(stats0.form_errors, 0);
    EXPECT_EQ(stats0.dom_bit_recess_errors, 0);
    EXPECT_EQ(stats0.recess_bit_dom_errors, 0);
    EXPECT_EQ(stats0.parity_errors, 0);
    EXPECT_EQ(stats0.crc_errors, 0);
    EXPECT_EQ(stats0.sw_receive_q_full, 0);
    EXPECT_EQ(stats0.error_warning_state_count, 0);
    EXPECT_EQ(stats0.error_passive_state_count, 0);
    EXPECT_EQ(stats0.bus_off_state_count, 0);
    EXPECT_EQ(stats0.bus_idle_count, 0);
    EXPECT_EQ(stats0.power_down_count, 0);
    EXPECT_EQ(stats0.wake_up_count, 0);

    close(fd0_tx);

    if (fd1_tx != -1) {
        get_stats_ret = get_stats(fd1_tx, &stats1);

        EXPECT_EQ(get_stats_ret, EOK);

        EXPECT_GE(stats1.transmitted_frames - initial_tx_frames1, record1_size);
        EXPECT_GE(stats1.received_frames - initial_rx_frames1, 0);
        EXPECT_LE(stats1.missing_ack - initial_missing_ack1,
                expected_missed);

        EXPECT_EQ(stats1.total_frame_errors - initial_total_frame_errors1, 0);
        EXPECT_EQ(stats1.hw_receive_overflows - initial_hw_receive_overflows1, 0);
        EXPECT_EQ(stats1.rx_interrupts - initial_rx_interrupts1, 0);
        EXPECT_EQ(stats1.tx_interrupts - initial_tx_interrupts1, 0);
        EXPECT_EQ(stats1.total_interrupts - initial_total_interrupts1, 0);

        EXPECT_EQ(stats1.stuff_errors, 0);
        EXPECT_EQ(stats1.form_errors, 0);
        EXPECT_EQ(stats1.dom_bit_recess_errors, 0);
        EXPECT_EQ(stats1.recess_bit_dom_errors, 0);
        EXPECT_EQ(stats1.parity_errors, 0);
        EXPECT_EQ(stats1.crc_errors, 0);
        EXPECT_EQ(stats1.sw_receive_q_full, 0);
        EXPECT_EQ(stats1.error_warning_state_count, 0);
        EXPECT_EQ(stats1.error_passive_state_count, 0);
        EXPECT_EQ(stats1.bus_off_state_count, 0);
        EXPECT_EQ(stats1.bus_idle_count, 0);
        EXPECT_EQ(stats1.power_down_count, 0);
        EXPECT_EQ(stats1.wake_up_count, 0);

        close(fd1_tx);
    }
}

#endif
