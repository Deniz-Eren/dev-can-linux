/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/bitops.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified by removing contents, such that only function
 *          fls_long() remains from the original file.
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

#ifndef _LINUX_BITOPS_H
#define _LINUX_BITOPS_H

#include <linux/bits.h>
#include <asm-generic/bitops/fls64.h>


static inline unsigned fls_long(unsigned long l)
{
	if (sizeof(l) == 4)
		return fls(l);
	return fls64(l);
}

#endif
