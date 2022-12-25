/*
 * \file    canctl.c
 * \brief   This program calls the native QNX canctl except for certain
 *          functions that the native version currently has issues handling.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/can_dcmd.h>

#define NATIVE_CANCTL_CMD "/system/xbin/canctl"


int main (int argc, char* argv[]) {
    int rc;
    int opt;
    char buffer[1024];

    int optu_unit;
    char optu_mailbox_str[8];
    int optu_mailbox_is_tx = 0;
    int optu_mailbox = 0;

    int optw = 0;
    int optw_mid = 0;
    int optw_mid_type = 0;
    char optw_data_str[32];

    while ((opt = getopt(argc, argv, "u:w:?h")) != -1) {
        switch (opt) {
        case 'u':
            sscanf(optarg, "%d,%s", &optu_unit, &optu_mailbox_str);
            strncpy(buffer, optu_mailbox_str, 8);
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
            optw = 1;
            sscanf( optarg, "0x%x,%d,0x%s",
                    &optw_mid, &optw_mid_type, &optw_data_str );

            break;

        case '?':
        case 'h':
            rc = system(NATIVE_CANCTL_CMD " -h");

            if( rc == -1 ) {
              printf( "shell could not be run\n" );
            }

            return WEXITSTATUS(rc);

        default:
            printf("invalid option %c\n", opt);
            break;
        }
    }

    if (optw) {
        int     fd, ret;
        struct  can_msg canmsg;

        int n = sscanf( optw_data_str, "%2x%2x%2x%2x%2x%2x%2x%2x",
                &canmsg.dat[0],
                &canmsg.dat[1],
                &canmsg.dat[2],
                &canmsg.dat[3],
                &canmsg.dat[4],
                &canmsg.dat[5],
                &canmsg.dat[6],
                &canmsg.dat[7] );

        int i;
        for (i = n; i < 8; ++i) {
            canmsg.dat[i] = 0;
        }

        canmsg.mid = optw_mid;
        canmsg.ext.timestamp = 0;
        canmsg.ext.is_extended_mid = optw_mid_type;
        canmsg.len = strnlen(optw_data_str, 16)/2;

        char OPEN_FILE[16];

        snprintf( OPEN_FILE, 16, "/dev/can%d/%s%d",
                optu_unit,
                (optu_mailbox_is_tx ? "tx" : "rx"), optu_mailbox );

        if ((fd = open(OPEN_FILE, O_RDWR)) == -1) {
            exit(EXIT_FAILURE);
        }

        if (EOK != (ret = devctl(
                fd, CAN_DEVCTL_WRITE_CANMSG_EXT,
                &canmsg, sizeof(struct can_msg), NULL )))
        {
            fprintf(stderr, "devctl CAN_DEVCTL_SET_MID: %s\n", strerror(ret));
        }

        return EXIT_SUCCESS;
    }

    strcpy(buffer, NATIVE_CANCTL_CMD);

    int i;
    for (i = 1; i < argc; ++i) {
        strcat(buffer, " ");
        strcat(buffer, argv[i]);
    }

    rc = system(buffer);

    if (rc == -1) {
      printf("shell could not be run\n");
    }

    return WEXITSTATUS(rc);
}
