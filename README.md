# DEV-CAN-LINUX

QNX CAN-bus driver project aimed at porting drivers from the Linux Kernel to
QNX. The project aims to achieve sufficient abstraction such that ongoing
updates to the Linux Kernel can be propagated easily.

For details about the current support, aim and methods see
["Linux Kernel"](src/kernel/).

## Usage

The compiled program is used as follows:

    dev-can-linux [options]

Options:

    -V             - print application version and exit
    -l             - list supported hardware and exit
    -ll            - list supported hardware details and exit
    -d {vid}:{did} - target desired device, e.g. -d 13fe:c302
    -v             - verbose 1; syslog(i) info
    -vv            - verbose 2; syslog(i) info & debug
    -vvv           - verbose 3; syslog(i) all, stdout(ii) info
    -vvvv          - verbose 4; syslog(i) all, stdout(ii) info & debug
    -vvvvv         - verbose 5; syslog(i) all + trace, stdout(ii) all
    -vvvvvv        - verbose 6; syslog(i) all + trace, stdout(ii) all + trace
    -q             - quiet mode trumps all verbose modes
    -w             - print warranty message and exit
    -c             - print license details and exit
    -?/h           - print help menu and exit

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


## Installation

To install untar a release package directly to your desired install prefix:

    tar -xf dev-can-linux-#.#.#-qnx710.tar.gz -C /opt/

This example command installs to the prefix "/opt/" but you can specify "/usr/"
or "/usr/local/" or another location.


## Check Supported Hardware

Run with '-l' option to check what hardware is supported:

    dev-can-linux -l

Current version output:

    Supports:
      - Advantech PCI cards
      - KVASER PCAN PCI cards
      - EMS CPC-PCI/PCIe/104P CAN cards
      - PEAK PCAN PCI family cards
      - PLX90xx PCI-bridge cards (with the SJA1000 chips)
    
    For more details use option `-ll'

For further details on what devices are supported:

    dev-can-linux -ll

Current version output:

    Advantech PCI cards:
      Driver: adv_pci
      Supported devices (detailed):
        { vendor: 13fe, device: c302, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
    KVASER PCAN PCI cards:
      Driver: kvaser_pci
      Supported devices (detailed):
        { vendor: 10e8, device: 8406, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1a07, device: 8, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
    EMS CPC-PCI/PCIe/104P CAN cards:
      Driver: ems_pci
      Supported devices (detailed):
        { vendor: 110a, device: 2104, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 10b5, subdevice: 4000, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 10b5, subdevice: 4002, class: 0, class_mask: 0 }
    PEAK PCAN PCI family cards:
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
    PLX90xx PCI-bridge cards (with the SJA1000 chips):
      Driver: sja1000_plx_pci
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
        { vendor: 10b5, device: 3432, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 1498, device: 32a, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 12c4, subdevice: 900, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: e1c5, subdevice: 301, class: 0, class_mask: 0 }
        { vendor: 1393, device: 100, subvendor: ffffffff, subdevice: ffffffff, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 3000, subdevice: 1001, class: 0, class_mask: 0 }
        { vendor: 10b5, device: 9030, subvendor: 3000, subdevice: 1002, class: 0, class_mask: 0 }

## Hardware Test Status

Actual hardware tested currently are:

- Advantech PCI (vendor: 13fe, device: c302)

Please report back your findings with any hardware you are testing with - greatly appreciated.
