/*
 * \file    linux/slab.h
 * \brief   General memory allocation functions.
 *
 * \details This file has the exact same name and path as the one supplied by
 *          the Linux Kernel source-code, however the contents have been
 *          completely replaced. This has been done to keep the rest of the
 *          Linux Kernel source-code unchanged so that ongoing updates to the
 *          Linux Kernel can be propagated to this project easily.
 *          Macros have been defined to redirect calls to kzalloc() and kfree()
 *          to simply malloc() and free() functions.
 *          TODO: need to verify if all usage is non-ISR otherwise ISR usage
 *          must move to fixed-block allocator implementation.
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

#ifndef _LINUX_SLAB_H
#define	_LINUX_SLAB_H

#include <linux/types.h>

/* Define mapping of kzalloc to simply malloc */
#define kzalloc(size, gfp) malloc(size)
#define kfree(ptr) free(ptr)

#endif	/* _LINUX_SLAB_H */
