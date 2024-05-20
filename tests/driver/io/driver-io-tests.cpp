/**
 * \file    driver-io-tests.cpp
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

static bool io_receive_loop0_started = false,
            io_receive_loop1_started = false,
            io_receive_loop2_started = false;
static volatile size_t io_record0_size = 0;
static volatile size_t io_record1_size = 0;
static volatile size_t io_record2_size = 0;

void* io_receive_loop0 (void* arg) {
    char* msg = (char*)arg;
    uint32_t mid = 0xABC;

    int fd = open(get_device0_rx0().c_str(), O_RDWR);

    set_mfilter(fd, mid);

    io_receive_loop0_started = true;

    if (read(fd, (void*)msg, 12) != -1) {
        io_record0_size++;

        set_mfilter(fd, 0xFFFFFFFF);
        close(fd);
        pthread_exit(msg);
    }

    set_mfilter(fd, 0xFFFFFFFF);
    close(fd);
    pthread_exit(NULL);
}

void* io_receive_loop1 (void* arg) {
    char* msg = (char*)arg;
    uint32_t mid = 0xABC;

    int fd = open(get_device1_rx0().c_str(), O_RDWR);

    set_mfilter(fd, mid);

    io_receive_loop1_started = true;

    if (read(fd, (void*)msg, 12) != -1) {
        io_record1_size++;

        set_mfilter(fd, 0xFFFFFFFF);
        close(fd);
        pthread_exit(msg);
    }

    set_mfilter(fd, 0xFFFFFFFF);
    close(fd);
    pthread_exit(NULL);
}

void* io_receive_loop2 (void* arg) {
    char* msg = (char*)arg;
    uint32_t mid = 0xABC;

    int fd = open(get_device0_rx0().c_str(), O_RDWR);

    set_mfilter(fd, mid);

    io_receive_loop2_started = true;

    if (read(fd, (void*)msg, 12) != -1) {
        io_record2_size++;

        set_mfilter(fd, 0xFFFFFFFF);
        close(fd);
        pthread_exit(msg);
    }

    set_mfilter(fd, 0xFFFFFFFF);
    close(fd);
    pthread_exit(NULL);
}

TEST( IO, SingleSendReceive ) {
    int fd0 = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd0, -1);

    int fd1 = -1;

    if (get_device1_tx0() != std::string("")) {
        fd1 = open(get_device1_tx0().c_str(), O_RDWR);

        EXPECT_NE(fd1, -1);
    }

    char msg[] = "test message";
    char wrong_msg[] = "wrong message";

    uint32_t mid = 0xABC;
    uint32_t wrong_mid = 0xAB1;

    char msg0[16], msg1[16];

    io_receive_loop0_started = io_receive_loop1_started = false;

    pthread_t thread0;
    pthread_create(&thread0, NULL, &io_receive_loop0, msg0);

    pthread_t thread1;

    if (fd1 != -1) {
        pthread_create(&thread1, NULL, &io_receive_loop1, msg1);
    }

    while (!io_receive_loop0_started) {
        usleep(1000);
    }

    if (fd1 != -1) {
        while (!io_receive_loop1_started) {
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

    int set_mid_ret = set_mid(fd0, wrong_mid);

    EXPECT_EQ(set_mid_ret, EOK);

    int n = write(fd0, wrong_msg, 12);

    set_mid_ret = set_mid(fd0, mid);

    EXPECT_EQ(set_mid_ret, EOK);

    n = write(fd0, msg, 12);

    EXPECT_EQ(n, 12);

    void* exit_ptr0;
    pthread_join(thread0, &exit_ptr0);

    EXPECT_EQ(exit_ptr0, msg0);

    msg0[n] = '\0';

    EXPECT_EQ(std::string(msg0), std::string("test message"));

    usleep(3000);

    if (fd1 != -1) {
        set_mid_ret = set_mid(fd1, wrong_mid);

        EXPECT_EQ(set_mid_ret, EOK);

        n = write(fd1, wrong_msg, 12);

        set_mid_ret = set_mid(fd1, mid);

        EXPECT_EQ(set_mid_ret, EOK);

        n = write(fd1, msg, 12);

        EXPECT_EQ(n, 12);

        void* exit_ptr1;
        pthread_join(thread1, &exit_ptr1);

        EXPECT_EQ(exit_ptr1, msg1);

        msg1[n] = '\0';

        EXPECT_EQ(std::string(msg1), std::string("test message"));
    }

    get_stats_ret = get_stats(fd0, &stats0);

    EXPECT_EQ(get_stats_ret, EOK);

    if (fd1 != -1) {
        get_stats_ret = get_stats(fd1, &stats1);

        EXPECT_EQ(get_stats_ret, EOK);
    }

    EXPECT_EQ(stats0.transmitted_frames - initial_tx_frames0, 4);
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
        EXPECT_EQ(stats1.transmitted_frames - initial_tx_frames1, 4);
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

TEST( IO, SingleSendMultiReceive ) {
    int fd = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd, -1);

    char msg[] = "test message";
    char wrong_msg[] = "wrong message";

    uint32_t mid = 0xABC;
    uint32_t wrong_mid = 0xAB1;

    char msg0[16], msg1[16];

    io_receive_loop0_started = false;

    pthread_t thread0;
    pthread_create(&thread0, NULL, &io_receive_loop0, msg0);

    pthread_t thread1;
    pthread_create(&thread1, NULL, &io_receive_loop2, msg1);

    while (!io_receive_loop0_started || !io_receive_loop2_started) {
        usleep(1000);
    }

    struct can_devctl_stats stats;

    int get_stats_ret = get_stats(fd, &stats);

    EXPECT_EQ(get_stats_ret, EOK);

    uint32_t initial_tx_frames = stats.transmitted_frames;
    uint32_t initial_rx_frames = stats.received_frames;
    uint32_t initial_missing_ack = stats.missing_ack;
    uint32_t initial_total_frame_errors = stats.total_frame_errors;
    uint32_t initial_hw_receive_overflows = stats.hw_receive_overflows;
    uint32_t initial_rx_interrupts = stats.rx_interrupts;
    uint32_t initial_tx_interrupts = stats.tx_interrupts;
    uint32_t initial_total_interrupts = stats.total_interrupts;

    int set_mid_ret = set_mid(fd, wrong_mid);

    EXPECT_EQ(set_mid_ret, EOK);

    int n = write(fd, wrong_msg, 12);

    set_mid_ret = set_mid(fd, mid);

    EXPECT_EQ(set_mid_ret, EOK);

    n = write(fd, msg, 12);

    EXPECT_EQ(n, 12);

    void *exit_ptr0, *exit_ptr1;
    pthread_join(thread0, &exit_ptr0);
    pthread_join(thread1, &exit_ptr1);

    EXPECT_EQ(exit_ptr0, msg0);

    msg0[n] = '\0';

    EXPECT_EQ(std::string(msg0), std::string("test message"));

    EXPECT_EQ(exit_ptr1, msg1);

    msg1[n] = '\0';

    EXPECT_EQ(std::string(msg1), std::string("test message"));

    get_stats_ret = get_stats(fd, &stats);

    EXPECT_EQ(get_stats_ret, EOK);

    EXPECT_EQ(stats.transmitted_frames - initial_tx_frames, 4);
    EXPECT_EQ(stats.received_frames - initial_rx_frames, 0);
    EXPECT_EQ(stats.missing_ack - initial_missing_ack, 0);
    EXPECT_EQ(stats.total_frame_errors - initial_total_frame_errors, 0);
    EXPECT_EQ(stats.hw_receive_overflows - initial_hw_receive_overflows, 0);
    EXPECT_EQ(stats.rx_interrupts - initial_rx_interrupts, 0);
    EXPECT_EQ(stats.tx_interrupts - initial_tx_interrupts, 0);
    EXPECT_EQ(stats.total_interrupts - initial_total_interrupts, 0);

    EXPECT_EQ(stats.stuff_errors, 0);
    EXPECT_EQ(stats.form_errors, 0);
    EXPECT_EQ(stats.dom_bit_recess_errors, 0);
    EXPECT_EQ(stats.recess_bit_dom_errors, 0);
    EXPECT_EQ(stats.parity_errors, 0);
    EXPECT_EQ(stats.crc_errors, 0);
    EXPECT_EQ(stats.sw_receive_q_full, 0);
    EXPECT_EQ(stats.error_warning_state_count, 0);
    EXPECT_EQ(stats.error_passive_state_count, 0);
    EXPECT_EQ(stats.bus_off_state_count, 0);
    EXPECT_EQ(stats.bus_idle_count, 0);
    EXPECT_EQ(stats.power_down_count, 0);
    EXPECT_EQ(stats.wake_up_count, 0);

    close(fd);
}
