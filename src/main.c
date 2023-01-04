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
#include <signal.h>

#include <config.h>
#include <pci.h>
#include <interrupt.h>
#include <prints.h>
#include <session.h>

struct pci_dev pdev = {
        .ba = NULL,
        .vendor = 0,
        .device = 0,
        .dev = { .driver_data = NULL },
        .irq = 0
};


void sigint_signal_handler (int sig_no) {
    terminate_run_wait();
}

int main (int argc, char* argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, "d:viqlVCwc?h")) != -1) {
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
            optl++;
            break;

        case 'i':
            opti++;
            break;

        case 'V':
            print_version();
            return EXIT_SUCCESS;

        case 'C':
            print_configs();
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

#if RELEASE_BUILD == 1
    if (optv > 2) {
        optv = 2;
    }

    if (optl > 2) {
        optl = 2;
    }
#endif

    if (opti) {
        print_support(opti > 1);

        return EXIT_SUCCESS;
    }

    if (!optq) {
        print_notice();
    }

    log_info("driver start (version: %s)\n", PROGRAM_VERSION);

    ThreadCtl(_NTO_TCTL_IO, 0);

    signal(SIGINT, sigint_signal_handler);

    struct driver_selection_t ds;

    if (process_driver_selection(&ds)) {
        return -1;
    }

    print_driver_selection_results(&ds);

    if (!detected_driver) {
        return -1;
    }

    pdev.vendor = ds.vid;
    pdev.device = ds.did;

    if (detected_driver->probe(&pdev, NULL)) {
        return -1;
    }

    run_wait();

    /*
     * In practice the program runs forever or until the user terminates it;
     * thus we can never reach here.
     */
    adv_pci_driver.remove(&pdev);

    free(device_sessions);

    return EXIT_SUCCESS;
}
