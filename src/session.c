/*
 * \file    session.c
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

#include "session.h"

device_sessions_t device_sessions[MAX_DEVICES];


int create_session (session_t* S, struct net_device* dev,
        const queue_attr_t* rx_attr,
        const queue_attr_t* tx_attr)
{
    int result;

    if (S == NULL) {
        return EFAULT; // Bad address
    }

    if ((result = create_queue(&S->rx, rx_attr)) != EOK) {
    }

    if ((result = create_queue(&S->tx, tx_attr)) != EOK) {
    }

    S->device = dev;

    return EOK;
}

void destroy_session (session_t* S) {
    if (S == NULL) {
        return;
    }

    destroy_queue(&S->rx);
    destroy_queue(&S->tx);
}
