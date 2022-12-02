/*
 * pci.c
 *
 *  Created on: Dec 2, 2022
 *      Author: Deniz Eren
 */

#include <stdio.h>
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <kernel/include/linux/pci.h>


int check_driver_support (const struct pci_driver* driver, pci_vid_t vid, pci_did_t did) {
	if (driver->id_table != NULL) {
		const struct pci_device_id *id_table = driver->id_table;

		while (id_table->vendor != 0) {
			if (id_table->vendor == vid &&
				id_table->device == did)
			{
				return 1;
			}

			++id_table;
		}
	}

	return 0;
}

void print_card (FILE* file, const struct pci_driver* driver) {
	fprintf(file, "  Driver: %s\n", driver->name);
	fprintf(file, "  Supported devices (detailed):\n");

	if (driver->id_table != NULL) {
		const struct pci_device_id *id_table = driver->id_table;

		while (id_table->vendor != 0) {
			fprintf(file, "    { vendor: %x, device: %x, subvendor: %x, subdevice: %x, class: %x, class_mask: %x\n",
				id_table->vendor,
				id_table->device,
				id_table->subvendor,
				id_table->subdevice,
				id_table->class,
				id_table->class_mask);
			++id_table;
		}
	}
}

int pci_enable_device(struct pci_dev *dev) {
	syslog(LOG_DEBUG, "pci_enable_device: %x:%x",
			dev->vendor, dev->device);

	uint_t idx = 0;
	pci_bdf_t bdf = 0;

	while ((bdf = pci_device_find(idx, dev->vendor, dev->device, PCI_CCODE_ANY)) != PCI_BDF_NONE)
	{
	    pci_err_t r;

		pci_devhdl_t hdl = pci_device_attach(bdf, pci_attachFlags_EXCLUSIVE_OWNER, &r);
		dev->hdl = hdl;

        if (hdl == NULL) {
			switch (r) {
			case PCI_ERR_EINVAL:
				syslog(LOG_ERR, "pci device attach failed; Invalid flags.");
				break;
			case PCI_ERR_ENODEV:
				syslog(LOG_ERR, "pci device attach failed; The bdf argument doesn't refer to a valid device.");
				break;
			case PCI_ERR_ATTACH_EXCLUSIVE:
				syslog(LOG_ERR, "pci device attach failed; The device identified by bdf is already exclusively owned.");
				break;
			case PCI_ERR_ATTACH_SHARED:
				syslog(LOG_ERR, "pci device attach failed; The request was for exclusive attachment, but the device identified by bdf has already been successfully attached to.");
				break;
			case PCI_ERR_ATTACH_OWNED:
				syslog(LOG_ERR, "pci device attach failed; The request was for ownership of the device, but the device is already owned.");
				break;
			case PCI_ERR_ENOMEM:
				syslog(LOG_ERR, "pci device attach failed; Memory for internal resources couldn't be obtained. This may be a temporary condition.");
				break;
			case PCI_ERR_LOCK_FAILURE:
				syslog(LOG_ERR, "pci device attach failed; There was an error related to the creation, acquisition, or use of a synchronization object.");
				break;
			case PCI_ERR_ATTACH_LIMIT:
				syslog(LOG_ERR, "pci device attach failed; There have been too many attachments to the device identified by bdf.");
				break;
			default:
				syslog(LOG_ERR, "pci device attach failed; Unknown error: %d", (int)r);
				break;
			}

			return -1;
        }

        /* read some basic info */
        pci_ssvid_t ssvid;
        pci_ssid_t ssid;
        pci_cs_t cs; /* chassis and slot */

	    if ((r = pci_device_read_ssvid(bdf, &ssvid)) != PCI_ERR_OK) {
    		syslog(LOG_ERR, "error reading ssvid");

    		return -1;
	    }

		syslog(LOG_INFO, "read ssvid: %x", ssvid);
		dev->subsystem_vendor = ssvid;

	    if ((r = pci_device_read_ssid(bdf, &ssid)) != PCI_ERR_OK) {
    		syslog(LOG_ERR, "error reading ssid");

    		return -1;
	    }

		syslog(LOG_INFO, "read ssid: %x", ssid);
	    dev->subsystem_device = ssid;

	    cs = pci_device_chassis_slot(bdf);

	    dev->devfn = PCI_DEVFN(PCI_SLOT(cs), PCI_FUNC(bdf));
		syslog(LOG_INFO, "read cs: %x, slot: %x, func: %x, devfn: %x", cs, PCI_SLOT(cs), PCI_FUNC(bdf), dev->devfn);

		/* optionally determine capabilities of device */
        uint_t capid_idx = 0;
        pci_capid_t capid;

        /* instead of looping could use pci_device_find_capid() to select which capabilities to use */
        while ((r = pci_device_read_capid(bdf, &capid, capid_idx)) == PCI_ERR_OK)
        {
			syslog(LOG_INFO, "read capability[%d]: %x", capid_idx, capid);

            /* get next capability ID */
            ++capid_idx;
        }

        pci_ba_t ba[7];    // the maximum number of entries that can be returned
        int_t nba = NELEMENTS(ba);

        /* read the address space information */
        r = pci_device_read_ba(hdl, &nba, ba, pci_reqType_e_UNSPECIFIED);
        if ((r == PCI_ERR_OK) && (nba > 0))
        {
			dev->nba = nba;
        	dev->ba = (pci_ba_t*)malloc(nba*sizeof(pci_ba_t));

            for (int_t i=0; i<nba; i++)
            {
            	dev->ba[i] = ba[i];

    			syslog(LOG_INFO, "read ba[%d] { addr: %x, size: %x }",
    					i, ba[i].addr, ba[i].size);
            }
        }

        pci_irq_t irq[10];
        int_t nirq = NELEMENTS(irq);

        /* read the irq information */
        r = pci_device_read_irq(hdl, &nirq, irq);
        if ((r == PCI_ERR_OK) && (nirq > 0))
        {
        	dev->irq = irq[0];

            for (int_t i=0; i<nirq; i++) {
    			syslog(LOG_INFO, "read irq[%d]: %d", i, irq[i]);
            }

        	if (nirq > 1) {
        		syslog(LOG_ERR, "read multiple (%d) IRQs", nirq);

        		return -1;
        	}
        }

	    /* get next device instance */
	    ++idx;
	}

	return 0;
}

