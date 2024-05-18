/*
 * \file    linux/module.h
 * \brief   Defines all MODULE_*() and EXPORT_*() macros with nothing.
 *
 * \details This file has the exact same name and path as the one supplied by
 *          the Linux Kernel source-code, however the contents have been
 *          completely replaced. This has been done to keep the rest of the
 *          Linux Kernel source-code unchanged so that ongoing updates to the
 *          Linux Kernel can be propagated to this project easily.
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

#ifndef _LINUX_MODULE_H
#define _LINUX_MODULE_H

/* Do nothing for the following macros */
#define MODULE_AUTHOR(str)
#define MODULE_DESCRIPTION(str)
#define MODULE_LICENSE(str)
#define MODULE_DEVICE_TABLE(pci, tbl)
#define module_pci_driver(pci_driver)
#define EXPORT_SYMBOL_GPL(func)
#define EXPORT_SYMBOL(func)
#define module_param(name, type, perm) // TODO: Support module_param
#define MODULE_PARM_DESC(_parm, desc)

#endif /* _LINUX_MODULE_H */
