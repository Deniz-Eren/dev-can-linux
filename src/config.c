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

#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT == 1 && \
    CONFIG_QNX_INTERRUPT_ATTACH == 1
#error Cannot set (CONFIG_QNX_) *INTERRUPT_ATTACH_EVENT and *INTERRUPT_ATTACH
#endif

int optv = 0;
int optl = 0;
int optq = 0;
int optd = 0, opt_vid = -1, opt_did = -1;
