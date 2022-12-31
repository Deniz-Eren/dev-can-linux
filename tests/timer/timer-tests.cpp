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
        double trigger_time_ms;
    };

    double TIMER_INTERVAL_MS = (((double)(TIMER_INTERVAL_NS))/MILLION);

    void test_callback_timer (void* data) {
        double now = get_clock_time_us() / 1000.0;

        struct test_data_t *test_data = (struct test_data_t*)data;

        test_data->trigger_time_ms = now;

        for (int i = 0; i < test_data->size; ++i) {
            test_data->sum += test_data->numbers[i];
        }
    }

    timer_record_t test_work1, test_work2, test_work3;
    test_data_t test_data1, test_data2, test_data3;
}

TEST( Timer, ScheduleWork ) {
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    test_data1.numbers = numbers;
    test_data1.size = sizeof(numbers)/sizeof(numbers[0]);
    test_data1.sum = 0;
    test_data1.trigger_time_ms = get_clock_time_us() / 1000.0;

    setup_timer(&test_work1, test_callback_timer, &test_data1);

    schedule_delayed_work(&test_work1, 5.0 / TIMER_INTERVAL_MS);

    double start_time_ms = get_clock_time_us() / 1000.0;
    usleep(20000);
    double end_time_ms = get_clock_time_us() / 1000.0;

    double test_diff_ms = end_time_ms - start_time_ms;
    double trig_diff_ms = test_data1.trigger_time_ms - start_time_ms;

    double n_periods_waited = test_diff_ms / 5.0;

    cancel_delayed_work_sync(&test_work1);

    // callback must not trigger earlier than expected
    EXPECT_GT(trig_diff_ms, 5.0);

    // give 1.5 millisecond buffer from expected time
    EXPECT_LT(trig_diff_ms, 6.5);

    // callback must be called only once
    EXPECT_EQ(test_data1.sum, 376);

    // wait at least 3 time longer than expected to be sure callback only gets
    // called once
    EXPECT_GT(n_periods_waited, 3);
}

TEST( Timer, ScheduleMoreWork ) {
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    test_data2.numbers = numbers;
    test_data2.size = sizeof(numbers)/sizeof(numbers[0]);
    test_data2.sum = 0;
    test_data2.trigger_time_ms = get_clock_time_us() / 1000.0;

    setup_timer(&test_work2, test_callback_timer, &test_data2);

    schedule_delayed_work(&test_work2, 2.0 / TIMER_INTERVAL_MS);

    double start_time_ms = get_clock_time_us() / 1000.0;
    usleep(20000);
    double end_time_ms = get_clock_time_us() / 1000.0;

    double test_diff_ms = end_time_ms - start_time_ms;
    double trig_diff_ms = test_data2.trigger_time_ms - start_time_ms;

    double n_periods_waited = test_diff_ms / 2.0;
    cancel_delayed_work_sync(&test_work2);

    // callback must not trigger earlier than expected
    EXPECT_GT(trig_diff_ms, 2.0);

    // give 1.5 millisecond buffer from expected time
    EXPECT_LT(trig_diff_ms, 3.5);

    // callback must be called only once
    EXPECT_EQ(test_data2.sum, 376);

    // wait at least 3 time longer than expected to be sure callback only gets
    // called once
    EXPECT_GT(n_periods_waited, 3);
}

TEST( Timer, ScheduleSameWorkAgain ) {
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    test_data1.numbers = numbers;
    test_data1.size = sizeof(numbers)/sizeof(numbers[0]);
    test_data1.sum = 0;
    test_data1.trigger_time_ms = get_clock_time_us() / 1000.0;

    schedule_delayed_work(&test_work1, 12.0 / TIMER_INTERVAL_MS);

    double start_time_ms = get_clock_time_us() / 1000.0;
    usleep(40000);
    double end_time_ms = get_clock_time_us() / 1000.0;

    double test_diff_ms = end_time_ms - start_time_ms;
    double trig_diff_ms = test_data1.trigger_time_ms - start_time_ms;

    double n_periods_waited = test_diff_ms / 12.0;
    cancel_delayed_work_sync(&test_work1);

    // callback must not trigger earlier than expected
    EXPECT_GT(trig_diff_ms, 12.0);

    // give 1.5 millisecond buffer from expected time
    EXPECT_LT(trig_diff_ms, 13.5);

    // callback must be called only once
    EXPECT_EQ(test_data1.sum, 376);

    // wait at least 3 time longer than expected to be sure callback only gets
    // called once
    EXPECT_GT(n_periods_waited, 3);
}

