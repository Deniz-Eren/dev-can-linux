/*
 * \file    pci-capability.c
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
#include <pci/cap_msi.h>
#include <pci/cap_msix.h>
#include <pci/cap_pcie.h>

#include "config.h"
#include "pci.h"
#include "pci-capability.h"

#include <linux/io.h>


static inline pci_err_t pcie_init (struct pci_dev* dev) {
    pci_err_t r;
    uint_t capid = CAPID_PCIe;
    int_t cap_index = -1;

    cap_index = pci_device_find_capid(pci_bdf(dev->hdl), capid);

    if (cap_index == -1) {
        return PCI_ERR_ENOTSUP; // Not supported
    }

    r = pci_device_read_cap(pci_bdf(dev->hdl), &dev->pcie_cap, cap_index);

    if (r != PCI_ERR_OK) {
        log_err("pci_device_read_cap error; %s\n", pci_strerror(r));

        return r;
    }

    log_info("read capability[%d]: 0x%02x\n", cap_index, capid);

    r = pci_device_cfg_cap_enable( dev->hdl,
            pci_reqType_e_MANDATORY, dev->pcie_cap );

    if (r != PCI_ERR_OK && r != PCI_ERR_EALREADY) {
        log_err("pci_device_cfg_cap_enable error; %s\n", pci_strerror(r));

        return r;
    }

    if (r == PCI_ERR_EALREADY) {
        log_info("capability 0x%02x (PCIe) already enabled\n", capid);
    }
    else {
        log_info("capability 0x%02x (PCIe) enabled\n", capid);
    }

    int_t pcie_version = cap_pcie_version(dev->pcie_cap);

    log_info("PCIe version: %d\n", pcie_version);

    return PCI_ERR_OK;
}

pci_err_t msix_init (struct pci_dev* dev) {
    pci_err_t r;
    int i;

    /*
     * By default MSI-X is disabled because it is still an experimental feature
     * that has not been validated with real hardware testing. To use it the
     * command-line options -e must be used.
     *
     * MSI is a hardware verified feature and is enabled by default.
     */

    bool disabled_msi = false;
    bool disabled_msix = true;

    pcie_init(dev);

    dev->msi_cap = NULL;

    /*
     * Enable MSI or MSI-X base on program options
     */

    for (i = 0; i < num_enable_device_cap_configs; ++i) {
        if (enable_device_cap_config[i].vid == dev->vendor &&
            enable_device_cap_config[i].did == dev->device &&
            enable_device_cap_config[i].cap == CAPID_MSI)
        {
            disabled_msi = false;

            break;
        }
    }

    for (i = 0; i < num_enable_device_cap_configs; ++i) {
        if (enable_device_cap_config[i].vid == dev->vendor &&
            enable_device_cap_config[i].did == dev->device &&
            enable_device_cap_config[i].cap == CAPID_MSIX)
        {
            disabled_msix = false;

            break;
        }
    }

    /*
     * Disable MSI or MSI-X base on program options (this takes precedence)
     */

    for (i = 0; i < num_disable_device_configs; ++i) {
        if (disable_device_config[i].vid == dev->vendor &&
            disable_device_config[i].did == dev->device)
        {
            if (disable_device_config[i].cap == 0x05) {
                if (disabled_msi)
                    continue;

                disabled_msi = true;
            }
            else if (disable_device_config[i].cap == 0x11) {
                if (disabled_msix)
                    continue;

                disabled_msix = true;
            }
            else continue;

            log_info( "capability 0x%02x disabled for %x:%x\n",
                    disable_device_config[i].cap,
                    disable_device_config[i].vid,
                    disable_device_config[i].did );
        }
    }

    int_t cap_index = -1;
    uint_t capid = 0;
    bool is_pcie_cap = false;

    /* Search for PCIe extended capability */
    if (!disabled_msix && dev->pcie_cap != NULL) {
        capid = CAPID_MSIX;
        cap_index = cap_pcie_find_xtnd_capid(dev->pcie_cap, capid);
    }

    if (!disabled_msi && cap_index == -1 && dev->pcie_cap != NULL) {
        capid = CAPID_MSI;
        cap_index = cap_pcie_find_xtnd_capid(dev->pcie_cap, capid);
    }

    if (cap_index == -1) {
        /* Search for regular capability */
        if (!disabled_msix) {
            capid = CAPID_MSIX;
            cap_index = pci_device_find_capid(pci_bdf(dev->hdl), capid);
        }

        if (!disabled_msi && cap_index == -1) {
            capid = CAPID_MSI;
            cap_index = pci_device_find_capid(pci_bdf(dev->hdl), capid);
        }
    }
    else {
        is_pcie_cap = true;
    }

    if (cap_index == -1) {
        return PCI_ERR_ENOTSUP; // Not supported
    }

    if (is_pcie_cap) { /* Read PCIe extended capability */
        r = cap_pcie_read_xtnd_cap(
                dev->pcie_cap, &dev->msi_cap, cap_index );

        if (r != PCI_ERR_OK) {
            log_err("cap_pcie_read_xtnd_cap error; %s\n", pci_strerror(r));

            return r;
        }
    }
    else { /* Read regular capability */
        r = pci_device_read_cap(pci_bdf(dev->hdl), &dev->msi_cap, cap_index);

        if (r != PCI_ERR_OK) {
            log_err("pci_device_read_cap error; %s\n", pci_strerror(r));

            return r;
        }
    }

    log_info("read capability[%d]: 0x%02x\n", cap_index, capid);

    uint_t nvecs = 0;

    if (capid == CAPID_MSIX) {
        nvecs = cap_msix_get_nirq(dev->msi_cap);
    }
    else {
        nvecs = cap_msi_get_nirq(dev->msi_cap);
    }

    if (nvecs < 1) {
        log_err("error cap_msi_get_nirq returned %d\n", nvecs);

        return PCI_ERR_ENOTSUP; // Not supported
    }

    log_info("nirq: %d\n", nvecs);

    if (capid == CAPID_MSIX) {
        for (int i = 0; i < nvecs; ++i) {
            r = cap_msix_set_irq_entry(dev->hdl, dev->msi_cap, i, i);

            if (r != PCI_ERR_OK) {
                log_err("cap_msix_set_irq_entry error; %s\n", pci_strerror(r));

                return r;
            }
        }
    }
    else {
        r = cap_msi_set_nirq(dev->hdl, dev->msi_cap, nvecs);

        if (r != PCI_ERR_OK) {
            log_err("cap_msi_set_nirq error; %s\n", pci_strerror(r));

            return r;
        }
    }

    r = pci_device_cfg_cap_enable( dev->hdl,
            pci_reqType_e_MANDATORY, dev->msi_cap );

    if (r != PCI_ERR_OK) {
        log_err("pci_device_cfg_cap_enable error; %s\n", pci_strerror(r));

        return r;
    }

    if (capid == CAPID_MSI) {
        cap_msi_mask_t mask;

        r = cap_msi_get_irq_mask(dev->hdl, dev->msi_cap, &mask);

        dev->is_msi = true;

        if (r == PCI_ERR_ENOTSUP) {
            log_err("capability 0x%02x (MSI) Per Vector Masking (PVM) not "
                    "supported\n", capid);

            dev->is_msi = false;
        }
        else if (r != PCI_ERR_OK) {
            log_err("cap_msi_get_irq_mask error; %s\n", pci_strerror(r));

            msix_uninit(dev);
            return r;
        }
    }

    if (capid == CAPID_MSIX) {
        log_info("capability 0x%02x (MSI-X) enabled\n", capid);
        dev->is_msix = true;

        switch (cap_msix_unmask_irq_all(dev->hdl, dev->msi_cap)) {
            case PCI_ERR_EALREADY:
                log_info("Unmask IRQ (all): PCI_ERR_EALREADY\n");
                break;
            case PCI_ERR_OK:
                log_info("Unmask IRQ (all): PCI_ERR_OK\n");
                break;
            default:
                log_info("something else\n");
                break;
        };
    }
    else {
        log_info("capability 0x%02x (MSI) enabled\n", capid);
        dev->is_msix = false;
    }

    return PCI_ERR_OK;
}

