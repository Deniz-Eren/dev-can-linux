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

    -V         - Print application version and exit.
    -C         - Print build configurations and exit.
    -i         - List supported hardware and exit.
    -ii        - List supported hardware details and exit.
    -d vid:did - Target desired device, e.g. -d 13fe:c302
    -w         - Print warranty message and exit.
    -c         - Print license details and exit.
    -q         - Quiet mode turns of all terminal printing and trumps all
                 verbose modes. Both stdout and stderr are turned off!
                 Errors and warnings are printed to stderr normally when this
                 option is not selected. Logging to syslog is not impacted by
                 this option.
    -s         - Silent mode disables all CAN-bus TX capability
    -t         - User timestamp mode disables internal timestamping of CAN-bus
                 messages. Instead the driver expects devctl()
                 CAN_DEVCTL_SET_TIMESTAMP command (or set_timestamp() function
                 in dev-can-linux headers) to set the timestamps.
                 In the absence of this option the devctl() set_timestamp()
                 command will synchronize the internal timestamp to the supplied
                 timestamp (in milliseconds). By default this timestamp is with
                 reference to boot-up time.
    -v         - Verbose 1; prints out info to stdout.
    -vv        - Verbose 2; prints out info & debug to stdout.
    -vvv       - Verbose 3; prints out info, debug & trace to stdout.
                 Do NOT enable this for general usage, it is only intended for
                 debugging during development.
    -l         - Log 1; syslog entries for info.
    -ll        - Log 2; syslog entries for info & debug.
    -lll       - Log 3; syslog entries for info, debug & trace.
                 NOT for general use.
    -?/h       - Print help menu and exit.

    NOTES
    
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

    dev-can-linux -d 13fe:c302 -vv -ll


## Configuration

Once the driver is up and running, to configure the CAN-bus channels the QNX
tool canctl is used.

Firstly, when you start the driver the supported number of channels will have
associated file descriptors created. For example, for the dual channel Advantech
PCI devices we will have /dev/can0/rx0, /dev/can0/tx0, /dev/can1/rx0 and
/dev/can1/tx0 appear.


### Setting Bitrate

All configurations done using canctl refer to the file descriptors of the
channels. For example, to set the bitrate:

    dev-can-linux                       # 1) start the driver
    waitfor /dev/can0/rx0               # 2) wait for driver to start
    canctl -u0,rx0 -c 500k,0,0,0,0      # 3) command /dev/can0 to 500kbits/sec

Sample output:

    set_bitrate 500000bits/second: OK   # response on success

Notes: when you set /dev/can0/rx0 the /dev/can0/tx0 is also set and vice-versa.
For any given channel both rx and tx run at the same bitrate.

Regarding the option given above '-c 500k,0,0,0,0' indicates 0 for fields bprm,
ts1, ts2 and sjw. When _bprm_ is set to 0 the driver computes all the fields
bprm, ts1, ts2 and sjw such that the requested baud or bitrate is satisfied. If
the bitrate is specified as 0, the current bitrate is used. The initial default
bitrate is set to 250kbits/second.

You do however have the option to specify all the fields as you like. When you
do the driver will check the numbers and ensure to set them to achievable
values.

Here are some other examples where specific fields are selected:

    canctl -u0,rx0 -c 250k,2,7,2,1
    canctl -u0,rx0 -c 1M,1,3,2,1

To get further verification you can query the canctl tool for channel
information:

    canctl -u0,rx0 -i       

Sample output:

    Message queue size: 0
    Wait queue size:    0
    Mode:               RAW Frame
    Bitrate:            500000 Baud
    Bitrate prescaler:  1
    Sync jump width:    1
    Time segment 1:     7
    Time segment 2:     2
    TX mailboxes:       0
    RX mailboxes:       0
    Loopback:           INTERNAL
    Autobus:            OFF
    Silent mode:        OFF


## Sending and Receiving Test Messages

The candump tool can be used to send and receive test messages also. To listen
to a channel:

    candump -u0,rx0 &

Then to send to the same channel, use cansend tool:

    cansend -u0,tx0 -w0x1234,1,0xABCD

Sample output:

    /dev/can0/rx0 TS: 21531219ms [EFF] 1234 [2] AB CD 00 00 00 00 00 00

To get statistics on sends, receives, errors and more:

    canctl -u0,rx0 -s                       

Sample output:

    Transmitted frames:    7
    Received frames:       0
    Missing ACK:           0
    Total frame errors:    0
    Stuff errors:          0
    Form errors:           0
    Dominant bit recessiv: 0
    Recessiv bit dominant: 0
    Parity errors:         0
    CRC errors:            0
    RX FIFO overflow:      0
    RX queue overflow:     0
    Error warning state #: 0
    Error passive state #: 0
    Bus off state #:       0
    Bus idle state #:      0
    Power down #:          0
    Wake up #:             0
    RX interrupts #:       0
    TX interrupts #:       0
    Total interrupts #:    0

Note "Received frames" will remain at 0 until an external message is received;
the echo from tx file descriptor doesn't count here.


## Installation

To install untar a release package directly to your desired install prefix:

    tar -xf dev-can-linux-#.#.#-qnx710.tar.gz -C /opt/

This example command installs to the prefix "/opt/" but you can specify "/usr/"
or "/usr/local/" or another location.


## Hardware Emulation for Testing

To run QEmu VM with CAN-bus hardware emulation for testing see
["Emulation"](tests/emulation/).


## Building

Please refer to the ["Development"](dev/) documentation.


## Example Applications

If you are interested in developing applications that utilize CAN-bus through
our driver, then take a look at the [cansend](tools/cansend),
[candump](tools/candump) and [canread](tools/canread) applications. These double
up as excellent testing tools and examples of applications that talke to our
driver.

Note also that together with our driver installer package a '-dev' variant
installer is also packaged. This contains the necessary C headers to develop
applications, intended to be installed onto your development environment.


## Check Supported Hardware

Run with '-i' option to check what hardware is supported:

    dev-can-linux -i

Current version output:

    Supports:
      - Advantech PCI cards
      - KVASER PCAN PCI cards
      - EMS CPC-PCI/PCIe/104P CAN cards
      - PEAK PCAN PCI family cards
      - PLX90xx PCI-bridge cards (with the SJA1000 chips)
    
    For more details use option `-ii'

For further details on what devices are supported:

    dev-can-linux -ii

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

Please report back findings from any hardware you test so we can update this
list, your help is much appreciated.
