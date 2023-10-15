/*
 * \file    candump.c
 * \brief   This program reads CAN messages from dev-can-* driver devices and
 *          dump them to screen in human readable form.
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

#include <prints.h>
#include <dev-can-linux/commands.h>


static int fd = -1;

static void sigint_signal_handler (int sig_no) {
    close(fd);

    exit(0);
}

void help (char* program_name) {
    print_notice();

    printf("\n");
    printf("\e[1mSYNOPSIS\e[m\n");
    printf("    \e[1m%s\e[m [options]\n", program_name);
    printf("\n");
    printf("\e[1mDESCRIPTION\e[m\n");
    printf("    \e[1mDEV-CAN-LINUX\e[m is a QNX CAN-bus driver project that aims at porting drivers\n");
    printf("    from the open-source Linux Kernel project to QNX RTOS.\n");
    printf("\n");
    printf("    \e[1mCANDUMP\e[m is an accompanying tool used to read raw CAN messages.\n");
    printf("\n");
    printf("\e[1mOPTIONS\e[m\n");
    printf("    \e[1m-u subopts\e[m - Specify the device RX file descriptors.\n");
    printf("\n");
    printf("                 Suboptions (\e[1msubopts\e[m):\n");
    printf("\n");
    printf("                 \e[1m#\e[m      - Specify ID number of the device to configure;\n");
    printf("                          e.g. /dev/can0/ is -u0\n");
    printf("                 \e[1mrx#\e[m    - ID of RX file descriptors to connect to\n");
    printf("\n");
    printf("                 Example:\n");
    printf("                     candump -u0,rx0\n");
    printf("\n");
    printf("    \e[1m-w\e[m         - Print warranty message and exit.\n");
    printf("    \e[1m-c\e[m         - Print license details and exit.\n");
    printf("    \e[1m-?/h\e[m       - Print help menu and exit.\n");
    printf("\n");
    printf("\e[1mBUGS\e[m\n");
    printf("    If you find a bug, please report it.\n");
}

int main (int argc, char* argv[]) {
    int opt;
    char* buffer;

    int optu_unit = 0;
    char optu_mailbox_str[32];
    int optu_mailbox_is_tx = 0;
    int optu_mailbox = 0;

    while ((opt = getopt(argc, argv, "u:wc?h")) != -1) {
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
            print_warranty();
            return EXIT_SUCCESS;

        case 'c':
            print_license();
            return EXIT_SUCCESS;

        case '?':
        case 'h':
            help(argv[0]);
            return EXIT_SUCCESS;

        default:
            printf("invalid option %c\n", opt);
            break;
        }
    }

    signal(SIGINT, sigint_signal_handler);

    int     ret = EOK;
    struct  can_msg canmsg;

    char OPEN_FILE[16];

    snprintf( OPEN_FILE, 16, "/dev/can%d/%s%d",
            optu_unit,
            (optu_mailbox_is_tx ? "tx" : "rx"), optu_mailbox );

    if ((fd = open(OPEN_FILE, O_RDWR)) == -1) {
        printf("candump error: %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    while (ret == EOK) {
        if ((ret = read_frame_raw_block(fd, &canmsg)) == EOK) {
            printf("  %s TS: %ums [%s] %X [%d] " \
                      "%02X %02X %02X %02X %02X %02X %02X %02X\n",
                    OPEN_FILE,
                    canmsg.ext.timestamp,
                    canmsg.ext.is_extended_mid ? "EFF" : "SFF",
                    /**
                     * Message IDs or MIDs are slightly different on QNX compared
                     * to Linux. The form of the ID depends on whether or not the
                     * driver is using extended MIDs:
                     *
                     *      - In standard 11-bit MIDs, bits 18–28 define the MID.
                     *      - In extended 29-bit MIDs, bits 0–28 define the MID.
                     */
                    canmsg.ext.is_extended_mid ? canmsg.mid : canmsg.mid >> 18,
                    canmsg.len,
                    canmsg.dat[0],
                    canmsg.dat[1],
                    canmsg.dat[2],
                    canmsg.dat[3],
                    canmsg.dat[4],
                    canmsg.dat[5],
                    canmsg.dat[6],
                    canmsg.dat[7]);
        }
    }

    close(fd);
    return ret;
}
