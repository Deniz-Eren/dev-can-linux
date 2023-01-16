/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_X86_IO_H
#define _ASM_X86_IO_H

/**
 * ioremap     -   map bus memory into CPU space
 * @offset:    bus address of the memory
 * @size:      size of the resource to map
 *
 * ioremap performs a platform specific sequence of operations to
 * make bus memory CPU accessible via the readb/readw/readl/writeb/
 * writew/writel functions and the other mmio helpers. The returned
 * address is not guaranteed to be usable directly as a virtual
 * address.
 *
 * If the area you are trying to map is a PCI BAR you should have a
 * look at pci_iomap().
 */
void __iomem* ioremap (uintptr_t offset, size_t size);

#endif /* _ASM_X86_IO_H */
