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
struct driver_selection_t {
    pci_vid_t vid;
    pci_did_t did;

    int driver_auto;
    int driver_pick;
    int driver_ignored;
    int driver_unsupported;
};

/* Supported CAN-bus PCI drivers */
extern struct pci_driver adv_pci_driver;
extern struct pci_driver kvaser_pci_driver;
extern struct pci_driver ems_pci_driver;
extern struct pci_driver peak_pci_driver;
extern struct pci_driver plx_pci_driver;

/* Detected PCI driver */
extern struct pci_driver *detected_driver;

extern int process_driver_selection (struct driver_selection_t* ds);
extern void print_driver_selection_results (struct driver_selection_t* ds);

#endif /* SRC_PCI_H_ */
