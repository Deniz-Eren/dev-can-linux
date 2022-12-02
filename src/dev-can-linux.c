#include <stdio.h>
#include <unistd.h>
#include <hw/inout.h>
#include <pthread.h>

#include <linux/pci.h>
#include <linux/can/dev.h>

#define program_version "1.0.0"

int optv = 0;
int optq = 0;
int optd = 0, opt_vid = -1, opt_did = -1;

extern struct pci_driver *detected_driver;

/* Defined in src/dev.c */
extern struct net_device* device[16];
extern netdev_tx_t (*dev_xmit[16]) (struct sk_buff *skb, struct net_device *dev);

/* Defined in src/interrupt.c */
extern void run_interrupt_wait();

/* Defined in src/pci.c */
extern void print_card (FILE* file, const struct pci_driver* driver);
extern int process_driver_selection (struct driver_selection_t* ds);


void* test_tx (void*  arg) {
	struct sk_buff *skb;
	struct can_frame *cf;

	/* create zero'ed CAN frame buffer */
	skb = alloc_can_skb(device[1], &cf);

	if (skb == NULL) {
		return (0);
	}

	while (1) {
		sleep(5);
		dev_xmit[1](skb, device[1]);
	}

	return (0);
}

int main (int argc, char* argv[]) {
    log_info("driver start (version: %s)\n", program_version);

    int opt;

    while ((opt = getopt(argc, argv, "d:vql?h")) != -1) {
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
        	printf("Card(s): Advantech PCI:\n");
        	print_card(stdout, &adv_pci_driver);

        	printf("Card(s): KVASER PCAN PCI CAN:\n");
        	print_card(stdout, &kvaser_pci_driver);

        	printf("Card(s): EMS CPC-PCI/PCIe/104P CAN:\n");
        	print_card(stdout, &ems_pci_driver);

        	printf("Card(s): PEAK PCAN PCI/PCIe/PCIeC miniPCI CAN cards,\n");
        	printf("    PEAK PCAN miniPCIe/cPCI PC/104+ PCI/104e CAN Cards:\n");
        	print_card(stdout, &peak_pci_driver);

        	printf("Card(s): Adlink PCI-7841/cPCI-7841,\n"
        			"    Adlink PCI-7841/cPCI-7841 SE,\n"
        			"    Marathon CAN-bus-PCI,\n"
        			"    TEWS TECHNOLOGIES TPMC810,\n"
        			"    esd CAN-PCI/CPCI/PCI104/200,\n"
        			"    esd CAN-PCI/PMC/266,\n"
        			"    esd CAN-PCIe/2000,\n"
        			"    Connect Tech Inc. CANpro/104-Plus Opto (CRG001),\n"
        			"    IXXAT PC-I 04/PCI,\n"
        			"    ELCUS CAN-200-PCI:");
        	print_card(stdout, &plx_pci_driver);
        	return EXIT_SUCCESS;

        case '?':
        case 'h':
            printf("%s - Linux SJA1000 Drivers Ported to QNX\n", argv[0]);
            printf("version %s\n\n", program_version);
            printf("%s [-l] [-d {vid}:{did}] [-v[v..]] [-?/h]\n", argv[0]);
            printf("\n");
            printf("Command-line arguments:\n");
            printf("\n");
            printf(" -l             - list supported drivers\n");
            printf(" -d {vid}:{did} - manually target desired device, e.g. -d 13fe:c302\n");
            printf(" -v             - verbose 1; syslog(i) info\n");
            printf(" -vv            - verbose 2; syslog(i) info & debug\n");
            printf(" -vvv           - verbose 3; syslog(i) all, stdout(ii) info\n");
            printf(" -vvvv          - verbose 4; syslog(i) all, stdout(ii) info & debug\n");
            printf(" -vvvvv         - verbose 5; syslog(i) all + trace, stdout(ii) all\n");
            printf(" -vvvvvv        - verbose 6; syslog(i) all + trace, stdout(ii) all + trace\n");
            printf(" -q             - quiet mode trumps all verbose modes\n");
            printf(" -?/h           - help menu\n");
            printf("\n");
            printf("Notes:\n");
            printf("\n");
            printf("  (i) use command slog2info to check output to syslog\n");
            printf(" (ii) stdout is the standard output stream you are reading now\n");
            printf("(iii) errors & warnings are logged to syslog & stdout unaffected by verbose\n");
            printf("      modes but silenced by quiet mode\n");
			printf(" (iv) \"trace\" level logging is only useful when single messages are sent\n");
			printf("      and received, intended only for testing during implementation of new\n");
			printf("      driver support.\n");
            printf("\n");
            printf("Examples:\n");
            printf("\n");
            printf("Run with auto detection of hardware:\n");
            printf("  dev-can-linux\n");
            printf("\n");
            printf("Check syslog for errors & warnings:\n");
            printf("  slog2info\n");
            printf("\n");
            printf("If multiple supported cards are installed, the first supported card will be\n");
            printf("automatically chosen. To override this behaviour and manually specify the\n");
            printf("desired device, first find out what the vendor ID (vid) and device ID (did) of\n");
            printf("the desired card is as follows:\n");
			printf("  pci-tool -v\n");
			printf("\n");
            printf("An example output looks like this:\n");
			printf("  B000:D05:F00 @ idx 7\n");
			printf("          vid/did: 13fe/c302\n");
			printf("                  <vendor id - unknown>, <device id - unknown>\n");
			printf("          class/subclass/reg: 0c/09/00\n");
			printf("                  CANbus Serial Bus Controller\n");
			printf("\n");
			printf("In this example we would chose the numbers vid/did: 13fe/c302\n");
			printf("\n");
            printf("Target specific hardware detection of hardware and enable max verbose mode for\n");
            printf("debugging:\n");
            printf("  dev-can-linux -d 13fe:c302 -vvvv\n");
        	return EXIT_SUCCESS;

        default:
            printf("invalid option %c\n", opt);
            break;
        }
    }

	ThreadCtl(_NTO_TCTL_IO, 0);

	struct driver_selection_t ds;

	if (process_driver_selection(&ds)) {
		return -1;
	}

	if (ds.driver_auto) {
		log_info("Auto detected device (%x:%x) successfully; (driver \"%s\")\n", ds.vid, ds.did, detected_driver->name);
	}
	else if (ds.driver_pick) {
		log_info("Device (%x:%x) accepted successfully; (driver \"%s\")\n", ds.vid, ds.did, detected_driver->name);
	}
	else if (ds.driver_unsupported) {
		log_info("Device (%x:%x) not supported by any driver\n", opt_vid, opt_did);
	}
	else {
		log_info("Device (%x:%x) not a valid device\n", opt_vid, opt_did);
	}

	if (ds.driver_ignored) {
		log_info("Note: one or more supported devices have been ignored because of manual device selection\n");
	}

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
