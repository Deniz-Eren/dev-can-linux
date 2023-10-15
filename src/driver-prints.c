/*
 * \file    driver-prints.c
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

#include "config.h"
#include "pci.h"
#include "driver-prints.h"


void print_driver_selection_results() {
    driver_selection_t** location = &driver_selection_root;

    while (*location != NULL) {
        log_info("Auto detected device (%x:%x) successfully: (driver \"%s\")\n",
                (*location)->pdev.vendor,
                (*location)->pdev.device,
                (*location)->driver->name);

        location = &(*location)->next;
    }
}

static void print_card (FILE* file, const struct pci_driver* driver) {
    fprintf(file, "  Driver: \e[1m%s\e[m\n", driver->name);
    fprintf(file, "  Supported devices (detailed):\n");

    if (driver->id_table != NULL) {
        const struct pci_device_id *id_table = driver->id_table;

        while (id_table->vendor != 0) {
            fprintf(file, "    { vendor: \e[1m%x\e[m, device: \e[1m%x\e[m, subvendor: %x, "
                                "subdevice: %x, class: %x, class_mask: %x }\n",
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

void print_support (int detailed) {
    print_notice();

    printf("\n");

    if (detailed) {
        printf("\e[1mAdvantech PCI cards:\e[m\n");
        print_card(stdout, &adv_pci_driver);

        printf("\e[1mKVASER PCAN PCI cards:\e[m\n");
        print_card(stdout, &kvaser_pci_driver);

        printf("\e[1mEMS CPC-PCI/PCIe/104P CAN cards:\e[m\n");
        print_card(stdout, &ems_pci_driver);

        printf("\e[1mPEAK PCAN PCI family cards:\e[m\n");
        print_card(stdout, &peak_pci_driver);

        printf("\e[1mPLX90xx PCI-bridge cards (with the SJA1000 chips):\e[m\n");
        print_card(stdout, &plx_pci_driver);

        return;
    }

    printf("Supports:\n");
    printf("  - Advantech PCI cards\n");
    printf("  - KVASER PCAN PCI cards\n");
    printf("  - EMS CPC-PCI/PCIe/104P CAN cards\n");
    printf("  - PEAK PCAN PCI family cards\n");
    printf("  - PLX90xx PCI-bridge cards (with the SJA1000 chips)\n");
    printf("\n");
    printf("For more details use option `\e[1m-ii\e[m'\n");
}

/**
 * Error print helpers
 */

void print_pci_device_attach_errors (pci_err_t r) {
    switch (r) {
    case PCI_ERR_OK: // No errors
        break;

    case PCI_ERR_EINVAL:
        log_err("pci device attach failed; Invalid flags.\n");
        break;
    case PCI_ERR_ENODEV:
        log_err("pci device attach failed; "
                "The bdf argument doesn't refer to a valid device.\n");
        break;
    case PCI_ERR_ATTACH_EXCLUSIVE:
        log_err("pci device attach failed; "
                "The device identified by bdf is already exclusively "
                "owned.\n");
        break;
    case PCI_ERR_ATTACH_SHARED:
        log_err("pci device attach failed; "
                "The request was for exclusive attachment, but the "
                "device identified by bdf has already been "
                "successfully attached to.\n");
        break;
    case PCI_ERR_ATTACH_OWNED:
        log_err("pci device attach failed; "
                "The request was for ownership of the device, but the "
                "device is already owned.\n");
        break;
    case PCI_ERR_ENOMEM:
        log_err("pci device attach failed; Memory for internal "
                "resources couldn't be obtained. This may be a "
                "temporary condition.\n");
        break;
    case PCI_ERR_LOCK_FAILURE:
        log_err("pci device attach failed; "
                "There was an error related to the creation, "
                "acquisition, or use of a synchronization object.\n");
        break;
    case PCI_ERR_ATTACH_LIMIT:
        log_err("pci device attach failed; There have been too many "
                "attachments to the device identified by bdf.\n");
        break;
    default:
        log_err("pci device attach failed; Unknown error: %d\n",
                (int)r);
        break;
    }
}

void print_pci_device_read_cap_errors (pci_err_t r) {
    switch (r) {
    case PCI_ERR_OK: // No errors
        break;

    case PCI_ERR_ENOENT:
        log_err("The capability at idx doesn't exist.\n");
        break;
    case PCI_ERR_EIO:
        log_err("A malformed capability “next” pointer was "
                "encountered.\n");
        break;
    case PCI_ERR_EINVAL:
        log_err("The cap argument was NULL.\n");
        break;
    case PCI_ERR_NO_MODULE:
        log_err("A capability module doesn't exist (in the "
                "capability module search path) for the "
                "capability at idx for the device identified "
                "by bdf.\n");
        break;
    case PCI_ERR_MODULE_BLACKLISTED:
        log_err("The identified capability module is contained "
                "in the PCI_MODULE_BLACKLIST environment "
                "variable.\n");
        break;
    case PCI_ERR_MODULE_SYM:
        log_err("The identified capability module doesn't have "
                "a proper initialization function.\n");
        break;
    case PCI_ERR_MOD_COMPAT:
        log_err("The identified capability module is "
                "incompatible with the version of either the "
                "pci-server binary or libpci.\n");
        break;
    case PCI_ERR_ENOMEM:
        log_err("Memory for internal resources couldn't be "
                "obtained. This may be a temporary "
                "condition.\n");
        break;
    case PCI_ERR_LOCK_FAILURE:
        log_err("An error occurred that's related to the "
                "creation, acquisition or use of a "
                "synchronization object.\n");
        break;
    default:
        log_err("pci device read capabilities failed; Unknown "
                "error: %d\n", (int)r);
        break;
    }
}

void print_pci_device_cfg_cap_enable_disable_errors (pci_err_t r) {
    switch (r) {
    case PCI_ERR_OK: // No errors
        break;

    case PCI_ERR_EALREADY:
        log_err("The capability identified by cap is already "
                "enabled/disabled.\n");
        break;
    case PCI_ERR_ENOTSUP:
        log_err("The capability can't be disabled.\n");
        break;
    case PCI_ERR_EINVAL:
        log_err("The hdl doesn't refer to a device that you "
                "attached to, or some other parameter is "
                "otherwise invalid.\n");
        break;
    case PCI_ERR_ENODEV:
        log_err("The device identified by hdl doesn't exist. "
                "Note that this error can also be returned if "
                "a device that supports live removal is "
                "removed after a successful call to "
                "pci_device_find().\n");
        break;
    case PCI_ERR_NOT_OWNER:
        log_err("You don't own the device associated with "
                "hdl.\n");
        break;
    case PCI_ERR_ENOMEM:
        log_err("Memory for internal resources couldn't be "
                "obtained. This may be a temporary "
                "condition.\n");
        break;
    case PCI_ERR_LOCK_FAILURE:
        log_err("An error occurred that's related to the "
                "creation, acquisition, or use of a "
                "synchronization object.\n");
        break;
    default:
        log_err("pci device capability enabling failed; Unknown "
                "error: %d\n", (int)r);
        break;
    }
}
