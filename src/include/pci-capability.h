/*
 * \file    pci-capability.h
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

#ifndef SRC_PCI_CAPABILITY_H_
#define SRC_PCI_CAPABILITY_H_

#include <pthread.h>
#include <linux/pci.h>


extern pci_err_t msix_init (struct pci_dev* dev);
extern void msix_uninit (struct pci_dev* dev);

#endif /* SRC_PCI_CAPABILITY_H_ */
