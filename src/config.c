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
