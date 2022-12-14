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

#include <unistd.h>

#include "interrupt.h"

/*
 * TODO: This implementation only works because current scenarios tested only
 * involved a single IRQ assigned to the PCI device; this needs to be made more
 * general.
 */

int id = -1;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1
struct sigevent event;
#endif

struct func_t {
    irq_handler_t handler;
    struct net_device *dev;
    unsigned int irq;
};

struct func_t funcs[16];
int funcs_size = 0;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1
/*
 * Interrupt Service Routine (ISR)
 *
 * The actual ISR actually does no work except returning a QNX signal event.
 */
const struct sigevent *irq_handler (void *area, int id) {
    log_enabled = false;

    int i;
    for (i = 0; i < funcs_size; ++i) {
        funcs[i].handler(funcs[i].irq, funcs[i].dev);
    }

    log_enabled = true;
    return NULL;
}
#endif

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
        unsigned flags =
            /*
             * Put the new event at the end of the list of existing events
             * instead of the start.
             */
              _NTO_INTR_FLAGS_END
            /*
             * Associate the interrupt event with the process instead of the
             * attaching thread. The interrupt event is removed when the process
             * exits, instead of when the attaching thread exits.
             */
            | _NTO_INTR_FLAGS_PROCESS
            /*
             * Track calls to InterruptMask() and InterruptUnmask() to make
             * detaching the interrupt handler safer.
             */
            | _NTO_INTR_FLAGS_TRK_MSK;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1
        SIGEV_SET_TYPE(&event, SIGEV_INTR);

        if ((id = InterruptAttachEvent(irq, &event, flags)) == -1) {
            log_err("internal error; interrupt attach event failure\n");
        }
#else
        if ((id = InterruptAttach(irq, &irq_handler, NULL, 0, flags)) == -1) {
            log_err("internal error; interrupt attach failure\n");
        }
#endif
    }

    return 0;
}

void free_irq (unsigned int irq, void *dev) {
    log_trace("free_irq; irq: %d\n", irq);

    InterruptDetach(id);
}

void run_wait() {
    while (1) {
#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1
        int i;
        InterruptWait(0, NULL);

        if (funcs_size) {
            InterruptUnmask(funcs[0].irq, id);
        }

        for (i = 0; i < funcs_size; ++i) {
            funcs[i].handler(funcs[i].irq, funcs[i].dev);
        }
#else
        sleep(60);
#endif
    }
}
