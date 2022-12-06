/*
 * \file    main.c
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
#include <unistd.h>
#include <hw/inout.h>
#include <pthread.h>

#include <linux/can/dev.h>

#include "pci.h"
#include "interrupt.h"
#include "config.h"
#include "dev.h"
#include "prints.h"

int optv = 0;
int optq = 0;
int optd = 0, opt_vid = -1, opt_did = -1;


void* test_tx (void*  arg) {
    struct sk_buff *skb;
    struct can_frame *cf;

    while (1) {
        sleep(5);

        /* create zero'ed CAN frame buffer */
        skb = alloc_can_skb(device[1], &cf);

        cf->can_id = 0x123;
        cf->can_dlc = 8;
        cf->data[0] = 0x11;
        cf->data[1] = 0x22;
        cf->data[2] = 0x33;
        cf->data[3] = 0x44;
        cf->data[4] = 0x55;
        cf->data[5] = 0x66;
        cf->data[6] = 0x77;
        cf->data[7] = 0x88;

        if (skb == NULL) {
            return (0);
        }

        device[1]->netdev_ops->ndo_start_xmit(skb, device[1]);
    }

    return (0);
}

int main (int argc, char* argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "d:vqlwc?h")) != -1) {
        switch (opt) {
        case 'd':
            optd = 1;
            sscanf(optarg, "%x:%x", &opt_vid, &opt_did);
            log_info("manual device selection: %x:%x\n", opt_vid, opt_did);
            break;

        case 'v':
            optv++;
            break;

        case 'q':
            optq = 1;
            break;

        case 'l':
            print_support();
            return EXIT_SUCCESS;

        case 'w':
            print_warranty();
            return EXIT_SUCCESS;

        case 'c':
            print_license();
            return EXIT_SUCCESS;

        case '?':
        case 'h':
            print_help(argv[0]);
            return EXIT_SUCCESS;

        default:
            printf("invalid option %c\n", opt);
            break;
        }
    }

    if (!optq) {
        print_notice();
    }

    log_info("driver start (version: %s)\n", program_version);

    ThreadCtl(_NTO_TCTL_IO, 0);

    struct driver_selection_t ds;

    if (process_driver_selection(&ds)) {
        return -1;
    }

    print_driver_selection_results(&ds);

    if (!detected_driver) {
        return -1;
    }

    struct pci_dev pdev = {
            .ba = NULL,
            .vendor = ds.vid,
            .device = ds.did,
            .dev = { .driver_data = NULL },
            .irq = 10
    };

    if (detected_driver->probe(&pdev, NULL)) {
        return -1;
    }

    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(NULL, &attr, &test_tx, NULL);

    run_interrupt_wait();

    adv_pci_driver.remove(&pdev);

    return EXIT_SUCCESS;
}
