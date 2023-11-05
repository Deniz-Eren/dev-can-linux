/*
 * \file    interrupt.h
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

#ifndef SRC_INTERRUPT_H_
#define SRC_INTERRUPT_H_

#include <pci/cap_msi.h>
#include <pci/cap_msix.h>

#include <linux/interrupt.h>

//#define MSI_DEBUG

/* shutdown check */
extern volatile unsigned shutdown_program; // 0x0 = running, 0x1 = shutdown

extern void* irq_loop (void*);
extern void terminate_irq_loop();


#ifdef MSI_DEBUG

static inline void mask_irq_debug (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap
        && pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap))
    {
        if (attach->is_msix) {
            pci_err_t err =
                cap_msix_mask_irq_entry(
                        attach->hdl,
                        attach->msi_cap,
                        attach->irq_entry );

            log_dbg("IRQ: %d MSI-X\n", attach->irq);

            switch (err) {
                case PCI_ERR_EALREADY:
                    log_info("IRQ: PCI_ERR_EALREADY\n");
                    break;
                case PCI_ERR_OK:
                    break;
                default:
                    log_err("cap_msix_mask_irq_entry error: %s\n",
                            pci_strerror(err));
                    break;
            };
        }
        else if (attach->is_msi) {
            pci_err_t err =
                cap_msi_mask_irq_entry(
                        attach->hdl,
                        attach->msi_cap,
                        attach->irq_entry );

            log_dbg("IRQ: %d MSI\n", attach->irq);

            switch (err) {
                case PCI_ERR_ENOTSUP:
                    log_info("IRQ: Per Vector Masking (PVM) not "
                             "supported\n");
                    break;
                case PCI_ERR_OK:
                    break;
                default:
                    log_err("cap_msi_mask_irq_entry error: %s\n",
                            pci_strerror(err));
                    break;
            };
        }
        else {
            log_dbg("IRQ: %d Legacy-MSI\n", attach->irq);

            InterruptMask(attach->irq, attach->id);
        }
    }
    else {
        log_dbg("IRQ: %d non-MSI\n", attach->irq);

        InterruptMask(attach->irq, attach->id);
    }
}

static inline void unmask_irq_debug (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap
        && pci_device_cfg_cap_isenabled(
                attach->hdl, attach->msi_cap ))
    {
        if (attach->is_msix) {
            pci_err_t err =
                cap_msix_unmask_irq_entry(
                        attach->hdl,
                        attach->msi_cap,
                        attach->irq_entry );

#if CONFIG_QNX_INTERRUPT_MASK_ISR == 1
            log_dbg("IRQ: %d MSI-X\n", attach->irq);
#endif

            switch (err) {
                case PCI_ERR_EALREADY:
                    log_info("IRQ: PCI_ERR_EALREADY\n");
                    break;
                case PCI_ERR_OK:
                    break;
                default:
                    log_err("cap_msix_unmask_irq_entry error: %s\n",
                            pci_strerror(err));
                    break;
            };
        }
        else if (attach->is_msi) {
            pci_err_t err =
                cap_msi_unmask_irq_entry(
                        attach->hdl,
                        attach->msi_cap,
                        attach->irq_entry );

#if CONFIG_QNX_INTERRUPT_MASK_ISR == 1
            log_dbg("IRQ: %d MSI\n", attach->irq);
#endif

            switch (err) {
                case PCI_ERR_ENOTSUP:
                    log_info("IRQ: Per Vector Masking (PVM) not "
                             "supported\n");
                    break;
                case PCI_ERR_OK:
                    break;
                default:
                    log_err("cap_msi_unmask_irq_entry error: %s\n",
                            pci_strerror(err));
                    break;
            };
        }
        else {
#if CONFIG_QNX_INTERRUPT_MASK_ISR == 1
            log_dbg("IRQ: %d Legacy-MSI\n", attach->irq);
#endif
            InterruptUnmask(attach->irq, attach->id);
        }
    }
    else {
#if CONFIG_QNX_INTERRUPT_MASK_ISR == 1
        log_dbg("IRQ: %d non-MSI\n", attach->irq);
#endif

        InterruptUnmask(attach->irq, attach->id);
    }
}

