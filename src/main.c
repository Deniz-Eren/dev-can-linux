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
#include <driver-prints.h>
#include <session.h>


static void sigint_signal_handler (int sig_no) {
    terminate_run_wait();
}

int main (int argc, char* argv[]) {
    int opt;
    char *options, *value;

    /*
     * Program options
     *
     * See print_help() or dev-can-linux -h for details
     */

    char *sub_opts[] = {
#define CHANNEL_ID      0
        "id",
#define READ_CHANNELS   1
        "rx",
#define WRITE_CHANNELS  2
        "tx",
#define IS_STANDARD_MID 3
        "s",
#define IS_EXTENDED_MID 4
        "x",
#define FREQ            5
        "freq",
#define BPRM            6
        "bprm",
#define TS1             7
        "ts1",
#define TS2             8
        "ts2",
#define SJW             9
        "sjw",
#define BTR0            10
        "btr0",
#define BTR1            11
        "btr1",
        NULL
    };

    while ((opt = getopt(argc, argv, "b:d:U:u:viqstlVCwcx?h")) != -1) {
        switch (opt) {
        case 'b':
            optb++;
            optb_restart_ms = atoi(optarg);
            break;

        case 'd':
            optd++;
            sscanf(optarg, "%x:%x", &opt_vid, &opt_did);
            log_info("manually disabling device: %x:%x\n", opt_vid, opt_did);
            break;

        case 'U':
            next_device_id = atoi(optarg);

            if (next_device_id < 0) {
                printf("invalid -U value: %d\n", next_device_id);

                return EXIT_FAILURE;
            }
            break;

        case 'u':
            optu++;
            if ((optarg = strdup(optarg)) == NULL)  {
                printf("strdup failure\n");

                return EXIT_FAILURE;
            }
            channel_config_t default_channel_config = {
                .id = -1,
                .num_rx_channels = DEFAULT_NUM_RX_CHANNELS,
                .num_tx_channels = DEFAULT_NUM_TX_CHANNELS,
                .is_extended_mid = -1,
                .bitrate = 0,           /* Bit-rate in bits/second */
                .bprm = 0,              /* Bit-rate prescaler */
                .phase_seg1 = 0,        /* Phase buffer segment 1 in TQs */
                .phase_seg2 = 0,        /* Phase buffer segment 2 in TQs */
                .sjw = 0,               /* Synchronisation jump width in TQs */
                .btr0 = 0,              /* SJA1000 BTR0 register */
                .btr1 = 0               /* SJA1000 BTR1 register */
            };

            channel_config_t new_channel_config = default_channel_config;

            options = optarg;
            while (*options != '\0') {
                switch (getsubopt(&options, sub_opts, &value)) {
                case CHANNEL_ID:        /* process id option */
                    new_channel_config.id = atoi(value);
                    break;

                case READ_CHANNELS:     /* process rx option */
                    new_channel_config.num_rx_channels = atoi(value);
                    break;

                case WRITE_CHANNELS:    /* process tx option */
                    new_channel_config.num_tx_channels = atoi(value);
                    break;

                case IS_STANDARD_MID:   /* process standard MID option */
                    // Only applicable for read and write functions not used for
                    // direct devctl send/receive functionality.
                    if (new_channel_config.is_extended_mid != -1) {
                        printf("error with MID configuration parameters\n");

                        return EXIT_FAILURE;
                    }

                    new_channel_config.is_extended_mid = 0;
                    break;

                case IS_EXTENDED_MID:   /* process extended MID option */
                    // Only applicable for read and write functions not used for
                    // direct devctl send/receive functionality.
                    if (new_channel_config.is_extended_mid != -1) {
                        printf("error with MID configuration parameters\n");

                        return EXIT_FAILURE;
                    }

                    new_channel_config.is_extended_mid = 1;
                    break;

                case FREQ:              /* process freq[kKmM] option */
                {
                    int freq;
                    char units[16];

                    sscanf(value, "%d%s", &freq, units);

                    if (units[0] == 'k' || units[0] == 'K') {
                        new_channel_config.bitrate = freq * 1000;
                    }
                    else if (units[0] == 'm' || units[0] == 'M') {
                        new_channel_config.bitrate = freq * 1000000;
                    }
                    else {
                        new_channel_config.bitrate = freq;
                    }
                    break;
                }
                case BPRM:              /* process bprm option */
                    new_channel_config.bprm = atoi(value);
                    break;

                case TS1:               /* process ts1 option */
                    new_channel_config.phase_seg1 = atoi(value);
                    break;

                case TS2:               /* process ts2 option */
                    new_channel_config.phase_seg2 = atoi(value);
                    break;

                case SJW:               /* process sjw option */
                    new_channel_config.sjw = atoi(value);
                    break;

                case BTR0:              /* process btr0 option */
                {
                    int btr0;
                    sscanf(value, "%x", &btr0);
                    new_channel_config.btr0 = btr0;
                    break;
                }
                case BTR1:              /* process btr1 option */
                {
                    int btr1;
                    sscanf(value, "%x", &btr1);
                    new_channel_config.btr1 = btr1;
                    break;
                }
                default :
                    /* process unknown token */
                    printf("unknown\n");
                    break;
                }
            }
            free(optarg);

            int id = new_channel_config.id;

            if (num_optu_configs < id + 1) {
                channel_config_t* new_optu_config;

                int new_num_optu_configs = id + 1;
                new_optu_config =
                    malloc(new_num_optu_configs*sizeof(channel_config_t));

                /* Fill all newly created configs with default settings */
                int i;
                for (i = num_optu_configs; i < new_num_optu_configs; ++i) {
                    new_optu_config[i] = default_channel_config;
                }

                /* Copy previous configs to the new configs */
                if (optu_config != NULL) {
                    memcpy( new_optu_config, optu_config,
                            num_optu_configs*sizeof(channel_config_t) );

                    free(optu_config);
                }

                /* Set the configs to the new configs */
                num_optu_configs = new_num_optu_configs;
                optu_config = new_optu_config;
            }

            if (id >= 0) {
                optu_config[id] = new_channel_config;
            }
            else {
                printf("channel id (%d) invalid\n", id);

                return EXIT_FAILURE;
            }
            break;

        case 'v':
            optv++;
            break;

        case 'q':
            optq = 1;
            break;

        case 's':
            opts = 1;
            break;

        case 't':
            optt = 1;
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

        case 'x':
            optx = 1;
            break;

        case '?':
        case 'h':
            print_help(argv[0]);
            return EXIT_SUCCESS;

        default:
            printf("invalid option %c\n", opt);
            break;
        }
    }

    if (optb > 1) {
        printf("error: only a single entry for option -b is allowed.\n");

        return -1;
    }

    if (optd > 1) {
        printf("error: only a single device can be disabled.\n");

        return -1;
    }

#if RELEASE_BUILD == 1
    if (optv > 2) {
        optv = 2;

        printf("warning: release versions allow at max -vv option.\n");
    }

    if (optl > 2) {
        optl = 2;

        printf("warning: release versions allow at max -ll option.\n");
    }
#endif

    if (opti) {
        print_support(opti);

        return EXIT_SUCCESS;
    }

    if (!optq) {
        print_notice();
    }

    log_info("driver start (version: %s)\n", PROGRAM_VERSION);

    ThreadCtl(_NTO_TCTL_IO, 0);

    signal(SIGINT, sigint_signal_handler);

    if (fixed_memory_init()) {
        log_err("fixed_memory_init fail\n");

        return -1;
    }

    if (process_driver_selection()) {
        return -1;
    }

    print_driver_selection_results();

    if (probe_all_driver_selections()) {
        return -1;
    }

    run_wait();

    /*
     * In practice the program runs forever or until the user terminates it;
     * thus we can never reach here.
     */
    remove_all_driver_selections();

    free(optu_config);

    return EXIT_SUCCESS;
}