static inline void pcie_uninit (struct pci_dev* dev) {
    pci_err_t r;

    if (dev->pcie_cap == NULL) {
        return;
    }

    log_info("disabling PCIe capability\n");

    if (!pci_device_cfg_cap_isenabled(dev->hdl, dev->pcie_cap)) {
        log_err("error capability not enabled\n");

        return;
    }

    r = pci_device_cfg_cap_disable( dev->hdl,
            pci_reqType_e_UNSPECIFIED, dev->pcie_cap );

    if (r != PCI_ERR_OK && r != PCI_ERR_ENOTSUP) {
        log_err("pci_device_cfg_cap_disable error; %s\n", pci_strerror(r));
    }

    if (r == PCI_ERR_ENOTSUP) {
        log_info("disabling PCIe capability not allowed\n");
    }

    free(dev->pcie_cap);
    dev->pcie_cap = NULL;
}

void msix_uninit (struct pci_dev* dev) {
    pci_err_t r;

    if (dev->msi_cap == NULL) {
        return;
    }

    if (dev->is_msix) {
        log_info("disabling MSI-X capability\n");
    }
    else {
        log_info("disabling MSI capability\n");
    }

    if (!pci_device_cfg_cap_isenabled(dev->hdl, dev->msi_cap)) {
        log_err("error capability not enabled\n");

        return;
    }

    r = pci_device_cfg_cap_disable( dev->hdl,
            pci_reqType_e_UNSPECIFIED, dev->msi_cap );

    if (r != PCI_ERR_OK) {
        log_err("pci_device_cfg_cap_disable error; %s\n", pci_strerror(r));
    }

    free(dev->msi_cap);
    dev->msi_cap = NULL;

    pcie_uninit(dev);
}
