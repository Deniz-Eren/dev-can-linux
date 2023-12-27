/*
 * \file    pci.c
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

#include <stdio.h>
#include <string.h>

#include <sys/neutrino.h>
#include <sys/mman.h>

#include "config.h"
#include "pci.h"
#include "pci-capability.h"

#include <linux/io.h>

size_t io_port_addr_threshold = 0x0; // used in linux/io.h

// IO and MEM address space tracking
size_t pci_io_addr_space_begin = 0x0;
size_t pci_io_addr_space_end = 0x0;
size_t pci_mem_addr_space_begin = 0x0;
size_t pci_mem_addr_space_end = 0x0;

driver_selection_t* driver_selection_root = NULL;
driver_selection_t* probe_driver_selection = NULL;
int next_device_id = 0;

/* Helper structures */
bar_t* bar_list_root = NULL;
ioblock_t* ioblock_root = NULL;
pthread_mutex_t ioblock_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * TODO: use these error codes PCIBIOS_* from linux/pci.h for return values
 */

static int check_driver_support (const struct pci_driver* driver,
        pci_vid_t vid, pci_did_t did)
{
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

int process_driver_selection() {
    uint_t idx = 0;
    pci_bdf_t bdf = 0;

    while ((bdf = pci_device_find(
            idx, PCI_VID_ANY, PCI_DID_ANY, PCI_CCODE_ANY) ) != PCI_BDF_NONE)
    {
        pci_vid_t vid;
        pci_did_t did;

        pci_err_t r = pci_device_read_vid(bdf, &vid);

        if (r != PCI_ERR_OK) {
            continue;
        }

        r = pci_device_read_did(bdf, &did);

        if (r == PCI_ERR_OK)
        {
            /* does this driver handle this device ? */

            struct pci_driver* detected_driver_temp = NULL;

            if (check_driver_support(&adv_pci_driver, vid, did)) {
                detected_driver_temp = &adv_pci_driver;
            }
            else if (check_driver_support(&kvaser_pci_driver, vid, did)) {
                detected_driver_temp = &kvaser_pci_driver;
            }
            else if (check_driver_support(&ems_pci_driver, vid, did)) {
                detected_driver_temp = &ems_pci_driver;
            }
            else if (check_driver_support(&peak_pci_driver, vid, did)) {
                detected_driver_temp = &peak_pci_driver;
            }
            else if (check_driver_support(&plx_pci_driver, vid, did)) {
                detected_driver_temp = &plx_pci_driver;
            }

            if (detected_driver_temp) {
                bool device_disabled = false;

                int i;
                for (i = 0; i < num_disable_device_configs; ++i) {
                    if (disable_device_config[i].vid == vid &&
                        disable_device_config[i].did == did &&
                        disable_device_config[i].cap == -1)
                    {
                        device_disabled = true;

                        break;
                    }
                }

                if (!optd || !device_disabled) {
                    store_driver_selection(vid, did, detected_driver_temp);
                }
            }
        }

        /* get next device instance */
        ++idx;
    }

    return 0;
}

pci_err_t pci_enable_device (struct pci_dev* dev) {
    log_trace("pci_enable_device: %04x:%04x\n",
            dev->vendor, dev->device);

    uint_t idx = 0;
    pci_bdf_t bdf;

    while ((bdf = pci_device_find(
            idx, dev->vendor, dev->device, PCI_CCODE_ANY) ) != PCI_BDF_NONE)
    {
        if (idx > 0) {
            log_err("only single device per vendor and device id combination "
                    "supported\n");

            return PCI_ERR_ENOTSUP; // Not supported
        }

        pci_err_t r;

        dev->hdl = pci_device_attach(bdf, pci_attachFlags_EXCLUSIVE_OWNER, &r);

        if (dev->hdl == NULL) {
            log_err("pci_device_attach error: %s\n", pci_strerror(r));

            log_warn("only a single instance of the driver is allowed\n");

            return r;
        }

        /*
         * Read some basic info
         */

        pci_ssvid_t ssvid;
        pci_ssid_t ssid;
        pci_cs_t cs; /* chassis and slot */

        if ((r = pci_device_read_ssvid(bdf, &ssvid)) != PCI_ERR_OK) {
            log_err("pci_device_read_ssvid error: %s\n", pci_strerror(r));

            return r;
        }

        log_info("read ssvid: %04x\n", ssvid);
        dev->subsystem_vendor = ssvid;

        if ((r = pci_device_read_ssid(bdf, &ssid)) != PCI_ERR_OK) {
            log_err("pci_device_read_ssid error: %s\n", pci_strerror(r));

            return r;
        }

        log_info("read ssid: %04x\n", ssid);
        dev->subsystem_device = ssid;

        cs = pci_device_chassis_slot(bdf);

        dev->devfn = PCI_DEVFN(PCI_SLOT(cs), PCI_FUNC(bdf));

        log_info("read cs: %x, slot: %x, func: %x, devfn: %x\n",
                cs, PCI_SLOT(cs), PCI_FUNC(bdf), dev->devfn);

        /*
         * Load capabilities
         */

        msix_init(dev);

        /*
         * Process bar info
         */

#define MAX_NUM_BA  32 /* Making this larger than what should be needed */

        pci_ba_t ba[MAX_NUM_BA];    /* the maximum number of entries that can
                                       be returned */
        int_t nba = NELEMENTS(ba);

        /* read the address space information */
        r = pci_device_read_ba(dev->hdl, &nba, ba, pci_reqType_e_UNSPECIFIED);

        if ((r == PCI_ERR_OK) && (nba > 0))
        {
            dev->nba = nba;
            dev->ba = (pci_ba_t*)malloc(nba*sizeof(pci_ba_t));
            dev->addr = (void __iomem**)malloc(nba*sizeof(void*));

            for (int_t i = 0; i < nba; i++) {
                dev->ba[i] = ba[i];
                store_bar(ba[i]);

                char type[16];

                size_t new_begin = dev->ba[i].addr;
                size_t new_end = dev->ba[i].addr + dev->ba[i].size;

                if (dev->ba[i].type == pci_asType_e_IO) {
                    snprintf(type, 16, "I/O");

                    // Update IO address space tracking
                    if (pci_io_addr_space_begin == 0x0 ||
                        pci_io_addr_space_end == 0x0)
                    {
                        pci_io_addr_space_begin = new_begin;
                        pci_io_addr_space_end = new_end;
                    }
                    else {
                        if (pci_io_addr_space_begin > new_begin) {
                            pci_io_addr_space_begin = new_begin;
                        }

                        if (pci_io_addr_space_end < new_end) {
                            pci_io_addr_space_end = new_end;
                        }
                    }
                }
                else if (dev->ba[i].type == pci_asType_e_MEM) {
                    snprintf(type, 16, "MEM");

                    // Update MEM address space tracking
                    if (pci_mem_addr_space_begin == 0x0 ||
                        pci_mem_addr_space_end == 0x0)
                    {
                        pci_mem_addr_space_begin = new_begin;
                        pci_mem_addr_space_end = new_end;
                    }
                    else {
                        if (pci_mem_addr_space_begin > new_begin) {
                            pci_mem_addr_space_begin = new_begin;
                        }

                        if (pci_mem_addr_space_end < new_end) {
                            pci_mem_addr_space_end = new_end;
                        }
                    }
                }
                else {
                    log_err("pci_enable_device error; unknown PCI region "
                            "type: %d\n", dev->ba[i].type);

                    return PCI_ERR_ENOTSUP; // Not supported
                }

                log_info("read ba[%d] %s { addr: %x, size: %x }\n",
                        i, type, (u32)dev->ba[i].addr, (u32)dev->ba[i].size);

                // Check IO vs MEM space clash and update io_port_addr_threshold
                if (pci_io_addr_space_end > pci_mem_addr_space_begin &&
                    pci_io_addr_space_end != 0x0 &&
                    pci_mem_addr_space_begin != 0x0)
                {
                    log_err("pci_enable_device error; IO/MEM space clash "
                            "detected; [%p:%p] vs [%p:%p]\n",
                            (void*)pci_io_addr_space_begin,
                            (void*)pci_io_addr_space_end,
                            (void*)pci_mem_addr_space_begin,
                            (void*)pci_mem_addr_space_end);

                    return PCI_ERR_ENOTSUP; // Not supported
                }

                // Used in linux/io.h
                io_port_addr_threshold = pci_io_addr_space_end;

                log_trace("io threshold: %p; I/O[%p:%p], MEM[%p:%p]\n",
                        (void*)io_port_addr_threshold,
                        (void*)pci_io_addr_space_begin,
                        (void*)pci_io_addr_space_end,
                        (void*)pci_mem_addr_space_begin,
                        (void*)pci_mem_addr_space_end);
            }
        }
#undef MAX_NUM_BA

        /*
         * Process IRQ info
         */

        pci_irq_t irq[32];
        int_t nirq = NELEMENTS(irq);

        /* read the irq information */
        r = pci_device_read_irq(dev->hdl, &nirq, irq);

        if ((r == PCI_ERR_OK) && (nirq > 0))
        {
            dev->irq = irq[0];

            for (int_t i=0; i<nirq; i++) {
                log_info("read irq[%d]: %d\n", i, irq[i]);
            }

            irq_group_add( irq, nirq, dev->hdl, dev->msi_cap,
                    dev->is_msi, dev->is_msix );
        }

        if (dev->irq == 0) {
            log_err("failed to read any IRQs\n");

            return PCI_ERR_ENOTSUP; // Not supported
        }

        /* get next device instance */
        ++idx;
    }

    return PCI_ERR_OK;
}

void pci_disable_device (struct pci_dev* dev) {
    log_trace("pci_disable_device\n");

    if (pci_bdf(dev->hdl) == PCI_BDF_NONE) {
        log_err("pci device not enabled\n");

        return;
    }

    if (dev->hdl == NULL) {
        log_err("pci device not attached\n");

        return;
    }

    msix_uninit(dev);

    if (dev != NULL) {
        if (dev->hdl != NULL) {
            pci_device_detach(dev->hdl);
        }
    }
}

void __iomem* pci_iomap (struct pci_dev* dev, int bar, unsigned long max) {
    log_trace("pci_iomap; bar: %d, addr: %p, max: %p\n", bar,
            (void*)(unsigned long)dev->ba[bar].addr, (void*)max);

    if (bar >= dev->nba) {
        log_err("internal error; bar: %d, nba: %d\n", bar, dev->nba);
    }

    /* mmap() the address space(s) */

    void __iomem* memptr = NULL;

    if (dev->ba[bar].type == pci_asType_e_MEM) {
        memptr = mmap_device_memory(
                0,
                dev->ba[bar].size,
                PROT_READ | PROT_WRITE | PROT_NOCACHE,
                0,
                dev->ba[bar].addr );

        if (memptr == MAP_FAILED) {
            log_err("pci device address mapping failed; %s\n", strerror(errno));

            return NULL;
        }
    }
    else if (dev->ba[bar].type == pci_asType_e_IO) {
        memptr = (void __iomem*)mmap_device_io(
                dev->ba[bar].size, dev->ba[bar].addr );

        if ((uintptr_t)memptr == MAP_DEVICE_FAILED) {
            log_err("pci device address mapping failed; %s\n", strerror(errno));

            return NULL;
        }
    }
    else {
        log_err("ioremap error: unknown ba type\n");

        return NULL;
    }

    log_dbg("ba[%d] mapping successful\n", bar);

    dev->addr[bar] = memptr;
    store_block(dev->addr[bar], dev->ba[bar].size, dev->ba[bar]);

    return dev->addr[bar];
}

uintptr_t pci_resource_start (struct pci_dev* dev, int bar) {
    log_trace("pci_resource_start; bar: %d, addr: %p\n", bar,
            (void*)(unsigned long)dev->ba[bar].addr);

    return dev->ba[bar].addr;
}

void __iomem* ioremap (uintptr_t offset, size_t size) {
    log_trace("ioremap; offset: %p, size: %p\n",
            (void*)offset, (void*)size);

    /* mmap() the address space(s) */

    bar_t* bar = get_bar(offset);

    if (bar == NULL) {
        log_err("get_bar fail\n");

        return NULL;
    }

    void __iomem* memptr = NULL;

    if (bar->ba.type == pci_asType_e_MEM) {
        memptr = mmap_device_memory(
                0,
                size,
                PROT_READ | PROT_WRITE | PROT_NOCACHE,
                0,
                offset );

        if (memptr == MAP_FAILED) {
            log_err("pci device address mapping failed; %s\n", strerror(errno));

            return NULL;
        }
    }
    else if (bar->ba.type == pci_asType_e_IO) {
        memptr = (void __iomem*)mmap_device_io(size, offset);

        if ((uintptr_t)memptr == MAP_DEVICE_FAILED) {
            log_err("pci device address mapping failed; %s\n", strerror(errno));

            return NULL;
        }
    }
    else {
        log_err("ioremap error: unknown ba type\n");

        return NULL;
    }

    log_dbg("ioremap [%p] mapping to [%p] successful\n",
            (void*)offset, memptr);

    store_block(memptr, size, bar->ba);

    return memptr;
}

void pci_iounmap (struct pci_dev* dev, void __iomem* addr) {
    log_trace("pci_iounmap; addr: %p\n", addr);

    ioblock_t* block = remove_block(addr);

    if (block == NULL) {
        log_err("pci_iounmap; remove_block error\n");

        return;
    }

    log_trace("pci_iounmap; addr: %p, size: %p\n",
            block->addr, (void*)block->size);

    if (block->ba.type == pci_asType_e_MEM) {
        if (munmap_device_memory((void*)block->addr, block->size) == -1) {
            log_err("internal error; munmap_device_memory failure: %s\n",
                    strerror(errno));
        }
    }
    else if (block->ba.type == pci_asType_e_IO) {
        if (munmap_device_io((uintptr_t)block->addr, block->size) == -1) {
            log_err("internal error; munmap_device_io failure: %s\n",
                    strerror(errno));
        }
    }
    else {
        log_err("pci_iounmap error: unknown ba type\n");

        return;
    }

    free(block);
}

void pci_set_master (struct pci_dev *dev) {
    log_trace("pci_set_master\n");
}

int pci_request_regions (struct pci_dev* dev, const char* res_name) {
    log_trace("pci_request_regions\n");

    // Return OK we will perform this check during pci_enable_device()
    return 0;
}

void pci_release_regions (struct pci_dev* dev) {
    log_trace("pci_release_regions\n");

    // We will perform this in pci_disable_device()
}

int pci_alloc_irq_vectors (struct pci_dev *dev,
        unsigned int min_vecs, unsigned int max_vecs, unsigned int flags)
{
    log_trace("pci_alloc_irq_vectors\n");

    // We will perform this in pci_enable_device()

    if (dev->msi_cap) {
        return 1;
    }

    return 0;
}

void pci_free_irq_vectors (struct pci_dev *dev) {
    log_trace("pci_free_irq_vectors\n");

    // We will perform this in pci_disable_device()
}

/**
 * Important Note!
 *
 * Start offset of the device dependent portion of the 256/4096 byte PCI/PCIe
 * configuration space from 0x40 to 0xFF/0xFFF.
 *
 * In Linux function pci_read_config_word() and pci_write_config_word() address
 * "where" starts at 0x40.
 *
 * In QNX functions pci_device_cfg_rd*() and pci_device_cfg_wr*() have absolute
 * address offset and it starts at 0x0. These functions will return an error if
 * the offset is below 0x40.
 *
 * Thus to translate from Linux to QNX we must add 0x40 offset to address.
 */
#define PCI_CFG_SPACE_OFFSET 0x40

int pci_read_config_word (const struct pci_dev* dev, int where, u16* val) {
    pci_bdf_t bdf = pci_bdf(dev->hdl);

    if (bdf == PCI_BDF_NONE) {
        return -1;
    }

    pci_err_t err = pci_device_cfg_rd16( bdf,
            where + PCI_CFG_SPACE_OFFSET /* See Important Note above */,
                             val );

    if (err != PCI_ERR_OK) {
        return -1;
    }

    return 0;
}

int pci_write_config_word (const struct pci_dev* dev, int where, u16 val) {
    pci_err_t err = pci_device_cfg_wr16( dev->hdl,
            where + PCI_CFG_SPACE_OFFSET /* See Important Note above */,
                             val, NULL );

    if (err != PCI_ERR_OK) {
        return -1;
    }

    return 0;
}
