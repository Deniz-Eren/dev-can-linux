/*
 * \file    linux/io.h
 * \brief   Read and write functions to access device I/O registers mapped using
 *          QNX mmap_device_io() function.
 *
 * \details This file has the exact same name and path as the one supplied by
 *          the Linux Kernel source-code, however the contents have been
 *          completely replaced. This has been done to keep the rest of the
 *          Linux Kernel source-code unchanged so that ongoing updates to the
 *          Linux Kernel can be propagated to this project easily.
 *          Macros have been defined to redirect calls to read*(), ioread*(),
 *          write*() and iowrite*() to QNX equivalents in*() and out*()
 *          functions.
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

#ifndef _LINUX_IO_H
#define _LINUX_IO_H

#include <hw/inout.h> /* QNX header */


/* Define mapping to QNX IO */
#define readb(addr)         in8(addr)
#define readw(addr)         in16(addr)
#define readl(addr)         in32(addr)

#define ioread8(addr)       readb(addr)
#define ioread16(addr)      readw(addr)
#define ioread32(addr)      readl(addr)

#define writeb(v, addr)     out8((addr), (v))
#define writew(v, addr)     out16((addr), (v))
#define writel(v, addr)     out32((addr), (v))

#define iowrite8(v, addr)   writeb((v), (addr))
#define iowrite16(v, addr)  writew((v), (addr))
#define iowrite32(v, addr)  writel((v), (addr))

#endif /* _LINUX_IO_H */