void pci_disable_device(struct pci_dev *dev) {
	syslog(LOG_DEBUG, "pci_disable_device");

	if (dev != NULL) {
		if (dev->hdl != NULL) {
			pci_device_detach(dev->hdl);
		}
	}
}

uintptr_t pci_iomap(struct pci_dev *dev, int bar, unsigned long max) {
	syslog(LOG_DEBUG, "pci_iomap; bar: %d, max: %d", bar, max);

	if (bar >= dev->nba) {
		syslog(LOG_ERR, "internal error; bar: %d, nba: %d", bar, dev->nba);
	}

	if (dev->ba[bar].size > max) {
		dev->ba[bar].size = max;
	}

	/* mmap() the address space(s) */

	if (mmap_device_io(dev->ba[bar].size, dev->ba[bar].addr) == MAP_DEVICE_FAILED) {
		switch (errno) {
		case EINVAL:
			syslog(LOG_ERR, "pci device address mapping failed; Invalid flags type, or len is 0.");
			break;
		case ENOMEM:
			syslog(LOG_ERR, "pci device address mapping failed; The address range requested is outside of the allowed process address range, or there wasn't enough memory to satisfy the request.");
			break;
		case ENXIO:
			syslog(LOG_ERR, "pci device address mapping failed; The address from io for len bytes is invalid.");
			break;
		case EPERM:
			syslog(LOG_ERR, "pci device address mapping failed; The calling process doesn't have the required permission; see procmgr_ability().");
			break;
		default:
			syslog(LOG_ERR, "pci device address mapping failed; Unknown error: %d", errno);
			break;
		}
	}
	else {
		syslog(LOG_DEBUG, "ba[%d] mapping successful", bar);
	}

	return dev->ba[bar].addr;
}

void pci_iounmap(struct pci_dev *dev, uintptr_t p) {
	int bar = -1;
	uint64_t size = 0u;

	for (int i = 0; i < dev->nba; ++i) {
		if (dev->ba[i].addr == p) {
			bar = i;
			size = dev->ba[i].size;
			break;
		}
	}

	syslog(LOG_DEBUG, "pci_iounmap; bar: %d, size: %d", bar, size);

	if (bar == -1 || !size) {
		syslog(LOG_ERR, "internal error; bar size failure during pci_iounmap");
	}

	if (munmap_device_io(p, size) == -1) {
		syslog(LOG_ERR, "internal error; munmap_device_io failure");
	}
}

int pci_request_regions(struct pci_dev *dev, const char *res_name) {
	syslog(LOG_DEBUG, "pci_request_regions");

	return -1;
}

void pci_release_regions(struct pci_dev *dev) {
	syslog(LOG_DEBUG, "pci_release_regions");
}

int pci_read_config_word(const struct pci_dev *dev, int where, u16 *val) {
	syslog(LOG_DEBUG, "pci_read_config_word");

	return -1;
}

int pci_write_config_word(const struct pci_dev *dev, int where, u16 val) {
	syslog(LOG_DEBUG, "pci_write_config_word");

	return -1;
}
