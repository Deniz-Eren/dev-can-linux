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


#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 || \
    CONFIG_QNX_INTERRUPT_ATTACH == 1
int irq_chid = -1;
struct sigevent terminate_event;
#endif

/* atomic shutdown check */
volatile unsigned shutdown_program = 0x0; // 0x0 = running, 0x1 = shutdown

void terminate_irq_loop () {
    atomic_set_value(&shutdown_program, 0x01);

#if CONFIG_QNX_INTERRUPT_ATTACH == 1 || \
    CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1

    if (MsgDeliverEvent(0, &terminate_event) == -1) {
        log_err("irq_trigger_event error; %s\n", strerror(errno));
    }
#else
    sleep(2);
#endif
}

#if CONFIG_QNX_INTERRUPT_ATTACH == 1
/*
 * Interrupt Service Routine (ISR)
 *
 * The actual ISR actually does no work except returning a QNX signal event.
 */
const struct sigevent *irq_handler (void *area, int id) {
    if (area == NULL) {
        return NULL;
    }

    irq_attach_t* attach = (irq_attach_t*)area;

    return &attach->event;
}
#elif CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1
/*
 * Interrupt Service Routine (ISR)
 *
 * The actual ISR performs the work.
 */
const struct sigevent *irq_handler (void *area, int id) {
    log_enabled = false;

    // TODO: If raw ISR is ever needed it needs to be implemented here

    log_enabled = true;
    return NULL;
}
#endif

int request_irq (unsigned int irq, irq_handler_t handler, unsigned long flags,
        const char *name, void *dev)
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

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 || \
    CONFIG_QNX_INTERRUPT_ATTACH == 1
    if (irq_chid == -1) {
        unsigned flags = _NTO_CHF_PRIVATE;

        irq_chid = ChannelCreate(flags);
        int coid =
            ConnectAttach(ND_LOCAL_NODE, 0, irq_chid, _NTO_SIDE_CHANNEL, 0);

        SIGEV_SET_TYPE(&terminate_event, SIGEV_PULSE);
        terminate_event.sigev_coid = coid;
        terminate_event.sigev_code = 0; // not needed for termination
        terminate_event.sigev_priority =
            param.sched_priority + IRQ_SCHED_PRIORITY_BOOST;
    }
#endif

    irq_group_t* group = irq_to_group_map[irq];

    uint_t i;
    for (i = 0; i < group->num_irq; ++i) {
        if (irq_attach_size == MAX_IRQ_ATTACH_COUNT) {
            log_err( "error request_irq reached max IRQ attach count: %d\n",
                    (int)irq_attach_size );

            return -1;
        }

        int k = irq_attach_size++;

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 || \
    CONFIG_QNX_INTERRUPT_ATTACH == 1
        SIGEV_SET_TYPE(&irq_attach[k].event, SIGEV_PULSE);
#endif

        int coid =
            ConnectAttach(ND_LOCAL_NODE, 0, irq_chid, _NTO_SIDE_CHANNEL, 0);

        irq_attach[k].event.sigev_coid = coid;
        irq_attach[k].event.sigev_code = k;
        irq_attach[k].event.sigev_priority =
            param.sched_priority + IRQ_SCHED_PRIORITY_BOOST;

        irq_attach[k].irq = group->irq[i];
        irq_attach[k].irq_entry = i;
        irq_attach[k].handler = handler;
        irq_attach[k].dev = ndev;
        irq_attach[k].hdl = group->hdl;
        irq_attach[k].msi_cap = group->msi_cap;
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
            else { // MSI Support
                irq_attach[k].mask = mask_irq_msi;
                irq_attach[k].unmask = unmask_irq_msi;

                log_trace("attached MSI IRQ %d\n", group->irq[i]);
            }
        }
        else { // Regular IRQ
            irq_attach[k].mask = mask_irq_regular;
            irq_attach[k].unmask = unmask_irq_regular;

            log_trace("attached regular IRQ %d\n", group->irq[i]);
        }
#endif

        int id;
#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1
        if ((id = InterruptAttachEvent( group->irq[i],
                        &irq_attach[k].event, attach_flags )) == -1)
        {
            log_err("internal error; interrupt attach event failure\n");

            return -1;
        }
#else
        if ((id = InterruptAttach( group->irq[i],
                        &irq_handler, &irq_attach[k], 0, attach_flags )) == -1)
        {
            log_err("internal error; interrupt attach failure\n");

            return -1;
        }
#endif
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
#if CONFIG_QNX_INTERRUPT_ATTACH == 1 || \
    CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1

    int rcvid;

    for(;;) {
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

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1
        /*
         * Case when InterruptAttachEvent() is used:
         *
         * To prevent infinite interrupt recursion, the kernel automatically does
         * an InterruptMask() for intr when delivering the event. After the
         * interrupt-handling thread has dealt with the event, it must call
         * InterruptUnmask() to reenable the interrupt.
         */

        InterruptUnmask(attach->irq, attach->id);
#endif

        if (attach->mask == NULL || attach->unmask == NULL) {
            continue;
        }

        attach->mask(k);

        // Handle IRQ
        irq_attach[k].handler(irq_attach[k].irq, irq_attach[k].dev);

        attach->unmask(k);
    }
#else /* CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1 &&
         CONFIG_QNX_INTERRUPT_ATTACH != 1 */
    while (1) {

        if (shutdown_program) {
            return NULL;
        }

        /* Just spend some time doing nothing so the loop doesn't hard-lock */
        sleep(1);
    }
#endif
}
