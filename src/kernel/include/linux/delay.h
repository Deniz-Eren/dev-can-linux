/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/delay.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified by redefining delay functions to interface QNX delay
 *          functions instead.
 *
 * \details TODO: further investigation and testing needed regarding the
 *          accuracy of the current implementation. Currently the Linux
 *          implementations have been primarily replaced with QNX default usleep
 *          function; this may not be good enough.
 *
 * Delay routines, using a pre-computed "loops_per_jiffy" value.
 *
 * Please note that ndelay(), udelay() and mdelay() may return early for
 * several reasons:
 *  1. computed loops_per_jiffy too low (due to the time taken to
 *     execute the timer interrupt.)
 *  2. cache behaviour affecting the time it takes to execute the
 *     loop function.
 *  3. CPU clock rate changes.
 *
 * Please see this thread:
 *   https://lists.openwall.net/linux-kernel/2011/01/09/56
 *
 * Copyright (C) 1993 Linus Torvalds
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
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

#ifndef _LINUX_DELAY_H
#define _LINUX_DELAY_H

#ifdef __QNX__
#include <unistd.h>
#include <linux/kernel.h>

#define udelay(useconds) usleep(useconds)
#else
#include <linux/math.h>
#include <linux/sched.h>

extern unsigned long loops_per_jiffy;

#include <asm/delay.h>
#endif

/*
 * Using udelay() for intervals greater than a few milliseconds can
 * risk overflow for high loops_per_jiffy (high bogomips) machines. The
 * mdelay() provides a wrapper to prevent this.  For delays greater
 * than MAX_UDELAY_MS milliseconds, the wrapper is used.  Architecture
 * specific values can be defined in asm-???/delay.h as an override.
 * The 2nd mdelay() definition ensures GCC will optimize away the 
 * while loop for the common cases where n <= MAX_UDELAY_MS  --  Paul G.
 */
#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_MS	5
#endif

#ifndef mdelay
/**
 * mdelay - Inserting a delay based on milliseconds with busy waiting
 * @n:	requested delay in milliseconds
 *
 * See udelay() for basic information about mdelay() and it's variants.
 *
 * Please double check, whether mdelay() is the right way to go or whether a
 * refactoring of the code is the better variant to be able to use msleep()
 * instead.
 */
#define mdelay(n) (\
	(__builtin_constant_p(n) && (n)<=MAX_UDELAY_MS) ? udelay((n)*1000) : \
	({unsigned long __ms=(n); while (__ms--) udelay(1000);}))
#endif

#ifndef ndelay
static inline void ndelay(unsigned long x)
{
#ifdef __QNX__
    udelay(x / 1000); /* TODO: check this; currently doing simple division */
#else
	udelay(DIV_ROUND_UP(x, 1000));
#endif
}
#define ndelay(x) ndelay(x)
#endif

extern unsigned long lpj_fine;
void calibrate_delay(void);
unsigned long calibrate_delay_is_known(void);
void __attribute__((weak)) calibration_delay_done(void);
void msleep(unsigned int msecs);
unsigned long msleep_interruptible(unsigned int msecs);
void usleep_range_state(unsigned long min, unsigned long max,
			unsigned int state);

/**
 * usleep_range - Sleep for an approximate time
 * @min:	Minimum time in microseconds to sleep
 * @max:	Maximum time in microseconds to sleep
 *
 * For basic information please refere to usleep_range_state().
 *
 * The task will be in the state TASK_UNINTERRUPTIBLE during the sleep.
 */
static inline void usleep_range(unsigned long min, unsigned long max) {
#ifdef __QNX__
    udelay(min); /* TODO: check this; currently just using udelay min */
#else
	usleep_range_state(min, max, TASK_UNINTERRUPTIBLE);
#endif
}

#ifndef __QNX__
/**
 * usleep_range_idle - Sleep for an approximate time with idle time accounting
 * @min:	Minimum time in microseconds to sleep
 * @max:	Maximum time in microseconds to sleep
 *
 * For basic information please refere to usleep_range_state().
 *
 * The sleeping task has the state TASK_IDLE during the sleep to prevent
 * contribution to the load avarage.
 */
static inline void usleep_range_idle(unsigned long min, unsigned long max)
{
	usleep_range_state(min, max, TASK_IDLE);
}

/**
 * ssleep - wrapper for seconds around msleep
 * @seconds:	Requested sleep duration in seconds
 *
 * Please refere to msleep() for detailed information.
 */
static inline void ssleep(unsigned int seconds)
{
	msleep(seconds * 1000);
}

static const unsigned int max_slack_shift = 2;
#define USLEEP_RANGE_UPPER_BOUND	((TICK_NSEC << max_slack_shift) / NSEC_PER_USEC)

/**
 * fsleep - flexible sleep which autoselects the best mechanism
 * @usecs:	requested sleep duration in microseconds
 *
 * flseep() selects the best mechanism that will provide maximum 25% slack
 * to the requested sleep duration. Therefore it uses:
 *
 * * udelay() loop for sleep durations <= 10 microseconds to avoid hrtimer
 *   overhead for really short sleep durations.
 * * usleep_range() for sleep durations which would lead with the usage of
 *   msleep() to a slack larger than 25%. This depends on the granularity of
 *   jiffies.
 * * msleep() for all other sleep durations.
 *
 * Note: When %CONFIG_HIGH_RES_TIMERS is not set, all sleeps are processed with
 * the granularity of jiffies and the slack might exceed 25% especially for
 * short sleep durations.
 */
static inline void fsleep(unsigned long usecs)
{
	if (usecs <= 10)
		udelay(usecs);
	else if (usecs < USLEEP_RANGE_UPPER_BOUND)
		usleep_range(usecs, usecs + (usecs >> max_slack_shift));
	else
		msleep(DIV_ROUND_UP(usecs, USEC_PER_MSEC));
}
#endif

#endif /* defined(_LINUX_DELAY_H) */
