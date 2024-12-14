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
#include <atomic.h>
#include <sys/netmgr.h>

#include <linux/netdevice.h>

#include "interrupt.h"
#include "session.h"


int irq_chid = -1;
struct sigevent terminate_event;

/* atomic shutdown check */
volatile unsigned shutdown_program = 0x0; // 0x0 = running, 0x1 = shutdown

void terminate_irq_loop () {
    atomic_set_value(&shutdown_program, 0x01);

    if (MsgDeliverEvent(0, &terminate_event) == -1) {
        log_err("irq_trigger_event error; %s\n", strerror(errno));
    }
}

/*
 * Interrupt Service Routine (ISR)
 *
 * The ISR returns a QNX signal event.
 */
const struct sigevent *irq_handler (void *area, int id) {
    if (area == NULL) {
        return NULL;
    }

    irq_attach_t* attach = (irq_attach_t*)area;

# if CONFIG_QNX_INTERRUPT_MASK_ISR == 1
    if (attach->mask == NULL) {
        return NULL;
    }

    int k = attach->event.sigev_code;

    if (k < 0 || k >= MAX_IRQ_ATTACH_COUNT) {
        return NULL;
    }

    log_enabled = false;

    attach->mask(k);

    log_enabled = true;
# endif

    return &attach->event;
}

int request_irq (unsigned int irq, irq_handler_t handler, unsigned long flags,
        const char *name, void *dev)
{
    return request_threaded_irq(irq, handler, NULL, flags, name, dev);
}

int request_threaded_irq(unsigned int irq, irq_handler_t handler,
        irq_handler_t thread_fn,
        unsigned long flags, const char *name, void *dev)
{
    log_trace("request_irq; irq: %d, name: %s\n", irq, name);

    unsigned attach_flags =
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

    struct net_device *ndev = (struct net_device *)dev;

    int err;
    int policy;
    struct sched_param param;

    err = pthread_getschedparam(pthread_self(), &policy, &param);

    if (err != EOK) {
        log_err("error pthread_getschedparam: %s\n", strerror(err));

        return -1;
    }

    if (irq_chid == -1) {
        unsigned flags = _NTO_CHF_PRIVATE;

        irq_chid = ChannelCreate(flags);
        int coid =
            ConnectAttach(ND_LOCAL_NODE, 0, irq_chid, _NTO_SIDE_CHANNEL, 0);

        SIGEV_SET_TYPE(&terminate_event, SIGEV_PULSE);
        terminate_event.sigev_coid = coid;
        terminate_event.sigev_code = 0; // not needed for termination
        terminate_event.sigev_priority =
            param.sched_priority + CONFIG_IRQ_SCHED_PRIORITY_BOOST;
    }

    irq_group_t* group = irq_to_group_map[irq];

    uint_t i;
    for (i = 0; i < group->num_irq; ++i) {
        if (irq_attach_size == MAX_IRQ_ATTACH_COUNT) {
            log_err( "error request_irq reached max IRQ attach count: %d\n",
                    (int)irq_attach_size );

            return -1;
        }

        bool create_new_attach = true;

        uint_t j;
        for (j = 0; j < irq_attach_size; ++j) {
            if (irq_attach[j].irq == group->irq[i]) {
                int h = irq_attach[j].num_handlers++;

                if (irq_attach[j].num_handlers >= MAX_HANDLERS_PER_IRQ) {
                    log_err( "error reached max handlers per IRQ count: %d\n",
                            MAX_HANDLERS_PER_IRQ );

                    return -1;
                }

                irq_attach[j].handler[h] = handler;
                irq_attach[j].reset_interrupt[h] = thread_fn;
                irq_attach[j].dev[h] = ndev;

                create_new_attach = false;
                break;
            }
        }

        if (!create_new_attach) {
            continue;
        }

        int k = irq_attach_size++;

        SIGEV_SET_TYPE(&irq_attach[k].event, SIGEV_PULSE);

        int coid =
            ConnectAttach(ND_LOCAL_NODE, 0, irq_chid, _NTO_SIDE_CHANNEL, 0);

        irq_attach[k].event.sigev_coid = coid;
        irq_attach[k].event.sigev_code = k;
        irq_attach[k].event.sigev_priority =
            param.sched_priority + CONFIG_IRQ_SCHED_PRIORITY_BOOST;

        irq_attach[k].irq = group->irq[i];
        irq_attach[k].irq_entry = i;

        /* Guaranteed to be the first handler installation */
        irq_attach[k].handler[0] = handler;
        irq_attach[k].reset_interrupt[0] = thread_fn;
        irq_attach[k].dev[0] = ndev;
        irq_attach[k].num_handlers = 1;

        irq_attach[k].hdl = group->hdl;
        irq_attach[k].msi_cap = group->msi_cap;
        irq_attach[k].is_msi = group->is_msi;
        irq_attach[k].is_msix = group->is_msix;
        irq_attach[k].mask = NULL;
        irq_attach[k].unmask = NULL;

#ifdef MSI_DEBUG
        irq_attach[k].mask = mask_irq_debug;
        irq_attach[k].unmask = unmask_irq_debug;
#else
        if (group->msi_cap
            && pci_device_cfg_cap_isenabled(group->hdl, group->msi_cap))
        {
            if (group->is_msix) { // MSI-X Support
                irq_attach[k].mask = mask_irq_msix;
                irq_attach[k].unmask = unmask_irq_msix;

                log_trace("attached MSI-X IRQ %d\n", group->irq[i]);
            }
            else if (group->is_msi) { // MSI Support (with PVM)
                irq_attach[k].mask = mask_irq_msi;
                irq_attach[k].unmask = unmask_irq_msi;

                log_trace("attached MSI IRQ %d\n", group->irq[i]);
            }
            else { // MSI Support (without PVM)
                irq_attach[k].mask = mask_irq_msi_legacy;
                irq_attach[k].unmask = unmask_irq_msi_legacy;

                log_trace("attached Legacy-MSI IRQ %d\n", group->irq[i]);
            }
        }
        else { // Regular IRQ
            irq_attach[k].mask = mask_irq_regular;
            irq_attach[k].unmask = unmask_irq_regular;

            log_trace("attached regular IRQ %d\n", group->irq[i]);
        }
#endif

        int id;

        if (irq_attach[k].mask == mask_irq_msi_legacy ||
            irq_attach[k].mask == mask_irq_regular)
        {
            // When working with regular IRQ or legacy MSI devices that
            // are handled at the regular IRQ level, we use InterruptAttachEvent
            // to minimize Kernel called ISR code.
            //
            // Additionally, InterruptAttachEvent will already mask the regular
            // IRQ.

            if ((id = InterruptAttachEvent( group->irq[i],
                        &irq_attach[k].event, attach_flags )) == -1)
            {
                log_err("internal error; interrupt attach event failure\n");

                return -1;
            }
        }
        else {
            // When working with MSI supported (with PVM) or MSI-X supported
            // devices, we want to use InterruptAttach. This is because
            // InterruptAttachEvent would mask the regular IRQ assoicated with
            // the device. We definitely want to avoid doing that and instead we
            // want to benefit from the MSI/MSI-X PVM support.
            //
            // As such, in this case we cannot avoid implementing an ISR, which
            // the Kernel will call.

            if ((id = InterruptAttach( group->irq[i],
                        &irq_handler, &irq_attach[k], 0, attach_flags )) == -1)
            {
                log_err("internal error; interrupt attach failure\n");

                return -1;
            }
        }

        irq_attach[k].id = id;

        irq_attach[k].unmask(k);
    }

    return 0;
}

