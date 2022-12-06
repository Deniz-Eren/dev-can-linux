/*
 * \file    interrupt.c
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

#include "interrupt.h"

/*
 * TODO: This implementation only works because current scenarios tested only
 * involved a single IRQ assigned to the PCI device; this needs to be made more general.
 */

struct sigevent event;
int id = -1;

struct func_t {
    irq_handler_t handler;
    struct net_device *dev;
    unsigned int irq;
};

struct func_t funcs[16];
int funcs_size = 0;


const struct sigevent *irq_handler (void *area, int id) {
    return &event;
}

int request_irq (unsigned int irq, irq_handler_t handler, unsigned long flags,
        const char *name, void *dev)
{
    log_trace("request_irq; irq: %d, name: %s\n", irq, name);

    struct net_device *ndev = (struct net_device *)dev;

    struct func_t f = {
            .handler = handler,
            .dev = ndev,
            .irq = irq
    };

    funcs[funcs_size++] = f;

    if (id == -1) {
        SIGEV_SET_TYPE(&event, SIGEV_INTR);

        if ((id = InterruptAttach(irq, &irq_handler, NULL, 0, 0)) == -1) {
            log_err("internal error; interrupt attach failure\n");
        }
    }

    return 0;
}

void free_irq (unsigned int irq, void *dev) {
    log_trace("free_irq; irq: %d\n", irq);

    InterruptDetach(id);
}

void run_interrupt_wait() {
    while (1) {
        int i;
        InterruptWait( 0, NULL );

        for (i = 0; i < funcs_size; ++i) {
            funcs[i].handler(funcs[i].irq, funcs[i].dev);
        }
    }
}
