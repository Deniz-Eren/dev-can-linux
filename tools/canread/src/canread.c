/*
 * \file    canread.c
 * \brief   This program reads CAN messages from dev-can-* driver devices and
 *          dump the data to screen as string.
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
    printf("    \e[1mCANREAD\e[m is an accompanying tool used to read textual bytes from CAN\n");
    printf("    messages.\n");
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
    printf("                     canread -u0,rx0\n");
    printf("\n");
    printf("    \e[1m-n bytes\e[m   - Number of bytes to receive before flushing to stdout.\n");
    printf("    \e[1m-w\e[m         - Print warranty message and exit.\n");
    printf("    \e[1m-c\e[m         - Print license details and exit.\n");
    printf("    \e[1m-?/h\e[m       - Print help menu and exit.\n");
    printf("\n");
    printf("\e[1mEXAMPLES\e[m\n");
    printf("    Receive bytes:\n");
    printf("\n");
    printf("        \e[1mcanread -u0,rx0\e[m\n");
    printf("\n");
    printf("    Send bytes to see from another terminal:\n");
    printf("\n");
    printf("        \e[1mecho -n abc > /dev/can0/tx0\e[m\n");
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

    int optn = 1;

    while ((opt = getopt(argc, argv, "u:n:wc?h")) != -1) {
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

        case 'n':
            optn = atoi(optarg);
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

    char OPEN_FILE[16];
    char buf[1024];

    snprintf( OPEN_FILE, 16, "/dev/can%d/%s%d",
            optu_unit,
            (optu_mailbox_is_tx ? "tx" : "rx"), optu_mailbox );

    if ((fd = open(OPEN_FILE, O_RDWR)) == -1) {
        printf("candump error: %s\n", strerror(errno));

        exit(EXIT_FAILURE);
    }

    while (ret == EOK) {
        ssize_t n = read(fd, (void*)buf, optn);

        if (n == -1) {
            printf("canread error: %s\n", strerror(errno));

            exit(EXIT_FAILURE);
        }

        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
    }

    close(fd);
    return ret;
}