TEST( Timer, CancelWork ) {
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    test_data3.numbers = numbers;
    test_data3.size = sizeof(numbers)/sizeof(numbers[0]);
    test_data3.sum = 0;
    test_data3.trigger_time_ms = get_clock_time_us() / 1000.0;

    setup_timer(&test_work3, test_callback_timer, &test_data3);
    schedule_delayed_work(&test_work3, 9.0 / TIMER_INTERVAL_MS);
    double start_time_ms = get_clock_time_us() / 1000.0;

    usleep(3000);
    cancel_delayed_work_sync(&test_work3);

    usleep(30000);
    double end_time_ms = get_clock_time_us() / 1000.0;

    double test_diff_ms = end_time_ms - start_time_ms;
    double trig_diff_ms = test_data3.trigger_time_ms - start_time_ms;

    double n_periods_waited = test_diff_ms / 9.0;

    // we expect no callback, so trigger time difference should be negative
    EXPECT_LT(trig_diff_ms, 0.0);

    // callback must not be called at all
    EXPECT_EQ(test_data3.sum, 0);

    // wait at least 3 time longer than expected to be sure callback got
    // sufficient time to reply if it was going to
    EXPECT_GT(n_periods_waited, 3);
}

TEST( Timer, CancelRescheduledWork ) {
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    test_data1.numbers = numbers;
    test_data1.size = sizeof(numbers)/sizeof(numbers[0]);
    test_data1.sum = 0;
    test_data1.trigger_time_ms = get_clock_time_us() / 1000.0;

    schedule_delayed_work(&test_work1, 7.0 / TIMER_INTERVAL_MS);
    double start_time_ms = get_clock_time_us() / 1000.0;

    usleep(2000);
    cancel_delayed_work_sync(&test_work1);

    usleep(25000);
    double end_time_ms = get_clock_time_us() / 1000.0;

    double test_diff_ms = end_time_ms - start_time_ms;
    double trig_diff_ms = test_data1.trigger_time_ms - start_time_ms;

    double n_periods_waited = test_diff_ms / 7.0;

    // we expect no callback, so trigger time difference should be negative
    EXPECT_LT(trig_diff_ms, 0.0);

    // callback must not be called at all
    EXPECT_EQ(test_data1.sum, 0);

    // wait at least 3 time longer than expected to be sure callback got
    // sufficient time to reply if it was going to
    EXPECT_GT(n_periods_waited, 3);
}

TEST( Timer, SetupAgainAndScheduleSameWork ) {
    int16_t numbers[] = { 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

    test_data1.numbers = numbers;
    test_data1.size = sizeof(numbers)/sizeof(numbers[0]);
    test_data1.sum = 0;
    test_data1.trigger_time_ms = get_clock_time_us() / 1000.0;

    setup_timer(&test_work1, test_callback_timer, &test_data1);
    schedule_delayed_work(&test_work1, 12.0 / TIMER_INTERVAL_MS);

    double start_time_ms = get_clock_time_us() / 1000.0;
    usleep(40000);
    double end_time_ms = get_clock_time_us() / 1000.0;

    double test_diff_ms = end_time_ms - start_time_ms;
    double trig_diff_ms = test_data1.trigger_time_ms - start_time_ms;

    double n_periods_waited = test_diff_ms / 12.0;
    cancel_delayed_work_sync(&test_work1);

    // callback must not trigger earlier than expected
    EXPECT_GT(trig_diff_ms, 12.0);

    // give 1.5 millisecond buffer from expected time
    EXPECT_LT(trig_diff_ms, 13.5);

    // callback must be called only once
    EXPECT_EQ(test_data1.sum, 376);

    // wait at least 3 time longer than expected to be sure callback only gets
    // called once
    EXPECT_GT(n_periods_waited, 3);
}
