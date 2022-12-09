/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    vdso/bits.h
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

#ifndef __VDSO_BITS_H
#define __VDSO_BITS_H

#include <vdso/const.h>

#define BIT(nr)			(UL(1) << (nr))

#endif	/* __VDSO_BITS_H */
