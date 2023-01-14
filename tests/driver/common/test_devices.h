/*
 * \file    tests/driver/common.h
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

#ifndef TESTS_DRIVER_COMMON_H_
#define TESTS_DRIVER_COMMON_H_

#include <stdlib.h>


static inline std::string get_device0_rx0 (void) {
    char* dev = getenv("DRIVER_TEST_DEVICE0_RX0");

    if (dev != NULL) {
        return std::string(dev);
    }

    return std::string("");
}

static inline std::string get_device1_rx0 (void) {
    char* dev = getenv("DRIVER_TEST_DEVICE1_RX0");

    if (dev != NULL) {
        return std::string(dev);
    }

    return std::string("");
}

static inline std::string get_device0_tx0 (void) {
    char* dev = getenv("DRIVER_TEST_DEVICE0_TX0");

    if (dev != NULL) {
        return std::string(dev);
    }

    return std::string("");
}

static inline std::string get_device1_tx0 (void) {
    char* dev = getenv("DRIVER_TEST_DEVICE1_TX0");

    if (dev != NULL) {
        return std::string(dev);
    }

    return std::string("");
}

#endif /* TESTS_DRIVER_COMMON_H_ */
