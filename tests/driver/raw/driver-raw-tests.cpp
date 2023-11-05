/**
 * \file    driver-raw-tests.cpp
 * \brief   Driver RAW message integration tests definition file; basic single
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
#include <tests/driver/common/test_devices.h>

extern "C" {
    #include <timer.h>
    #include <dev-can-linux/commands.h>
}

static bool receive_loop0_started = false, receive_loop1_started = false;
static volatile size_t record0_size = 0;
static volatile size_t record1_size = 0;

void* receive_loop0 (void* arg) {
    struct can_msg* canmsg = (struct can_msg*)arg;

    int fd = open(get_device0_rx0().c_str(), O_RDWR);

    receive_loop0_started = true;

    if (read_frame_raw_block(fd, canmsg) == EOK) {
        record0_size++;

        close(fd);
        pthread_exit(canmsg);
    }

    pthread_exit(NULL);
}

void* receive_loop1 (void* arg) {
    struct can_msg* canmsg = (struct can_msg*)arg;

    int fd = open(get_device1_rx0().c_str(), O_RDWR);

    receive_loop1_started = true;

    if (read_frame_raw_block(fd, canmsg) == EOK) {
        record1_size++;

        close(fd);
        pthread_exit(canmsg);
    }

    pthread_exit(NULL);
}

TEST( Raw, SingleSendReceive ) {
    int fd0 = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd0, -1);

    int fd1 = -1;

    if (get_device1_tx0() != std::string("")) {
        int fd1 = open(get_device1_tx0().c_str(), O_RDWR);

        EXPECT_NE(fd1, -1);
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

    struct can_msg canmsg0, canmsg1;

    receive_loop0_started = receive_loop1_started = false;

    pthread_t thread0;
    pthread_create(&thread0, NULL, &receive_loop0, &canmsg0);

    while (!receive_loop0_started) {
        usleep(1000);
    }

    pthread_t thread1;

    if (fd1 != -1) {
        pthread_create(&thread1, NULL, &receive_loop1, &canmsg1);

        while (!receive_loop1_started) {
            usleep(1000);
        }
    }

    struct can_devctl_stats stats0, stats1;

    int get_stats_ret = get_stats(fd0, &stats0);

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

    if (fd1 != -1) {
        get_stats_ret = get_stats(fd1, &stats1);

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

    int write_ret = write_frame_raw(fd0, &canmsg);

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
    EXPECT_GE(canmsg0.ext.timestamp - start_ms, 1);
    EXPECT_EQ(canmsg0.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg0.ext.is_remote_frame, 0);

    if (fd1 != -1) {
        usleep(3000);
        write_ret = write_frame_raw(fd1, &canmsg);

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
        EXPECT_GE(canmsg1.ext.timestamp - start_ms, 4);
        EXPECT_EQ(canmsg1.ext.is_extended_mid, 1);
        EXPECT_EQ(canmsg1.ext.is_remote_frame, 0);
    }

    get_stats_ret = get_stats(fd0, &stats0);

    EXPECT_EQ(get_stats_ret, EOK);

    if (fd1 != -1) {
        get_stats_ret = get_stats(fd1, &stats1);

        EXPECT_EQ(get_stats_ret, EOK);
    }

    EXPECT_EQ(stats0.transmitted_frames - initial_tx_frames0, 1);
    EXPECT_EQ(stats0.received_frames - initial_rx_frames0, 0);
    EXPECT_EQ(stats0.missing_ack - initial_missing_ack0, 0);
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

    close(fd0);

    if (fd1 != -1) {
        EXPECT_EQ(stats1.transmitted_frames - initial_tx_frames1, 1);
        EXPECT_EQ(stats1.received_frames - initial_rx_frames1, 0);
        EXPECT_EQ(stats1.missing_ack - initial_missing_ack1, 0);
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

        close(fd1);
    }
}

TEST( Raw, SingleSendMultiReceive ) {
    int fd = open(get_device0_tx0().c_str(), O_RDWR);

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

    receive_loop0_started = false;

    struct can_msg canmsg0, canmsg1;

    pthread_t thread0;
    pthread_create(&thread0, NULL, &receive_loop0, &canmsg0);

    pthread_t thread1;
    pthread_create(&thread1, NULL, &receive_loop0, &canmsg1);

    while (!receive_loop0_started) {
        usleep(1000);
    }

    int write_ret = write_frame_raw(fd, &canmsg);

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
    EXPECT_GE(canmsg0.ext.timestamp - start_ms, 1);
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
    EXPECT_GE(canmsg1.ext.timestamp - start_ms, 1);
    EXPECT_EQ(canmsg1.ext.is_extended_mid, 1);
    EXPECT_EQ(canmsg1.ext.is_remote_frame, 0);

    close(fd);
}
