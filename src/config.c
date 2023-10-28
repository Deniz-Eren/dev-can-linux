/*
 * \file    config.c
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

#include <config.h>

/*
 * Check configuration macros are valid
 */
#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH == 1
#error Cannot set (CONFIG_QNX_) *INTERRUPT_ATTACH_EVENT and *INTERRUPT_ATTACH
#endif

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 0 && \
    CONFIG_QNX_INTERRUPT_ATTACH == 0
#error Must set (CONFIG_QNX_) *INTERRUPT_ATTACH_EVENT or *INTERRUPT_ATTACH
#endif

#if CONFIG_QNX_RESMGR_SINGLE_THREAD == 1 && \
    CONFIG_QNX_RESMGR_THREAD_POOL == 1
#error Cannot set (CONFIG_QNX_RESMGR_) *SINGLE_THREAD and *THREAD_POOL
#endif

#if CONFIG_QNX_RESMGR_SINGLE_THREAD == 0 && \
    CONFIG_QNX_RESMGR_THREAD_POOL == 0
#error Must set (CONFIG_QNX_RESMGR_) *SINGLE_THREAD or *THREAD_POOL
#endif

/*
 * Program options, initial values
 *
 * See print_help() or dev-can-linux -h for details
 */
int optb = 0;
int optb_restart_ms = DEFAULT_RESTART_MS;
int optv = 0;
int optl = 0;
int opti = 0;
int optq = 0;
int optd = 0;
int opte = 0;
int opts = 0;
int optt = 0;
int optu = 0;
int optx = 0;

size_t num_optu_configs = 0;
channel_config_t* optu_config = NULL;

size_t num_disable_device_configs;
disable_device_config_t* disable_device_config;

/*
 * IRQ Management
 */

irq_group_t*    irq_group = NULL;
size_t          irq_group_size = 0;
irq_group_t**   irq_to_group_map = NULL;
size_t          irq_to_group_map_size = 0;
irq_attach_t    irq_attach[MAX_IRQ_ATTACH_COUNT];
size_t          irq_attach_size = 0;

static inline void irq_to_group_add (
        pci_irq_t* irq, size_t nirq, size_t group_id)
{
    uint_t i;

    size_t new_irq_to_group_map_size = 0;

    for (i = 0; i < nirq; ++i) {
        if (new_irq_to_group_map_size < irq[i]+1) {
            new_irq_to_group_map_size = irq[i]+1;
        }
    }

    if (irq_to_group_map_size < new_irq_to_group_map_size) {
        irq_group_t** new_irq_to_group_map =
            malloc(new_irq_to_group_map_size*sizeof(irq_group_t*));

        memset( new_irq_to_group_map, 0,
                new_irq_to_group_map_size*sizeof(irq_group_t*) );

        for (i = 0; i < irq_to_group_map_size; ++i) {
            new_irq_to_group_map[i] = irq_to_group_map[i];
        }

        if (irq_to_group_map) {
            free(irq_to_group_map);
        }

        irq_to_group_map = new_irq_to_group_map;
        irq_to_group_map_size = new_irq_to_group_map_size;
    }

    for (i = 0; i < nirq; ++i) {
        irq_to_group_map[irq[i]] = &irq_group[group_id];
    }
}

void irq_group_add (pci_irq_t* irq, size_t nirq,
        pci_devhdl_t hdl, pci_cap_t msi_cap, bool is_msix)
{
    uint_t i;

    size_t new_irq_group_size = irq_group_size + 1;

    irq_group_t* new_irq_group = malloc(new_irq_group_size*sizeof(irq_group_t));

    for (i = 0; i < irq_group_size; ++i) {
        new_irq_group[i] = irq_group[i];
    }

    new_irq_group[irq_group_size].num_irq = nirq;
    new_irq_group[irq_group_size].irq = malloc(nirq*sizeof(pci_irq_t));
    new_irq_group[irq_group_size].hdl = hdl;
    new_irq_group[irq_group_size].msi_cap = msi_cap;
    new_irq_group[irq_group_size].is_msix = is_msix;

    for (i = 0; i < nirq; ++i) {
        new_irq_group[irq_group_size].irq[i] = irq[i];
    }

    /* Redirect irq_to_group_map pointers */
    for (i = 0; i < irq_to_group_map_size; ++i) {
        if (irq_to_group_map[i]) {
            for (uint_t j = 0; j < irq_group_size; ++j) {
                if (irq_to_group_map[i] == &irq_group[j]) {
                    irq_to_group_map[i] = &new_irq_group[j];
                    break;
                }
            }
        }
    }

    if (irq_group) {
        free(irq_group);
    }

    irq_group = new_irq_group;
    irq_group_size = new_irq_group_size;

    irq_to_group_add(irq, nirq, irq_group_size-1);
}

void irq_group_cleanup (void) {
    uint_t i;

    if (irq_to_group_map) {
        free(irq_to_group_map);

        irq_to_group_map = NULL;
    }

    if (irq_group) {
        for (i = 0; i < irq_group_size; ++i) {
            free(irq_group[i].irq);

            irq_group[i].irq = NULL;
        }

        free(irq_group);

        irq_group = NULL;
    }
}
