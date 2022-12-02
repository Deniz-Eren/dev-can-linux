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

struct pci_driver *detected_driver = NULL;


int check_driver_support (const struct pci_driver* driver, const struct driver_selection_t* ds) {
	if (driver->id_table != NULL) {
		const struct pci_device_id *id_table = driver->id_table;

		while (id_table->vendor != 0) {
			if (id_table->vendor == ds->vid &&
				id_table->device == ds->did)
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

int process_driver_selection (struct driver_selection_t* ds) {
	if (!ds) {
		log_err("internal error; driver selection invalid!");

		return -1;
	}

	ds->driver_auto = 0;
	ds->driver_pick = 0;
	ds->driver_ignored = 0;
	ds->driver_unsupported = 0;

	uint_t idx = 0;
	pci_bdf_t bdf = 0;

	while ((bdf = pci_device_find(idx, PCI_VID_ANY, PCI_DID_ANY, PCI_CCODE_ANY)) != PCI_BDF_NONE)
	{
	    pci_err_t r = pci_device_read_vid(bdf, &ds->vid);

	    if (r != PCI_ERR_OK) {
	    	continue;
	    }

	    r = pci_device_read_did(bdf, &ds->did);

	    if (r == PCI_ERR_OK)
	    {
	    	/* does this driver handle this device ? */

	    	struct pci_driver *detected_driver_temp = NULL;

	    	if (check_driver_support(&adv_pci_driver, ds)) {
	    		detected_driver_temp = &adv_pci_driver;
			}
			else if (check_driver_support(&kvaser_pci_driver, ds)) {
				detected_driver_temp = &kvaser_pci_driver;
			}
			else if (check_driver_support(&ems_pci_driver, ds)) {
				detected_driver_temp = &ems_pci_driver;
			}
			else if (check_driver_support(&peak_pci_driver, ds)) {
				detected_driver_temp = &peak_pci_driver;
			}
			else if (check_driver_support(&plx_pci_driver, ds)) {
				detected_driver_temp = &plx_pci_driver;
			}

	    	if (detected_driver_temp) {
				if (!optd || (optd && opt_vid == ds->vid && opt_did == ds->did)) {
					if (!detected_driver) detected_driver = detected_driver_temp;
				}
	    	}

		    if (!optd && detected_driver && detected_driver == detected_driver_temp) {
		    	ds->driver_auto = 1;
		    	log_info("checking device: %x:%x <- auto (%s)\n", ds->vid, ds->did, detected_driver->name);
		    }
		    else if (optd && detected_driver && detected_driver == detected_driver_temp) {
		    	ds->driver_pick = 1;
		    	log_info("checking device: %x:%x <- pick (%s)\n", ds->vid, ds->did, detected_driver->name);
		    }
		    else if (detected_driver_temp) {
		    	ds->driver_ignored = 1;
		    	log_info("checking device: %x:%x <- ignored (%s)\n", ds->vid, ds->did, detected_driver_temp->name);
		    }
		    else if (!detected_driver_temp && opt_vid == ds->vid && opt_did == ds->did) {
		    	ds->driver_unsupported = 1;
		    	log_info("checking device: %x:%x <- unsupported\n", ds->vid, ds->did);
		    }
		    else {
		    	log_info("checking device: %x:%x\n", ds->vid, ds->did);
		    }
	    }

	    /* get next device instance */
	    ++idx;
	}

	return 0;
}

int pci_enable_device(struct pci_dev *dev) {
	log_trace("pci_enable_device: %x:%x\n",
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
				log_err("pci device attach failed; Invalid flags.\n");
				break;
			case PCI_ERR_ENODEV:
				log_err("pci device attach failed; The bdf argument doesn't refer to a valid device.\n");
				break;
			case PCI_ERR_ATTACH_EXCLUSIVE:
				log_err("pci device attach failed; The device identified by bdf is already exclusively owned.\n");
				break;
			case PCI_ERR_ATTACH_SHARED:
				log_err("pci device attach failed; The request was for exclusive attachment, but the device identified by bdf has already been successfully attached to.\n");
				break;
			case PCI_ERR_ATTACH_OWNED:
				log_err("pci device attach failed; The request was for ownership of the device, but the device is already owned.\n");
				break;
			case PCI_ERR_ENOMEM:
				log_err("pci device attach failed; Memory for internal resources couldn't be obtained. This may be a temporary condition.\n");
				break;
			case PCI_ERR_LOCK_FAILURE:
				log_err("pci device attach failed; There was an error related to the creation, acquisition, or use of a synchronization object.\n");
				break;
			case PCI_ERR_ATTACH_LIMIT:
				log_err("pci device attach failed; There have been too many attachments to the device identified by bdf.\n");
				break;
			default:
				log_err("pci device attach failed; Unknown error: %d\n", (int)r);
				break;
			}

			return -1;
        }

        /* read some basic info */
        pci_ssvid_t ssvid;
        pci_ssid_t ssid;
        pci_cs_t cs; /* chassis and slot */

	    if ((r = pci_device_read_ssvid(bdf, &ssvid)) != PCI_ERR_OK) {
    		log_err("error reading ssvid\n");

    		return -1;
	    }

		log_info("read ssvid: %x\n", ssvid);
		dev->subsystem_vendor = ssvid;

	    if ((r = pci_device_read_ssid(bdf, &ssid)) != PCI_ERR_OK) {
    		log_err("error reading ssid\n");

    		return -1;
	    }

		log_info("read ssid: %x\n", ssid);
	    dev->subsystem_device = ssid;

	    cs = pci_device_chassis_slot(bdf);

	    dev->devfn = PCI_DEVFN(PCI_SLOT(cs), PCI_FUNC(bdf));
		log_info("read cs: %x, slot: %x, func: %x, devfn: %x\n", cs, PCI_SLOT(cs), PCI_FUNC(bdf), dev->devfn);

		/* optionally determine capabilities of device */
        uint_t capid_idx = 0;
        pci_capid_t capid;

        /* instead of looping could use pci_device_find_capid() to select which capabilities to use */
        while ((r = pci_device_read_capid(bdf, &capid, capid_idx)) == PCI_ERR_OK)
        {
			log_info("read capability[%d]: %x\n", capid_idx, capid);

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

    			log_info("read ba[%d] { addr: %x, size: %x }\n",
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
    			log_info("read irq[%d]: %d\n", i, irq[i]);
            }

        	if (nirq > 1) {
        		log_err("read multiple (%d) IRQs\n", nirq);

        		return -1;
        	}
        }

	    /* get next device instance */
	    ++idx;
	}

	return 0;
}

void pci_disable_device(struct pci_dev *dev) {
	log_trace("pci_disable_device\n");

	if (dev != NULL) {
		if (dev->hdl != NULL) {
			pci_device_detach(dev->hdl);
		}
	}
}

uintptr_t pci_iomap(struct pci_dev *dev, int bar, unsigned long max) {
	log_trace("pci_iomap; bar: %d, max: %d\n", bar, max);

	if (bar >= dev->nba) {
		log_err("internal error; bar: %d, nba: %d\n", bar, dev->nba);
	}

	if (dev->ba[bar].size > max) {
		dev->ba[bar].size = max;
	}

	/* mmap() the address space(s) */

	if (mmap_device_io(dev->ba[bar].size, dev->ba[bar].addr) == MAP_DEVICE_FAILED) {
		switch (errno) {
		case EINVAL:
			log_err("pci device address mapping failed; Invalid flags type, or len is 0.\n");
			break;
		case ENOMEM:
			log_err("pci device address mapping failed; The address range requested is outside of the allowed process address range, or there wasn't enough memory to satisfy the request.\n");
			break;
		case ENXIO:
			log_err("pci device address mapping failed; The address from io for len bytes is invalid.\n");
			break;
		case EPERM:
			log_err("pci device address mapping failed; The calling process doesn't have the required permission; see procmgr_ability().\n");
			break;
		default:
			log_err("pci device address mapping failed; Unknown error: %d\n", errno);
			break;
		}
	}
	else {
		log_dbg("ba[%d] mapping successful\n", bar);
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

	log_trace("pci_iounmap; bar: %d, size: %d\n", bar, size);

	if (bar == -1 || !size) {
		log_err("internal error; bar size failure during pci_iounmap\n");
	}

	if (munmap_device_io(p, size) == -1) {
		log_err("internal error; munmap_device_io failure\n");
	}
}

int pci_request_regions(struct pci_dev *dev, const char *res_name) {
	log_trace("pci_request_regions\n");

	return -1;
}

void pci_release_regions(struct pci_dev *dev) {
	log_trace("pci_release_regions\n");
}

int pci_read_config_word(const struct pci_dev *dev, int where, u16 *val) {
	log_trace("pci_read_config_word\n");

	return -1;
}

int pci_write_config_word(const struct pci_dev *dev, int where, u16 val) {
	log_trace("pci_write_config_word\n");

	return -1;
}
