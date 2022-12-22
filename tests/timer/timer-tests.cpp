/**
 * \file    timer-tests.cpp
 * \brief   Timer implementation test definition file
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
    #include <timer.h>

    struct test_data_t {
        int16_t* numbers;
        size_t size;
        uint64_t sum;
        uint64_t trigger_time_us;
    };

    double TIMER_INTERVAL_MS = (((double)(TIMER_INTERVAL_NS))/MILLION);

    void test_callback_timer (void* data) {
        uint64_t now = get_clock_time_us();

        struct test_data_t *test_data = (struct test_data_t*)data;

        test_data->trigger_time_us = now;

        for (int i = 0; i < test_data->size; ++i) {
            test_data->sum += test_data->numbers[i];
        }
    }
}

TEST( Timer, ScheduleWork ) {
    timer_t test_work;
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    struct test_data_t test_data = {
        .numbers = numbers,
        .size = sizeof(numbers)/sizeof(numbers[0]),
        .sum = 0,
        .trigger_time_us = get_clock_time_us()
    };

    setup_timer(&test_work, test_callback_timer, &test_data);

    schedule_delayed_work(&test_work, 5.0 / TIMER_INTERVAL_MS);

    uint64_t start_time_us = get_clock_time_us();
    usleep(20000);
    uint64_t end_time_us = get_clock_time_us();

    double test_diff_ms = (end_time_us - start_time_us) / 1000.0;
    double trig_diff_ms = (test_data.trigger_time_us - start_time_us) / 1000.0;

    double n_periods_waited = test_diff_ms / 5.0;

    cancel_delayed_work_sync(&test_work);

    // callback must not trigger earlier than expected
    EXPECT_GT(trig_diff_ms, 5.0);

    // give 1.5 millisecond buffer from expected time
    EXPECT_LT(trig_diff_ms, 6.5);

    // callback must be called only once
    EXPECT_EQ(test_data.sum, 376);

    // wait at least 3 time longer than expected to be sure callback only gets
    // called once
    EXPECT_GT(n_periods_waited, 3);
}
