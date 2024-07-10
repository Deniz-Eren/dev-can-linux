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
#include <unistd.h>

#include <sys/netmgr.h>

#include <config.h>
#include <pci.h>
#include <interrupt.h>
#include <driver-prints.h>
#include <session.h>


int main_chid = -1;

static void sigint_signal_handler (int sig_no) {
    struct sigevent terminate_event;

    terminate_irq_loop();

    if (main_chid == -1) {
        return;
    }

    int coid = ConnectAttach(ND_LOCAL_NODE, 0, main_chid, _NTO_SIDE_CHANNEL, 0);

    SIGEV_SET_TYPE(&terminate_event, SIGEV_PULSE);
    terminate_event.sigev_coid = coid;

    if (MsgDeliverEvent(0, &terminate_event) == -1) {
        log_err("MsgDeliverEvent error; %s\n", strerror(errno));
    }
}

int main (int argc, char* argv[]) {
    int opt, vid, did, cap, n;
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
#define F81601_EX_CLK   12
        "f81601_ex_clk",
        NULL
    };

    // Backup getopt() globals
    char *opt_bak_optarg = optarg;
    int opt_bak_optind = optind,
        opt_bak_opterr = opterr,
        opt_bak_optopt = optopt;

    // Need to parse -v and -l first so that log_*() functions work within the
    // command-line parsing loop following this one.
    while ((opt = getopt(argc, argv, "r:d:e:U:u:b:m:viqstlVCEwcx?h")) != -1) {
        switch (opt) {
        case 'v':
            optv++;
            break;

        case 'l':
            optl++;
            break;

        default:
            break;
        }
    }

    // Reset getopt() globals
    optarg = opt_bak_optarg;
    optind = opt_bak_optind;
    opterr = opt_bak_opterr;
    optopt = opt_bak_optopt;

    while ((opt = getopt(argc, argv, "r:R:d:e:U:u:b:m:viqstlVCEwcx?h")) != -1) {
        switch (opt) {
        case 'r':
            optr++;
            optr_restart_ms = atoi(optarg);
            break;

        case 'R':
            optR++;
            optR_error_count = atoi(optarg);
            break;

        case 'd':
        {
            device_config_t* new_disable_device_config;

            new_disable_device_config =
                malloc( (num_disable_device_configs + 1) *
                        sizeof(device_config_t) );

            int i;
            for (i = 0; i < num_disable_device_configs; ++i) {
                new_disable_device_config[i] = disable_device_config[i];
            }

            if (num_disable_device_configs) {
                free(disable_device_config);
            }

            optd++;
            n = sscanf(optarg, "%x:%x,%x", &vid, &did, &cap);

            new_disable_device_config[i].vid = vid;
            new_disable_device_config[i].did = did;
            new_disable_device_config[i].cap = -1;

            if (n == 3) {
                new_disable_device_config[i].cap = cap;
            }

            num_disable_device_configs++;
            disable_device_config = new_disable_device_config;

            if (cap == -1) {
                log_info("manually disabling device %04x:%04x\n", vid, did);
            }
            else {
                log_info("manually disabling capability 0x%02x for device "
                        "%04x:%04x\n", cap, vid, did);
            }

            break;
        }
        case 'e':
        {
            device_config_t* new_enable_device_cap_config;

            new_enable_device_cap_config =
                malloc( (num_enable_device_cap_configs + 1) *
                        sizeof(device_config_t) );

            int i;
            for (i = 0; i < num_enable_device_cap_configs; ++i) {
                new_enable_device_cap_config[i] = enable_device_cap_config[i];
            }

            if (num_enable_device_cap_configs) {
                free(enable_device_cap_config);
            }

            opte++;
            sscanf(optarg, "%x:%x,%x", &vid, &did, &cap);

            new_enable_device_cap_config[i].vid = vid;
            new_enable_device_cap_config[i].did = did;
            new_enable_device_cap_config[i].cap = cap;

            num_enable_device_cap_configs++;
            enable_device_cap_config = new_enable_device_cap_config;

            log_info( "enabling capability 0x%02x for device %04x:%04x\n",
                    cap, vid, did );
            break;
        }
        case 'U':
            next_device_id = atoi(optarg);

            if (next_device_id < 0) {
                printf("invalid -U value: %d\n", next_device_id);

                return EXIT_FAILURE;
            }
            break;

        case 'u':
        {
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
            };

            channel_config_t new_channel_config = default_channel_config;

            options = optarg;
            while (*options != '\0') {
                switch (getsubopt(&options, sub_opts, &value)) {
                case CHANNEL_ID:        /* process id option */
                    if (value == NULL) {
                        printf("error with channel id sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_channel_config.id = atoi(value);
                    break;

                case READ_CHANNELS:     /* process rx option */
                    if (value == NULL) {
                        printf("error with read channels sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_channel_config.num_rx_channels = atoi(value);
                    break;

                case WRITE_CHANNELS:    /* process tx option */
                    if (value == NULL) {
                        printf("error with write channels sub-option\n");

                        return EXIT_FAILURE;
                    }

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

                default :
                    /* process unknown token */
                    printf("error: Unknown suboption for -u\n");
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
        }
        case 'b':
        {
            optb++;
            if ((optarg = strdup(optarg)) == NULL)  {
                printf("strdup failure\n");

                return EXIT_FAILURE;
            }
            bitrate_config_t default_bitrate_config = {
                .id = -1,
                .bitrate = 0,           /* Bit-rate in bits/second */
                .bprm = 0,              /* Bit-rate prescaler */
                .phase_seg1 = 0,        /* Phase buffer segment 1 in TQs */
                .phase_seg2 = 0,        /* Phase buffer segment 2 in TQs */
                .sjw = 0,               /* Synchronisation jump width in TQs */
                .btr0 = 0,              /* SJA1000 BTR0 register */
                .btr1 = 0               /* SJA1000 BTR1 register */
            };

            bitrate_config_t new_bitrate_config = default_bitrate_config;

            options = optarg;
            while (*options != '\0') {
                switch (getsubopt(&options, sub_opts, &value)) {
                case CHANNEL_ID:        /* process id option */
                    if (value == NULL) {
                        printf("error with channel id sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_bitrate_config.id = atoi(value);
                    break;

                case FREQ:              /* process freq[kKmM] option */
                {
                    int freq;
                    char units[16];

                    if (value == NULL) {
                        printf("error with frequency sub-option\n");

                        return EXIT_FAILURE;
                    }

                    sscanf(value, "%d%s", &freq, units);

                    if (units[0] == 'k' || units[0] == 'K') {
                        new_bitrate_config.bitrate = freq * 1000;
                    }
                    else if (units[0] == 'm' || units[0] == 'M') {
                        new_bitrate_config.bitrate = freq * 1000000;
                    }
                    else {
                        new_bitrate_config.bitrate = freq;
                    }
                    break;
                }
                case BPRM:              /* process bprm option */
                    if (value == NULL) {
                        printf("error with bprm sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_bitrate_config.bprm = atoi(value);
                    break;

                case TS1:               /* process ts1 option */
                    if (value == NULL) {
                        printf("error with ts1 sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_bitrate_config.phase_seg1 = atoi(value);
                    break;

                case TS2:               /* process ts2 option */
                    if (value == NULL) {
                        printf("error with ts2 sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_bitrate_config.phase_seg2 = atoi(value);
                    break;

                case SJW:               /* process sjw option */
                    if (value == NULL) {
                        printf("error with sjw sub-option\n");

                        return EXIT_FAILURE;
                    }

                    new_bitrate_config.sjw = atoi(value);
                    break;

                case BTR0:              /* process btr0 option */
                {
                    int btr0;

                    if (value == NULL) {
                        printf("error with btr0 sub-option\n");

                        return EXIT_FAILURE;
                    }

                    sscanf(value, "%x", &btr0);
                    new_bitrate_config.btr0 = btr0;
                    break;
                }
                case BTR1:              /* process btr1 option */
                {
                    int btr1;

                    if (value == NULL) {
                        printf("error with btr1 sub-option\n");

                        return EXIT_FAILURE;
                    }

                    sscanf(value, "%x", &btr1);
                    new_bitrate_config.btr1 = btr1;
                    break;
                }
                default :
                    /* process unknown token */
                    printf("error: Unknown suboption for -b\n");
                    break;
                }
            }
            free(optarg);

            int id = new_bitrate_config.id;

            if (num_optb_configs < id + 1) {
                bitrate_config_t* new_optb_config;

                int new_num_optb_configs = id + 1;
                new_optb_config =
                    malloc(new_num_optb_configs*sizeof(bitrate_config_t));

                /* Fill all newly created configs with default settings */
                int i;
                for (i = num_optb_configs; i < new_num_optb_configs; ++i) {
                    new_optb_config[i] = default_bitrate_config;
                }

                /* Copy previous configs to the new configs */
                if (optb_config != NULL) {
                    memcpy( new_optb_config, optb_config,
                            num_optb_configs*sizeof(bitrate_config_t) );

                    free(optb_config);
                }

                /* Set the configs to the new configs */
                num_optb_configs = new_num_optb_configs;
                optb_config = new_optb_config;
            }

            if (id >= 0) {
                optb_config[id] = new_bitrate_config;
            }
            else {
                printf("channel id (%d) invalid\n", id);

                return EXIT_FAILURE;
            }
            break;
        }
        case 'm':
        {
            optm++;
            if ((optarg = strdup(optarg)) == NULL)  {
                printf("strdup failure\n");

                return EXIT_FAILURE;
            }

            options = optarg;
            while (*options != '\0') {
                switch (getsubopt(&options, sub_opts, &value)) {
                case F81601_EX_CLK:         /* f81601 kernel module parameter
                                               for external clock */
                {
                    if (value == NULL) {
                        printf("error with f81601 external clock sub-option\n");

                        return EXIT_FAILURE;
                    }

                    internal_clk = false;
                    external_clk = atoi(value);

                    log_info( "driver f81601 set to external clock (%d)\n",
                            external_clk );
                    break;
                }
                default :
                    /* process unknown token */
                    printf("error: Unknown suboption for -m\n");
                    break;
                }
            }

            free(optarg);
            break;
        }
        case 'v':
            // Already handled in the first loop
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
            // Already handled in the first loop
            break;

        case 'i':
            opti++;
            break;

        case 'E':
            optE = 1;
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

    if (opti) {
        print_support(opti);

        return EXIT_SUCCESS;
    }

    if (!optq) {
        print_notice();
    }

    bool allow_driver_start = true;

    if (optr > 1) {
        log_err("error: only a single entry for option -r is allowed.\n");

        allow_driver_start = false;
    }

#if RELEASE_BUILD == 1
    if (optv > 2) {
        optv = 2;

        log_warn("warning: release versions allow at max -vv option.\n");
    }

    if (optl > 2) {
        optl = 2;

        log_warn("warning: release versions allow at max -ll option.\n");
    }
#endif

    // Make sure all -u id=# are greater than the -U# base index
    for (int i = 0; i < num_optu_configs; ++i) {
        if (optu_config[i].id != -1
            && optu_config[i].id < next_device_id)
        {
            log_err("error: config -u with invalid id=%d less than -U %d\n",
                    optu_config[i].id, next_device_id);

            allow_driver_start = false;
        }
    }

    // Make sure all -b id=# are greater than the -U# base index
    for (int i = 0; i < num_optb_configs; ++i) {
        if (optb_config[i].id != -1
            && optb_config[i].id < next_device_id)
        {
            log_err("error: config -b with invalid id=%d less than -U %d\n",
                    optb_config[i].id, next_device_id);

            allow_driver_start = false;
        }
    }

    log_info("driver start (version: %s)\n", PROGRAM_VERSION);

    ThreadCtl(_NTO_TCTL_IO, 0);

    signal(SIGINT, sigint_signal_handler);

    if (fixed_memory_init()) {
        log_err("fixed_memory_init fail\n");

        return EXIT_FAILURE;
    }

    if (process_driver_selection()) {
        allow_driver_start = false;
    }

    print_driver_selection_results();

    if (probe_all_driver_selections()) {
        log_err("no devices detected\n");

        allow_driver_start = false;
    }

    // Make sure all -u id=# are less than the maximum detected device index
    for (int i = 0; i < num_optu_configs; ++i) {
        if (optu_config[i].id != -1
            && optu_config[i].id >= next_device_id)
        {
            log_err("error: config -u with invalid id=%d greater than maximum "
                    "detected device id=%d\n",
                    optu_config[i].id, next_device_id-1);

            allow_driver_start = false;
        }
    }

    // Make sure all -b id=# are less than the maximum detected device index
    for (int i = 0; i < num_optb_configs; ++i) {
        if (optb_config[i].id != -1
            && optb_config[i].id >= next_device_id)
        {
            log_err("error: config -b with invalid id=%d greater than maximum "
                    "detected device id=%d\n",
                    optb_config[i].id, next_device_id-1);

            allow_driver_start = false;
        }
    }

    if (!allow_driver_start) {
        return EXIT_FAILURE;
    }

    int err;
    int policy;
    struct sched_param param;

    err = pthread_getschedparam(pthread_self(), &policy, &param);

    if (err != EOK) {
        log_err("error pthread_getschedparam: %s\n", strerror(err));

        return EXIT_FAILURE;
    }

    pthread_attr_t irq_thread_attr;
    pthread_t irq_thread;

    pthread_attr_init(&irq_thread_attr);
    pthread_attr_setdetachstate(&irq_thread_attr, PTHREAD_CREATE_DETACHED);

    pthread_attr_setinheritsched(&irq_thread_attr, PTHREAD_EXPLICIT_SCHED);

    param.sched_priority += CONFIG_IRQ_SCHED_PRIORITY_BOOST;
    pthread_attr_setschedparam(&irq_thread_attr, &param);

    pthread_create(&irq_thread, &irq_thread_attr, &irq_loop, NULL);

    unsigned flags = _NTO_CHF_PRIVATE;
    main_chid = ChannelCreate(flags);

    for(;;) {
        struct _pulse pulse;
        MsgReceivePulse(main_chid, &pulse, sizeof(pulse), NULL);

        if (shutdown_program) {
            log_info("Shutdown program\n");

            break;
        }
    }

    /*
     * In practice the program runs forever or until the user terminates it;
     * thus we can never reach here.
     */
    remove_all_driver_selections();

    irq_group_cleanup();

    free(optu_config);

    return EXIT_SUCCESS;
}
