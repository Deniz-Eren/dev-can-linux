/*
 * \file    pci.h
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

#ifndef SRC_PCI_H_
#define SRC_PCI_H_

#include <linux/pci.h>


/* Structure used for driver selection processing */
typedef struct driver_selection {
    struct driver_selection *prev, *next;

    pci_vid_t vid;
    pci_did_t did;

    /* Detected PCI driver */
    struct pci_driver* driver;
} driver_selection_t;

/* Supported CAN-bus PCI drivers */
extern struct pci_driver adv_pci_driver;
extern struct pci_driver kvaser_pci_driver;
extern struct pci_driver ems_pci_driver;
extern struct pci_driver peak_pci_driver;
extern struct pci_driver plx_pci_driver;

extern int process_driver_selection();
extern void print_driver_selection_results();

extern driver_selection_t* driver_selection_root;
extern driver_selection_t* probe_driver_selection;
extern int device_id_count;


static inline void store_driver_selection (
        pci_vid_t vid, pci_did_t did, struct pci_driver* driver)
{
    driver_selection_t* new_driver_selection;
    new_driver_selection = malloc(sizeof(driver_selection_t));

    new_driver_selection->vid = vid;
    new_driver_selection->did = did;
    new_driver_selection->driver = driver;

    if (driver_selection_root == NULL) {
        driver_selection_root = new_driver_selection;
        new_driver_selection->prev = new_driver_selection->next = NULL;
        return;
    }

    driver_selection_t** last = &driver_selection_root;

    while ((*last)->next != NULL) {
        last = &(*last)->next;
    }

    (*last)->next = new_driver_selection;
    new_driver_selection->prev = (*last);
    new_driver_selection->next = NULL;
}

static inline int probe_all_driver_selections() {
    static struct pci_dev pdev = {
            .ba = NULL,
            .vendor = 0,
            .device = 0,
            .dev = { .driver_data = NULL },
            .irq = 0
    };

    driver_selection_t* location = driver_selection_root;

    while (location != NULL) {
        driver_selection_t** next = &location->next;

        pdev.vendor = location->vid;
        pdev.device = location->did;

        probe_driver_selection = location;

        if (location->driver->probe(&pdev, NULL)) {
            return -1;
        }

        probe_driver_selection = NULL;

        location = *next;
    }

    return 0;
}

static inline void remove_all_driver_selections() {
    driver_selection_t** location = &driver_selection_root;

    static struct pci_dev pdev = {
            .ba = NULL,
            .vendor = 0,
            .device = 0,
            .dev = { .driver_data = NULL },
            .irq = 0
    };

    while (*location != NULL) {
        driver_selection_t** next = &(*location)->next;

        pdev.vendor = (*location)->vid;
        pdev.device = (*location)->did;

        log_info("Shutting down %s\n",
                (*location)->driver->name);

        (*location)->driver->remove(&pdev);
        free(*location);

        *location = *next;
    }
}

#endif /* SRC_PCI_H_ */
