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

#include <pthread.h>
#include <linux/pci.h>


/* Structure used for driver selection processing */
typedef struct driver_selection {
    struct driver_selection *prev, *next;
    struct pci_dev pdev;

    /* Detected PCI driver */
    struct pci_driver* driver;
} driver_selection_t;

/* Supported CAN-bus PCI drivers */
extern struct pci_driver adv_pci_driver;
extern struct pci_driver kvaser_pci_driver;
extern struct pci_driver ems_pci_driver;
extern struct pci_driver peak_pci_driver;
extern struct pci_driver plx_pci_driver;
extern struct pci_driver f81601_pci_driver;
extern struct pci_driver vcan_driver;

extern int process_driver_selection();
extern void print_driver_selection_results();

extern driver_selection_t* driver_selection_root;
extern driver_selection_t* probe_driver_selection;
extern int next_device_id;


static inline void store_driver_selection (
        pci_vid_t vid, pci_did_t did, struct pci_driver* driver)
{
    driver_selection_t* new_driver_selection;
    new_driver_selection = malloc(sizeof(driver_selection_t));

    new_driver_selection->pdev.vendor = vid;
    new_driver_selection->pdev.device = did;
    new_driver_selection->driver = driver;

    if (driver_selection_root == NULL) {
        driver_selection_root = new_driver_selection;
        new_driver_selection->prev = new_driver_selection->next = NULL;
        return;
    }

    driver_selection_t* last = driver_selection_root;

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_driver_selection;
    new_driver_selection->prev = last;
    new_driver_selection->next = NULL;
}

static inline int probe_all_driver_selections() {
    bool any_probed = false;

    driver_selection_t* location = driver_selection_root;

    while (location != NULL) {
        driver_selection_t* next = location->next;

        probe_driver_selection = location;

        if (location->driver->probe(&location->pdev, NULL)) {
            return -1;
        }

        any_probed = true;

        probe_driver_selection = NULL;

        location = next;
    }

    if (!any_probed) {
        return -1;
    }

    return 0;
}

/*
 * Helper structures and functions
 */

typedef struct bar { // Needed to track bars for ioremap()
    struct bar *next, *prev;
    pci_ba_t ba;
} bar_t;

typedef struct ioblock {
    struct ioblock *prev, *next;

    void __iomem* addr;
    size_t size;

    pci_ba_t ba; // The block is part of this bar, not necessarily the entire
                 // bar is dedicated to this block.

    bool is_managed;
} ioblock_t;

extern bar_t* bar_list_root;
extern ioblock_t* ioblock_root;
extern pthread_mutex_t ioblock_mutex;

static inline void store_bar (pci_ba_t ba) {
    bar_t* new_bar = malloc(sizeof(bar_t));
    new_bar->ba = ba;

    if (bar_list_root == NULL) {
        bar_list_root = new_bar;
        new_bar->prev = new_bar->next = NULL;
        return;
    }

    bar_t* last = bar_list_root;

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_bar;
    new_bar->prev = last;
    new_bar->next = NULL;
}

static inline bar_t* get_bar (uintptr_t addr) {
    bar_t* location = bar_list_root;

    while (location != NULL) {
        if (addr >= location->ba.addr &&
            addr < location->ba.addr + location->ba.size)
        {
            return location;
        }

        location = location->next;
    }

    return NULL;
}

static inline void store_block (void __iomem* addr, size_t size, pci_ba_t ba) {
    pthread_mutex_lock(&ioblock_mutex);

    ioblock_t* new_block = malloc(sizeof(ioblock_t));
    new_block->addr = addr;
    new_block->size = size;
    new_block->ba = ba;

    if (ioblock_root == NULL) {
        ioblock_root = new_block;
        ioblock_root->prev = ioblock_root->next = NULL;

        pthread_mutex_unlock(&ioblock_mutex);
        return;
    }

    ioblock_t* last = ioblock_root;

    while (last->next != NULL) {
        last = last->next;
    }

    last->next = new_block;
    new_block->prev = last;
    new_block->next = NULL;
    pthread_mutex_unlock(&ioblock_mutex);
}

static inline ioblock_t* get_block (void __iomem* addr) {
    pthread_mutex_lock(&ioblock_mutex);
    ioblock_t* location = ioblock_root;

    while (location != NULL) {
        if (location->addr == addr) {
            pthread_mutex_unlock(&ioblock_mutex);
            return location;
        }

        location = location->next;
    }

    pthread_mutex_unlock(&ioblock_mutex);
    return NULL;
}

static inline ioblock_t* get_managed_block (pci_ba_t* ba) {
    pthread_mutex_lock(&ioblock_mutex);
    ioblock_t* location = ioblock_root;

    while (location != NULL) {
        if (location->ba.addr == ba->addr && location->is_managed) {
            pthread_mutex_unlock(&ioblock_mutex);
            return location;
        }

        location = location->next;
    }

    pthread_mutex_unlock(&ioblock_mutex);
    return NULL;
}

static inline ioblock_t* remove_block (void __iomem* addr) {
    pthread_mutex_lock(&ioblock_mutex);
    ioblock_t* location = ioblock_root;

    while (location != NULL) {
        if (location->addr == addr) {
            ioblock_t* result = location;

            if (result->prev == NULL) {
                ioblock_root = result->next;

                if (ioblock_root != NULL) {
                    ioblock_root->prev = NULL;
                }
            }
            else {
                result->prev->next = result->next;

                if (result->next != NULL) {
                    result->next->prev = result->prev;
                }
            }

            if (result != NULL) {
                result->prev = NULL;
                result->next = NULL;
            }

            pthread_mutex_unlock(&ioblock_mutex);
            return result;
        }

        location = location->next;
    }

    pthread_mutex_unlock(&ioblock_mutex);
    return NULL;
}

static inline void remove_all_driver_selections() {
    while (driver_selection_root != NULL) {
        driver_selection_t* next = driver_selection_root->next;

        log_info("Shutting down %s\n", driver_selection_root->driver->name);

        struct pci_dev *pdev = &driver_selection_root->pdev;

        if (pdev != NULL) {
            driver_selection_root->driver->remove(pdev);

            if (pdev->vendor && pdev->device) {
                // Only Virtual CAN (vcan) driver can have vendor=0 or device=0

                for (int_t i = 0; i < pdev->nba; ++i) {
                    ioblock_t* block = NULL;

                    do {
                        block = get_managed_block(&pdev->ba[i]);

                        if (block) {
                            log_info( "Managed pci_iounmap for %s\n",
                                driver_selection_root->driver->name );

                            pci_iounmap(pdev, block->addr);
                        }
                    } while (block);
                }

                if (pdev->is_managed) {
                    log_info( "Managed pci_disable_device for %s\n",
                        driver_selection_root->driver->name );

                    pci_disable_device(pdev);
                }
            }
        }

        free(driver_selection_root);

        driver_selection_root = next;
    }
}

#endif /* SRC_PCI_H_ */
