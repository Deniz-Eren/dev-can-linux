/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/can/platform/sja1000.h
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

#ifndef _CAN_PLATFORM_SJA1000_H
#define _CAN_PLATFORM_SJA1000_H

/* clock divider register */
#define CDR_CLKOUT_MASK 0x07
#define CDR_CLK_OFF	0x08 /* Clock off (CLKOUT pin) */
#define CDR_RXINPEN	0x20 /* TX1 output is RX irq output */
#define CDR_CBP		0x40 /* CAN input comparator bypass */
#define CDR_PELICAN	0x80 /* PeliCAN mode */

/* output control register */
#define OCR_MODE_BIPHASE  0x00
#define OCR_MODE_TEST     0x01
#define OCR_MODE_NORMAL   0x02
#define OCR_MODE_CLOCK    0x03
#define OCR_MODE_MASK     0x03
#define OCR_TX0_INVERT    0x04
#define OCR_TX0_PULLDOWN  0x08
#define OCR_TX0_PULLUP    0x10
#define OCR_TX0_PUSHPULL  0x18
#define OCR_TX1_INVERT    0x20
#define OCR_TX1_PULLDOWN  0x40
#define OCR_TX1_PULLUP    0x80
#define OCR_TX1_PUSHPULL  0xc0
#define OCR_TX_MASK       0xfc
#define OCR_TX_SHIFT      2

struct sja1000_platform_data {
	u32 osc_freq;	/* CAN bus oscillator frequency in Hz */

	u8 ocr;		/* output control register */
	u8 cdr;		/* clock divider register */
};

#endif	/* !_CAN_PLATFORM_SJA1000_H */
