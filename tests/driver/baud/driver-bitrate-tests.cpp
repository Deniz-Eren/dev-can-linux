/**
 * \file    driver-bitrate-tests.cpp
 * \brief   Driver baud or bitrate setting tests definition file.
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


TEST( Driver, DefaultBitTiming ) {
    int fd_tx = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd_tx, -1);

    int fd_rx = open(get_device0_rx0().c_str(), O_RDWR);

    EXPECT_NE(fd_rx, -1);

    struct can_devctl_info info_tx, info_rx;

    int get_info_ret = get_info(fd_tx, &info_tx);

    EXPECT_EQ(get_info_ret, EOK);

    /* Default bit rate */
    EXPECT_EQ(info_tx.bit_rate, 250000);

    /* Default bit rate prescaler */
    EXPECT_EQ(info_tx.bit_rate_prescaler, 2);

    /* Default time quantum Sync Jump Width */
    EXPECT_EQ(info_tx.sync_jump_width, 1);

    /* Default time quantum Time Segment 1 */
    EXPECT_EQ(info_tx.time_segment_1, 7);

    /* Default time quantum Time Segment 2 */
    EXPECT_EQ(info_tx.time_segment_2, 2);

    close(fd_tx);

    get_info_ret = get_info(fd_rx, &info_rx);

    EXPECT_EQ(get_info_ret, EOK);

    /* Default bit rate */
    EXPECT_EQ(info_rx.bit_rate, 250000);

    /* Default bit rate prescaler */
    EXPECT_EQ(info_rx.bit_rate_prescaler, 2);

    /* Default time quantum Sync Jump Width */
    EXPECT_EQ(info_rx.sync_jump_width, 1);

    /* Default time quantum Time Segment 1 */
    EXPECT_EQ(info_rx.time_segment_1, 7);

    /* Default time quantum Time Segment 2 */
    EXPECT_EQ(info_rx.time_segment_2, 2);

    close(fd_rx);
}

TEST( Driver, SetBitRate ) {
    int fd_tx = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd_tx, -1);

    int fd_rx = open(get_device0_rx0().c_str(), O_RDWR);

    EXPECT_NE(fd_rx, -1);

    int set_bitrate_ret = set_bitrate(fd_tx, 1000000);

    EXPECT_EQ(set_bitrate_ret, EOK);

    struct can_devctl_info info_tx, info_rx;

    int get_info_ret = get_info(fd_tx, &info_tx);

    EXPECT_EQ(get_info_ret, EOK);

    /* Default bit rate */
    EXPECT_EQ(info_tx.bit_rate, 1000000);

    /* Default bit rate prescaler */
    EXPECT_EQ(info_tx.bit_rate_prescaler, 1);

    /* Default time quantum Sync Jump Width */
    EXPECT_EQ(info_tx.sync_jump_width, 1);

    /* Default time quantum Time Segment 1 */
    EXPECT_EQ(info_tx.time_segment_1, 3);

    /* Default time quantum Time Segment 2 */
    EXPECT_EQ(info_tx.time_segment_2, 2);

    get_info_ret = get_info(fd_rx, &info_rx);

    EXPECT_EQ(get_info_ret, EOK);

    /* Default bit rate */
    EXPECT_EQ(info_rx.bit_rate, 1000000);

    /* Default bit rate prescaler */
    EXPECT_EQ(info_rx.bit_rate_prescaler, 1);

    /* Default time quantum Sync Jump Width */
    EXPECT_EQ(info_rx.sync_jump_width, 1);

    /* Default time quantum Time Segment 1 */
    EXPECT_EQ(info_rx.time_segment_1, 3);

    /* Default time quantum Time Segment 2 */
    EXPECT_EQ(info_rx.time_segment_2, 2);

    /* Set bitrate back to default value */
    set_bitrate_ret = set_bitrate(fd_tx, 250000);

    EXPECT_EQ(set_bitrate_ret, EOK);

    close(fd_tx);
    close(fd_rx);
}

TEST( Driver, SetBitTiming ) {
    int fd_tx = open(get_device0_tx0().c_str(), O_RDWR);

    EXPECT_NE(fd_tx, -1);

    int fd_rx = open(get_device0_rx0().c_str(), O_RDWR);

    EXPECT_NE(fd_rx, -1);

    struct can_devctl_timing timing = {
        .ref_clock_freq = 500000,
        .bit_rate_prescaler = 1,
        .sync_jump_width = 1,
        .time_segment_1 = 7,
        .time_segment_2 = 2
    };

    int set_bittiming_ret = set_bittiming(fd_rx, &timing);

    EXPECT_EQ(set_bittiming_ret, EOK);

    struct can_devctl_info info_tx, info_rx;

    int get_info_ret = get_info(fd_tx, &info_tx);

    EXPECT_EQ(get_info_ret, EOK);

    /* Default bit rate */
    EXPECT_EQ(info_tx.bit_rate, 500000);

    /* Default bit rate prescaler */
    EXPECT_EQ(info_tx.bit_rate_prescaler, 1);

    /* Default time quantum Sync Jump Width */
    EXPECT_EQ(info_tx.sync_jump_width, 1);

    /* Default time quantum Time Segment 1 */
    EXPECT_EQ(info_tx.time_segment_1, 7);

    /* Default time quantum Time Segment 2 */
    EXPECT_EQ(info_tx.time_segment_2, 2);

    get_info_ret = get_info(fd_rx, &info_rx);

    EXPECT_EQ(get_info_ret, EOK);

    /* Default bit rate */
    EXPECT_EQ(info_rx.bit_rate, 500000);

    /* Default bit rate prescaler */
    EXPECT_EQ(info_rx.bit_rate_prescaler, 1);

    /* Default time quantum Sync Jump Width */
    EXPECT_EQ(info_rx.sync_jump_width, 1);

    /* Default time quantum Time Segment 1 */
    EXPECT_EQ(info_rx.time_segment_1, 7);

    /* Default time quantum Time Segment 2 */
    EXPECT_EQ(info_rx.time_segment_2, 2);

    /* Set bitrate back to default value */
    int set_bitrate_ret = set_bitrate(fd_rx, 250000);

    EXPECT_EQ(set_bitrate_ret, EOK);

    /* Set bitrate back to default value */
    set_bitrate_ret = set_bitrate(fd_tx, 250000);

    EXPECT_EQ(set_bitrate_ret, EOK);

    /* Set bitrate back to default value */
    set_bitrate_ret = set_bitrate(fd_rx, 250000);

    EXPECT_EQ(set_bitrate_ret, EOK);

    close(fd_tx);
    close(fd_rx);
}