void free_irq (unsigned int irq, void *dev) {
    log_trace("free_irq; irq: %d\n", irq);

    uint_t i, k;
    for (i = 0; i < irq_to_group_map[irq]->num_irq; ++i) {
        for (k = 0; k < irq_attach_size; ++k) {
            if (irq_attach[k].id != -1 &&
                irq_to_group_map[irq]->irq[i] == irq_attach[k].irq &&
                irq_attach[k].dev == dev)
            {
                InterruptDetach(irq_attach[k].id);

                irq_attach[k].id = -1;
                break;
            }
        }
    }
}

void* irq_loop (void* arg) {
    int rcvid;
    irqreturn_t err;

    for(;;) {
        size_t i;
        struct _pulse pulse;
        rcvid = MsgReceivePulse(irq_chid, &pulse, sizeof(pulse), NULL);

        if (shutdown_program) {
            log_info("Shutdown IRQ loop\n");

            return NULL;
        }

        if (rcvid == -1 ) {
            log_err("MsgReceivePulse error; %s\n", strerror(errno));

            continue;
        }

        int_t k = pulse.code;

        if (k < 0) {
            continue;
        }

        irq_attach_t* attach = &irq_attach[k];

        if (attach->mask == NULL || attach->unmask == NULL) {
            continue;
        }

        if (irq_attach[k].mask != mask_irq_msi_legacy &&
            irq_attach[k].mask != mask_irq_regular)
        {
#if CONFIG_QNX_INTERRUPT_MASK_PULSE == 1
            attach->mask(k);
#endif
        }

        // Handle IRQ
        for (i = 0; i < irq_attach[k].num_handlers; ++i) {
            err = irq_attach[k].handler[i](
                    irq_attach[k].irq, irq_attach[k].dev[i] );

            if (err == IRQ_WAKE_THREAD
                && irq_attach[k].reset_interrupt[i] != NULL)
            {
                err = irq_attach[k].reset_interrupt[i](
                        irq_attach[k].irq, irq_attach[k].dev[i] );

                if (err != IRQ_HANDLED) {
                    log_err("reset_interrupt error: %d\n", err);
                }
            }
            else if (err != IRQ_NONE && err != IRQ_HANDLED) {
                log_err("IRQ handler error: %d\n", err);

                continue;
            }
        }

        attach->unmask(k);

        for (i = 0; i < irq_attach[k].num_handlers; ++i) {
            device_session_t* ds = irq_attach[k].dev[i]->device_session;
            if (queue_wake_pending(&ds->tx_queue)) {
                queue_start_signal(&ds->tx_queue);
                queue_awake(&ds->tx_queue);
            }
        }
    }
}
