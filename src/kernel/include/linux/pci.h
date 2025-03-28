/* SPDX-License-Identifier: GPL-2.0 */
/*
 * \file    linux/pci.h
 * \brief   This file is originally from the Linux Kernel source-code and has
 *          been modified to integrate to QNX RTOS.
 *
 * \details Changes have been made to remove PCI handling content and only keep
 *          heavily simplified pci_dev and pci_driver structures. Also the
 *          following functions have been turned into stubs that are
 *          reimplemented using QNX functions:
 *              pci_read_config_word()
 *              pci_write_config_word()
 *              pci_enable_device()
 *              pci_disable_device()
 *              pci_set_master()
 *              pci_request_regions()
 *              pci_release_regions()
 *              pci_alloc_irq_vectors()
 *              pci_free_irq_vectors()
 *              pci_iomap()
 *              pci_iounmap()
 *
 *	PCI defines and function prototypes
 *	Copyright 1994, Drew Eckhardt
 *	Copyright 1997--1999 Martin Mares <mj@ucw.cz>
 *
 *	PCI Express ASPM defines and function prototypes
 *	Copyright (c) 2007 Intel Corp.
 *		Zhang Yanmin (yanmin.zhang@intel.com)
 *		Shaohua Li (shaohua.li@intel.com)
 *
 *	For more information, please consult the following manuals (look at
 *	http://www.pcisig.com/ for how to get them):
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

#ifndef LINUX_PCI_H
#define LINUX_PCI_H

#include <errno.h>
#include <pci/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/pci_ids.h>
#include <uapi/linux/pci.h>

struct pci_dev;

extern void __iomem* pci_iomap(struct pci_dev *dev, int bar, unsigned long max);
extern void pci_iounmap(struct pci_dev *dev, void __iomem* addr);
extern void __iomem *pcim_iomap(struct pci_dev *pdev, int bar, unsigned long maxlen);


/* struct pci_dev - describes a PCI device
 *
 * @supported_speeds:	PCIe Supported Link Speeds Vector (+ reserved 0 at
 *			LSB). 0 when the supported speeds cannot be
 *			determined (e.g., for Root Complex Integrated
 *			Endpoints without the relevant Capability
 *			Registers).
 */
struct pci_dev {
    pci_devhdl_t    hdl;        /* QNX type; PCI device handle */
    pci_ba_t*       ba;         /* QNX type; Base address registers */
    int_t           nba;        /* QNX type; Number of base addresses */
    pci_cap_t       msi_cap;    /* QNX type; MSI capability structure */
    pci_cap_t       pcie_cap;   /* QNX type; PCIe capability structure */
    bool            is_msi;     /* MSI support flag */
    bool            is_msix;    /* MSIX support flag */
    void __iomem**  addr;       /* Memory-mapped I/O addresses */

	unsigned int	devfn;		/* Encoded device & function index */
	unsigned short	vendor;
	unsigned short	device;
	unsigned short	subsystem_vendor;
	unsigned short	subsystem_device;

	struct	device	dev;		/* Generic device interface */

	/*
	 * Instead of touching interrupt line and base address registers
	 * directly, use the values stored here. They might be different!
	 */
	unsigned int	irq;
    bool            is_managed;
};

/* Error values that may be returned by PCI functions */
#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED	0x81
#define PCIBIOS_BAD_VENDOR_ID		0x83
#define PCIBIOS_DEVICE_NOT_FOUND	0x86
#define PCIBIOS_BAD_REGISTER_NUMBER	0x87
#define PCIBIOS_SET_FAILED		0x88
#define PCIBIOS_BUFFER_TOO_SMALL	0x89

/* Translate above to generic errno for passing back through non-PCI code */
static inline int pcibios_err_to_errno(int err)
{
	if (err <= PCIBIOS_SUCCESSFUL)
		return err; /* Assume already errno */

	switch (err) {
	case PCIBIOS_FUNC_NOT_SUPPORTED:
		return -ENOENT;
	case PCIBIOS_BAD_VENDOR_ID:
		return -ENOTTY;
	case PCIBIOS_DEVICE_NOT_FOUND:
		return -ENODEV;
	case PCIBIOS_BAD_REGISTER_NUMBER:
		return -EFAULT;
	case PCIBIOS_SET_FAILED:
		return -EIO;
	case PCIBIOS_BUFFER_TOO_SMALL:
		return -ENOSPC;
	}

	return -ERANGE;
}

struct pci_driver {
	const char *name;
	const struct pci_device_id *id_table;	/* must be non-NULL for probe to be called */
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);	/* New device inserted */
	void (*remove) (struct pci_dev *dev);	/* Device removed (NULL if not a hot-plug capable driver) */
};

/**
 * PCI_DEVICE - macro used to describe a specific PCI device
 * @vend: the 16 bit PCI Vendor ID
 * @dev: the 16 bit PCI Device ID
 *
 * This macro is used to create a struct pci_device_id that matches a
 * specific device.  The subvendor and subdevice fields will be set to
 * PCI_ANY_ID.
 */
#define PCI_DEVICE(vend,dev) \
	.vendor = (vend), .device = (dev), \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID

extern int pci_read_config_byte(const struct pci_dev *dev, int where, u8 *val);
extern int pci_read_config_word(const struct pci_dev *dev, int where, u16 *val);
extern int pci_read_config_dword(const struct pci_dev *dev, int where, u32 *val);
extern int pci_write_config_byte(const struct pci_dev *dev, int where, u8 val);
extern int pci_write_config_word(const struct pci_dev *dev, int where, u16 val);
extern int pci_write_config_dword(const struct pci_dev *dev, int where, u32 val);

extern pci_err_t pci_enable_device (struct pci_dev* dev);
extern void pci_disable_device(struct pci_dev *dev);
extern int __must_check pcim_enable_device(struct pci_dev *pdev);

#define PCI_IRQ_LEGACY		(1 << 0) /* Allow legacy interrupts */
#define PCI_IRQ_MSI		(1 << 1) /* Allow MSI interrupts */
#define PCI_IRQ_MSIX		(1 << 2) /* Allow MSI-X interrupts */
#define PCI_IRQ_AFFINITY	(1 << 3) /* Auto-assign affinity */

void pci_set_master(struct pci_dev *dev);

int pci_request_regions(struct pci_dev *, const char *);
void pci_release_regions(struct pci_dev *);

int pci_alloc_irq_vectors(struct pci_dev *dev, unsigned int min_vecs,
			  unsigned int max_vecs, unsigned int flags);

void pci_free_irq_vectors(struct pci_dev *dev);

/* These manipulate per-pci_dev driver-specific data.  They are really just a
 * wrapper around the generic device structure functions of these calls.
 */
static inline void *pci_get_drvdata(struct pci_dev *pdev)
{
	return dev_get_drvdata(&pdev->dev);
}

static inline void pci_set_drvdata(struct pci_dev *pdev, void *data)
{
	dev_set_drvdata(&pdev->dev, data);
}

/*
 * These helpers provide future and backwards compatibility
 * for accessing popular PCI BAR info
 */
extern uintptr_t pci_resource_start (struct pci_dev* dev, int bar);
extern uintptr_t pci_resource_end (struct pci_dev* dev, int bar);
extern uintptr_t pci_resource_len (struct pci_dev* dev, int bar);

#endif /* LINUX_PCI_H */
