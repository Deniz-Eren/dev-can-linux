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
    driver_selection_t* location = driver_selection_root;

    while (location != NULL) {
        log_info("Auto detected device (%04x:%04x) successfully: (driver \"%s\")\n",
                location->pdev.vendor,
                location->pdev.device,
                location->driver->name);

        location = location->next;
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
