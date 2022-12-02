/*
 * Copyright 2006 PathScale, Inc.  All Rights Reserved.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _LINUX_IO_H
#define _LINUX_IO_H

#include <hw/inout.h>


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
