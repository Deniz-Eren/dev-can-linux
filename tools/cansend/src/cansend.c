/*
 * \file    cansend.c
 * \brief   This program writes CAN messages to dev-can-* driver devices.
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

#include <dev-can-linux/commands.h>


int main (int argc, char* argv[]) {
    int opt;
    char* buffer;

    int optu_unit;
    char optu_mailbox_str[32];
    int optu_mailbox_is_tx = 0;
    int optu_mailbox = 0;

    int optw_mid = 0;
    int optw_mid_type = 0;
    char optw_data_str[32];

    while ((opt = getopt(argc, argv, "u:w:?h")) != -1) {
        switch (opt) {
        case 'u':
            buffer = optu_mailbox_str;
            sscanf(optarg, "%d,%s", &optu_unit, buffer);
            buffer[2] = 0;

            if (strncmp(buffer, "tx", 2) == 0) {
                optu_mailbox_is_tx = 1;
            }
            else if (strncmp(buffer, "rx", 2) == 0) {
                optu_mailbox_is_tx = 0;
            }
            else {
                exit(EXIT_FAILURE);
            }

            sscanf(optu_mailbox_str+2, "%d", &optu_mailbox);
            break;

        case 'w':
            buffer = optw_data_str;
            sscanf(optarg, "0x%x,%d,0x%s", &optw_mid, &optw_mid_type, buffer);

            break;

        case '?':
        case 'h':
            return EXIT_SUCCESS;

        default:
            printf("invalid option %c\n", opt);
            break;
        }
    }

    int     fd;
    struct  can_msg canmsg;

    int n = strnlen(optw_data_str, 32);
    n = n/2 + n%2;

    int i;
    for (i = 0; i < n; ++i) {
        unsigned int k;

        sscanf(optw_data_str + 2*i, "%2x", &k);

        canmsg.dat[i] = k;
    }

    for (i = n; i < 8; ++i) {
        canmsg.dat[i] = 0;
    }

    canmsg.mid = optw_mid;
    canmsg.ext.timestamp = 0;
    canmsg.ext.is_extended_mid = optw_mid_type;
    canmsg.len = n;

    char OPEN_FILE[16];

    snprintf( OPEN_FILE, 16, "/dev/can%d/%s%d",
            optu_unit,
            (optu_mailbox_is_tx ? "tx" : "rx"), optu_mailbox );

    if ((fd = open(OPEN_FILE, O_RDWR)) == -1) {
        exit(EXIT_FAILURE);
    }

    write_frame_raw(fd, &canmsg);

    close(fd);
    return EXIT_SUCCESS;
}
