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

#include <dev-can-linux/commands.h>


static int fd = -1;

static void sigint_signal_handler (int sig_no) {
    close(fd);

    exit(0);
}

int main (int argc, char* argv[]) {
    int opt;
    char* buffer;

    int optu_unit = 0;
    char optu_mailbox_str[32];
    int optu_mailbox_is_tx = 0;
    int optu_mailbox = 0;

    int optW = 1;

    while ((opt = getopt(argc, argv, "u:W:?h")) != -1) {
        switch (opt) {
        case 'u':
            buffer = optu_mailbox_str;
            sscanf(optarg, "%d,%s", &optu_unit, buffer);
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

        case 'W':
            optW = atoi(optarg);
            break;

        case '?':
        case 'h':
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
        ssize_t n = read(fd, (void*)buf, optW);

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
