/*
 *	pci.h
 *
 *	PCI defines and function prototypes
 *	Copyright 1994, Drew Eckhardt
 *	Copyright 1997--1999 Martin Mares <mj@ucw.cz>
 *
 *	For more information, please consult the following manuals (look at
 *	http://www.pcisig.com/ for how to get them):
 *
 *	PCI BIOS Specification
 *	PCI Local Bus Specification
 *	PCI to PCI Bridge Specification
 *	PCI System Design Guide
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

extern uintptr_t pci_iomap(struct pci_dev *dev, int bar, unsigned long max);
extern void pci_iounmap(struct pci_dev *dev, uintptr_t p);


/*
 * The pci_dev structure is used to describe PCI devices.
 */
struct pci_dev {
	pci_devhdl_t hdl;
	pci_ba_t *ba;
	int_t nba;

	unsigned int	devfn;		/* encoded device & function index */
	unsigned short	vendor;
	unsigned short	device;
	unsigned short	subsystem_vendor;
	unsigned short	subsystem_device;
//	unsigned int	class;		/* 3 bytes: (base,sub,prog-if) */

	struct	device	dev;		/* Generic device interface */

	/*
	 * Instead of touching interrupt line and base address registers
	 * directly, use the values stored here. They might be different!
	 */
	unsigned int	irq;
};

struct pci_driver {
	const char *name;
	const struct pci_device_id *id_table;	/* must be non-NULL for probe to be called */
	int  (*probe)  (struct pci_dev *dev, const struct pci_device_id *id);	/* New device inserted */
	void (*remove) (struct pci_dev *dev);	/* Device removed (NULL if not a hot-plug capable driver) */
};

extern int pci_read_config_word(const struct pci_dev *dev, int where, u16 *val);
extern int pci_write_config_word(const struct pci_dev *dev, int where, u16 val);

extern int pci_enable_device(struct pci_dev *dev);
extern void pci_disable_device(struct pci_dev *dev);

int pci_request_regions(struct pci_dev *, const char *);
void pci_release_regions(struct pci_dev *);

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

#endif /* LINUX_PCI_H */
