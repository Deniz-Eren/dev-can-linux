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

#ifdef __QNX__
extern unsigned long lpj_fine;
void calibrate_delay(void);
void __attribute__((weak)) calibration_delay_done(void);
void msleep(unsigned int msecs);
unsigned long msleep_interruptible(unsigned int msecs);
void usleep_range_state(unsigned long min, unsigned long max,
			unsigned int state);
#endif

static inline void usleep_range(unsigned long min, unsigned long max) {
#ifdef __QNX__
    udelay(min); /* TODO: check this; currently just using usleep min */
#else
	usleep_range_state(min, max, TASK_UNINTERRUPTIBLE);
#endif
}

#ifndef __QNX__
static inline void usleep_idle_range(unsigned long min, unsigned long max)
{
	usleep_range_state(min, max, TASK_IDLE);
}

static inline void ssleep(unsigned int seconds)
{
	msleep(seconds * 1000);
}

/* see Documentation/timers/timers-howto.rst for the thresholds */
static inline void fsleep(unsigned long usecs)
{
	if (usecs <= 10)
		udelay(usecs);
	else if (usecs <= 20000)
		usleep_range(usecs, 2 * usecs);
	else
		msleep(DIV_ROUND_UP(usecs, 1000));
}
#endif

#endif /* defined(_LINUX_DELAY_H) */
