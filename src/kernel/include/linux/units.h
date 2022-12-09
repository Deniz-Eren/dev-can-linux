/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/units.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          not been modified.
 *
 * Please also check the "SPDX-License-Identifier" documentation from the Linux
 * Kernel source code repository: github.com/torvalds/linux.git for further
 * details.
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

#ifndef _LINUX_UNITS_H
#define _LINUX_UNITS_H

#include <linux/math.h>

/* Metric prefixes in accordance with Système international (d'unités) */
#define PETA	1000000000000000ULL
#define TERA	1000000000000ULL
#define GIGA	1000000000UL
#define MEGA	1000000UL
#define KILO	1000UL
#define HECTO	100UL
#define DECA	10UL
#define DECI	10UL
#define CENTI	100UL
#define MILLI	1000UL
#define MICRO	1000000UL
#define NANO	1000000000UL
#define PICO	1000000000000ULL
#define FEMTO	1000000000000000ULL

#define NANOHZ_PER_HZ		1000000000UL
#define MICROHZ_PER_HZ		1000000UL
#define MILLIHZ_PER_HZ		1000UL
#define HZ_PER_KHZ		1000UL
#define KHZ_PER_MHZ		1000UL
#define HZ_PER_MHZ		1000000UL

#define MILLIWATT_PER_WATT	1000UL
#define MICROWATT_PER_MILLIWATT	1000UL
#define MICROWATT_PER_WATT	1000000UL

#define ABSOLUTE_ZERO_MILLICELSIUS -273150

static inline long milli_kelvin_to_millicelsius(long t)
{
	return t + ABSOLUTE_ZERO_MILLICELSIUS;
}

static inline long millicelsius_to_milli_kelvin(long t)
{
	return t - ABSOLUTE_ZERO_MILLICELSIUS;
}

#define MILLIDEGREE_PER_DEGREE 1000
#define MILLIDEGREE_PER_DECIDEGREE 100

static inline long kelvin_to_millicelsius(long t)
{
	return milli_kelvin_to_millicelsius(t * MILLIDEGREE_PER_DEGREE);
}

static inline long millicelsius_to_kelvin(long t)
{
	t = millicelsius_to_milli_kelvin(t);

	return DIV_ROUND_CLOSEST(t, MILLIDEGREE_PER_DEGREE);
}

static inline long deci_kelvin_to_celsius(long t)
{
	t = milli_kelvin_to_millicelsius(t * MILLIDEGREE_PER_DECIDEGREE);

	return DIV_ROUND_CLOSEST(t, MILLIDEGREE_PER_DEGREE);
}

static inline long celsius_to_deci_kelvin(long t)
{
	t = millicelsius_to_milli_kelvin(t * MILLIDEGREE_PER_DEGREE);

	return DIV_ROUND_CLOSEST(t, MILLIDEGREE_PER_DECIDEGREE);
}

/**
 * deci_kelvin_to_millicelsius_with_offset - convert Kelvin to Celsius
 * @t: temperature value in decidegrees Kelvin
 * @offset: difference between Kelvin and Celsius in millidegrees
 *
 * Return: temperature value in millidegrees Celsius
 */
static inline long deci_kelvin_to_millicelsius_with_offset(long t, long offset)
{
	return t * MILLIDEGREE_PER_DECIDEGREE - offset;
}

static inline long deci_kelvin_to_millicelsius(long t)
{
	return milli_kelvin_to_millicelsius(t * MILLIDEGREE_PER_DECIDEGREE);
}

static inline long millicelsius_to_deci_kelvin(long t)
{
	t = millicelsius_to_milli_kelvin(t);

	return DIV_ROUND_CLOSEST(t, MILLIDEGREE_PER_DECIDEGREE);
}

static inline long kelvin_to_celsius(long t)
{
	return t + DIV_ROUND_CLOSEST(ABSOLUTE_ZERO_MILLICELSIUS,
				     MILLIDEGREE_PER_DEGREE);
}

static inline long celsius_to_kelvin(long t)
{
	return t - DIV_ROUND_CLOSEST(ABSOLUTE_ZERO_MILLICELSIUS,
				     MILLIDEGREE_PER_DEGREE);
}

#endif /* _LINUX_UNITS_H */
