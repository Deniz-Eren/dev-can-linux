# DEV-CAN-LINUX

QNX CAN-bus driver project aimed at porting drivers from the Linux Kernel to
QNX. The project aims to achieve sufficient abstraction such that ongoing
updates to the Linux Kernel can be propagated easily.

For details about the current support, aim and methods see
["Linux Kernel"](src/kernel/).

## Usage

The compiled program is used as follows:

    dev-can-linux [-l] [-d {vid}:{did}] [-v[v..]] [-?/h]

Command-line arguments:

    -l             - list supported drivers
    -d {vid}:{did} - target desired device, e.g. -d 13fe:c302
    -v             - verbose 1; syslog(i) info
    -vv            - verbose 2; syslog(i) info & debug
    -vvv           - verbose 3; syslog(i) all, stdout(ii) info
    -vvvv          - verbose 4; syslog(i) all, stdout(ii) info & debug
    -vvvvv         - verbose 5; syslog(i) all + trace, stdout(ii) all
    -vvvvvv        - verbose 6; syslog(i) all + trace, stdout(ii) all + trace
    -q             - quiet mode trumps all verbose modes
    -w             - warranty message
    -c             - license details
    -?/h           - help menu

    Notes:
    
      (i) use command slog2info to check output to syslog
     (ii) stdout is the standard output stream you are reading now on screen
    (iii) stderr is the standard error stream; by default writes to screen
     (iv) errors & warnings are logged to syslog & stderr unaffected by verbose
          modes but silenced by quiet mode
      (v) "trace" level logging is only useful when single messages are sent
          and received, intended only for testing during implementation of new
          driver support.

Examples:

Run with auto detection of hardware:

    dev-can-linux

Check syslog for errors & warnings:

    slog2info

If multiple supported cards are installed, the first supported card will be
automatically chosen. To override this behaviour and manually specify the
desired device, first find out what the vendor ID (vid) and device ID (did) of
the desired card is as follows:

    pci-tool -v

An example output looks like this:

    B000:D05:F00 @ idx 7
            vid/did: 13fe/c302
                    <vendor id - unknown>, <device id - unknown>
            class/subclass/reg: 0c/09/00
                    CANbus Serial Bus Controller

In this example we would chose the numbers vid/did: 13fe/c302

Target specific hardware detection of hardware and enable max verbose mode for
debugging:

    dev-can-linux -d 13fe:c302 -vvvv


## Check Supported Hardware

Run with '-l' option to check what hardware is supported:

    dev-can-linux -l

Current version output:

    Card(s): Advantech PCI:
      Driver: adv_pci
      Supported devices (detailed):
        { vendor: 13fe, device: c302, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
    Card(s): KVASER PCAN PCI CAN:
      Driver: kvaser_pci
      Supported devices (detailed):
        { vendor: 10e8, device: 8406, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1a07, device: 8, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
    Card(s): EMS CPC-PCI/PCIe/104P CAN:
      Driver: ems_pci
      Supported devices (detailed):
        { vendor: 110a, device: 2104, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 10b5, subdevice: 4000, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 10b5, subdevice: 4002, class: 0, class_mask: 0 }
    Card(s): PEAK PCAN PCI/PCIe/PCIeC miniPCI CAN cards,
        PEAK PCAN miniPCIe/cPCI PC/104+ PCI/104e CAN Cards:
      Driver: peak_pci
      Supported devices (detailed):
        { vendor: 1c, device: 1, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 3, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 5, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 8, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 6, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 7, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 4, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1c, device: 9, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
    Card(s): Adlink PCI-7841/cPCI-7841,
        Adlink PCI-7841/cPCI-7841 SE,
        Marathon CAN-bus-PCI,
        TEWS TECHNOLOGIES TPMC810,
        esd CAN-PCI/CPCI/PCI104/200,
        esd CAN-PCI/PMC/266,
        esd CAN-PCIe/2000,
        Connect Tech Inc. CANpro/104-Plus Opto (CRG001),
        IXXAT PC-I 04/PCI,
        ELCUS CAN-200-PCI:  Driver: sja1000_plx_pci
      Supported devices (detailed):
        { vendor: 144a, device: 7841, subvendor: ffffffff, subdevice: ffffffff, class: 28000, class_mask: ffffffff }
        { vendor: 144a, device: 7841, subvendor: ffffffff, subdevice: ffffffff, class: 78000, class_mask: ffffffff }
        { vendor: 10b5, device: 9050, subvendor: 12fe, subdevice: 4, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 12fe, subdevice: 10b, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 12fe, subdevice: 501, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9056, subvendor: 12fe, subdevice: 9, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9056, subvendor: 12fe, subdevice: e, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9056, subvendor: 12fe, subdevice: 200, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9050, subvendor: ffffffff, subdevice: 2540, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 2715, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1498, device: 32a, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 12c4, subdevice: 900, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: e1c5, subdevice: 301, class: 0, class_mask: 0 }


## Hardware Test Status

Actual hardware tested currently are:

- Advantech PCI (vendor: 13fe, device: c302)

Please report back your findings with any hardware you are testing with - greatly appreciated.
