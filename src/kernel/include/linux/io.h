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

#include <linux/err.h>
#include <sys/mman.h>
#include <hw/inout.h> /* QNX header */

extern size_t io_port_addr_threshold;


static inline uint8_t read8 (void __iomem* addr) {
    uint8_t ret;

#if defined(__X86__) || defined(__X86_64__)
    if ((uintptr_t)addr < io_port_addr_threshold) {
        ret = in8((uintptr_t __iomem)addr);
    }
    else {
        __cpu_membarrier();
        ret = *(uint8_t __iomem*)addr;
        __cpu_membarrier();
    }
#else
    ret = in8((uintptr_t __iomem)addr);
#endif

    return ret;
}

static inline uint16_t read16 (void __iomem* addr) {
    uint16_t ret;

#if defined(__X86__) || defined(__X86_64__)
    if ((uintptr_t)(addr + 1) < io_port_addr_threshold) {
        ret = in16((uintptr_t __iomem)addr);
    }
    else {
        __cpu_membarrier();
        ret = __cpu_unaligned_ret16(addr);
        __cpu_membarrier();
    }
#else
    ret = in16((uintptr_t __iomem)addr);
#endif

    return ret;
}

static inline uint32_t read32 (void __iomem* addr) {
    uint32_t ret;

#if defined(__X86__) || defined(__X86_64__)
    if ((uintptr_t)(addr + 3) < io_port_addr_threshold) {
        ret = in32((uintptr_t __iomem)addr);
    }
    else {
        __cpu_membarrier();
        ret = __cpu_unaligned_ret32(addr);
        __cpu_membarrier();
    }
#else
    ret = in32((uintptr_t __iomem)addr);
#endif

    return ret;
}

static inline void write8 (void __iomem* addr, uint8_t val) {
#if defined(__X86__) || defined(__X86_64__)
    if ((uintptr_t)addr < io_port_addr_threshold) {
        out8((__iomem uintptr_t)addr, val);
    }
    else {
        __cpu_membarrier();
        *(uint8_t __iomem*)addr = val;
    }
#else
    out8((__iomem uintptr_t)addr, val);
#endif
}

static inline void write16 (void __iomem* addr, uint16_t val) {
#if defined(__X86__) || defined(__X86_64__)
    if ((uintptr_t)(addr + 1) < io_port_addr_threshold) {
        out16((__iomem uintptr_t)addr, val);
    }
    else {
        __cpu_membarrier();
        __cpu_unaligned_put16(addr, val);
    }
#else
    out16((__iomem uintptr_t)addr, val);
#endif
}

static inline void write32 (void __iomem* addr, uint32_t val) {
#if defined(__X86__) || defined(__X86_64__)
    if ((uintptr_t)(addr + 3) < io_port_addr_threshold) {
        out32((__iomem uintptr_t)addr, val);
    }
    else {
        __cpu_membarrier();
        __cpu_unaligned_put32(addr, val);
    }
#else
    out32((__iomem uintptr_t)addr, val);
#endif
}

/* Define mapping to QNX IO */
#define readb(addr)         read8(addr)
#define readw(addr)         read16(addr)
#define readl(addr)         read32(addr)

#define ioread8(addr)       readb(addr)
#define ioread16(addr)      readw(addr)
#define ioread32(addr)      readl(addr)

#define writeb(v, addr)     write8(addr, v)
#define writew(v, addr)     write16(addr, v)
#define writel(v, addr)     write32(addr, v)

#define iowrite8(v, addr)   writeb(v, addr)
#define iowrite16(v, addr)  writew(v, addr)
#define iowrite32(v, addr)  writel(v, addr)

#endif /* _LINUX_IO_H */