#endif /* MSI_DEBUG */

static inline void mask_irq_msix (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap == NULL) {
        return;
    }

    /*
     * This function (mask_irq_msix) is optimized assuming the user has already
     * setup the PCI capabilities and verified they are enabled. Thus we skip the
     * check:
     *
     *      pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap)
     *      AND attach->is_msix
     */

    pci_err_t err = cap_msix_mask_irq_entry(
                attach->hdl,
                attach->msi_cap,
                attach->irq_entry );

    if (err != PCI_ERR_EALREADY && err != PCI_ERR_OK) {
        log_err("cap_msix_mask_irq_entry error: %s\n", pci_strerror(err));
    }
}

static inline void unmask_irq_msix (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap == NULL) {
        return;
    }

    /*
     * This function (unmask_irq_msix) is optimized assuming the user has already
     * setup the PCI capabilities and verified they are enabled. Thus we skip the
     * check:
     *
     *      pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap)
     *      AND attach->is_msix
     */

    pci_err_t err =
        cap_msix_unmask_irq_entry(
                attach->hdl,
                attach->msi_cap,
                attach->irq_entry );

    if (err != PCI_ERR_EALREADY && err != PCI_ERR_OK) {
        log_err("cap_msix_unmask_irq_entry error: %s\n", pci_strerror(err));
    }
}

static inline void mask_irq_msi (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap == NULL) {
        return;
    }

    /*
     * This function (mask_irq_msi) is optimized assuming the user has already
     * setup the PCI capabilities and verified they are enabled. Thus we skip the
     * check:
     *
     *      pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap)
     *      AND !attach->is_msix
     *      AND attach->is_msi
     */

    pci_err_t err =
        cap_msi_mask_irq_entry(
                attach->hdl,
                attach->msi_cap,
                attach->irq_entry );

    if (err != PCI_ERR_OK) {
        log_err("cap_msi_mask_irq_entry error: %s\n", pci_strerror(err));
    }
}

static inline void unmask_irq_msi (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap == NULL) {
        return;
    }

    /*
     * This function (unmask_irq_msi) is optimized assuming the user has already
     * setup the PCI capabilities and verified they are enabled. Thus we skip the
     * check:
     *
     *      pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap)
     *      AND !attach->is_msix
     *      AND attach->is_msi
     */

    pci_err_t err =
        cap_msi_unmask_irq_entry(
                attach->hdl,
                attach->msi_cap,
                attach->irq_entry );

    if (err != PCI_ERR_OK) {
        log_err("cap_msi_unmask_irq_entry error: %s\n", pci_strerror(err));
    }
}

static inline void mask_irq_msi_legacy (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap == NULL) {
        return;
    }

    /*
     * This function (mask_irq_msi) is optimized assuming the user has already
     * setup the PCI capabilities and verified they are enabled. Thus we skip the
     * check:
     *
     *      pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap)
     *      AND !attach->is_msix
     *      AND !attach->is_msi
     */

    InterruptMask(attach->irq, attach->id);
}

static inline void unmask_irq_msi_legacy (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    if (attach->msi_cap == NULL) {
        return;
    }

    /*
     * This function (unmask_irq_msi) is optimized assuming the user has already
     * setup the PCI capabilities and verified they are enabled. Thus we skip the
     * check:
     *
     *      pci_device_cfg_cap_isenabled(attach->hdl, attach->msi_cap)
     *      AND !attach->is_msix
     *      AND !attach->is_msi
     */

    InterruptUnmask(attach->irq, attach->id);
}

static inline void mask_irq_regular (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    InterruptMask(attach->irq, attach->id);
}

static inline void unmask_irq_regular (uint_t attach_index) {
    if (attach_index >= irq_attach_size) {
        return;
    }

    irq_attach_t* attach = &irq_attach[attach_index];

    InterruptUnmask(attach->irq, attach->id);
}

#endif /* SRC_INTERRUPT_H_ */
