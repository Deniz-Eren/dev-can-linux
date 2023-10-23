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
    -U base_id - First id to use for devices detected
                 e.g. /dev/can9/ is id=9
                 Default: 0
    -u subopts - Configure the device RX/TX file descriptors.

                 Suboptions (subopts):

                 id=#       - Specify ID number of the device to configure;
                              e.g. /dev/can0/ is id=0
                              This id is consistent with the base_id you have
                              specified with the -U option.
                 rx=#       - Number of RX file descriptors to create
                 tx=#       - Number of TX file descriptors to create
                 s          - Start the device with standard MIDs
                 x          - Start the device with extended MIDs
                              Default: is setup as standard MIDs or determined by
                              driver level -x option if specified

                 Baud Rate Suboptions (subopts):

                 Normally you would use canctl command to set baud rate, however
                 you do have the option of setting it here at driver startup.
                 See section [Setting Bitrate](#setting-bitrate) for more
                 details.

                 freq[kKmM]=# - The clock rate in Hz, optionally followed by a
                                multiplier of k or K for 1000, and m or M for
                                1000000

                 The driver will attempt to automatically compute the following
                 fields, however you can explicitly set them also:

                 bprm=#     - The baud rate prescaler
                 ts1=#      - The number of time quantas for time segment 1
                 ts2=#      - The number of time quantas for time segment 2
                 sjw=#      - The number of time quantas for the syncronization
                              jump width

                 For special applications that need to manually set the BTR0 and
                 BTR1 registers of the SJA1000 chipset use the following
                 suboptions. Note other suboptions (bprm, ts1, ts2 and sjw) will
                 be ignored and derived from the specified register values.
                 Suboption freq must be specified and will be taken and trusted
                 verbatim.

                 btr0=#     - SJA1000 Bus Timing Register 0
                 btr1=#     - SJA1000 Bus Timing Register 1

                 Examples:
                     # Specify 2 RX and 0 TX file descriptors in /dev/can0/*:
                     dev-can-linux -u id=0,rx=2,tx=0

                     # Setting baud-rate at driver start time:
                     dev-can-linux -u id=0,freq=250k,bprm=2,ts1=7,ts2=2,sjw=1

                     # (Special cases only) Forced btr* baud-rate setting method:
                     dev-can-linux -u id=0,freq=125k,btr0=0x07,btr1=0x14

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
    -d vid:did - Disable device, e.g. -d 13fe:c302
                 The driver detects and enables all supported PCI CAN-bus
                 devices on the bus. However, if you want the driver to ignore
                 a particular device use this option.
    -e vid:did,cap
               - Disable PCIe capability cap for device, e.g. -e 13fe:00d7,0x05
    -b delay   - Bus-off recovery delay timer length (milliseconds).
                 If set to 0ms, then the bus-off recovery is disabled!
                 The netif transmission queue fault recovery is also set to this
                 delay value.
                 Default: 50ms
    -x         - Start the driver with extended MIDs enabled.
                 Device suboptions take precedence over this option.
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

If multiple supported cards are installed, all supported cards will be
automatically loaded. To override this behaviour and manually specify a device
to ignore, find out the vendor ID (vid) and device ID (did) of the card. To do
this run the command:

    pci-tool -v

An example output looks like this:

    B000:D05:F00 @ idx 7
            vid/did: 13fe/c302
                    <vendor id - unknown>, <device id - unknown>
            class/subclass/reg: 0c/09/00
                    CANbus Serial Bus Controller

    B000:D06:F00 @ idx 8
            vid/did: 10e8/8406
                    <vendor id - unknown>, <device id - unknown>
            class/subclass/reg: ff/00/00
                    Unknown Class code

In this example, say we would like to disable the card with numbers
vid/did: 13fe/c302

Target specific hardware to disable and enable max verbose mode for debugging:

    dev-can-linux -d 13fe:c302 -vv -ll


## Configuration

Once the driver is up and running, to configure the CAN-bus channels the QNX
tool canctl is used.

Firstly, when you start the driver the supported number of channels will have
associated file descriptors created. For example, for the dual channel Advantech
PCI devices we will have /dev/can0/rx0, /dev/can0/tx0, /dev/can1/rx0 and
/dev/can1/tx0 appear.


### Setting Bitrate

Normally the configurations are done using canctl command, however they can also
be done at driver startup time, see [Usage](#usage) section.

For example, to set the bitrate:

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

For special applications that need to manually set the BTR0 and BTR1 registers
of the SJA1000 chipset use the baud-rate suboptions at driver startup; see
[Usage](#usage) section `-u` option for detailed description. Note the other
suboptions (bprm, ts1, ts2 and sjw) will be ignored when setting btr0 and btr1.
Suboption freq must be specified and will be taken and trusted verbatim.


## Sending and Receiving Test Messages

The candump tool can be used to send and receive test messages also. To listen
to a channel:

    candump -u0,rx0 &

Then to send to the same channel, use cansend tool:

    cansend -u0,tx0 -m0x1234,1,0xABCD

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
[Emulation (workspace/emulation/qnx710)](https://github.com/Deniz-Eren/workspace/tree/main/emulation/qnx710).


## Building

Refer to the
[Development (workspace/dev)](https://github.com/Deniz-Eren/workspace/tree/main/dev)
documentation for details on how to setup the development container.

Within the development container, to build (default is for x86_64 platform):

    cd dev-can-linux
    mkdir build ; cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF ..
    cpack

Other build types you can indicate are Debug, Coverage and Profiling.

To build for aarch64le architecture:

    cd dev-can-linux
    mkdir build ; cd build
    cmake -DCMAKE_TOOLCHAIN_FILE=../workspace/cmake/Toolchain/qnx710-aarch64le.toolchain.cmake \
          -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF ..
    cpack

The following installer files will be created:

    dev-can-linux-1.0.0-qnx710-[architecture][build type].tar.gz
    dev-can-linux-1.0.0-qnx710-dev.tar.gz

Where the "[architecture]" is the target architecture, for example x86_64 or
aarch64le, "[build type]" is empty for Release, "-g" for Debug, "-cov" for
Coverage and "-pro" for Profiling.

The '-dev' variant contains the application development headers used to develope
software that talks to dev-can-linux driver. You do not need to install this on
the target system, but rather your development environment if you are developing
an application that needs to talk to dev-can-linux channels.

From CMake you can also run the tests. First start the emulation environment and
copy/run the dev-can-linux release driver, then from the development environment:

    cd dev-can-linux
    . tests/driver/common/env/dual-channel.env
    rm -rf build ; mkdir build ; cd build
    cmake -DCMAKE_BUILD_TYPE=Coverage -DBUILD_TESTING=ON ..
    ctest

Because we are cross-compiling in CMake, we can only run the tests on a QNX
target. The CMake project is configured to talk to our QEmu hardware emulation
over SSH. You must make sure this emulator has been started up before running
ctest. To start the emulator check documentation
[Emulation (workspace/emulation/qnx710)](https://github.com/Deniz-Eren/workspace/tree/main/emulation/qnx710).


## Example Applications

If you are interested in developing applications that utilize CAN-bus through
our driver, then take a look at the [cansend](tools/cansend),
[candump](tools/candump) and [canread](tools/canread) applications. These double
up as excellent testing tools and examples of applications that talk to our
driver.

Note also that together with our driver installer package a '-dev' variant
installer is also packaged. This contains the necessary C headers to develop
applications, intended to be installed onto your development environment.


## How Does It Work?

When you start the driver a number of device RX/TX file descriptors are created
that facilitate application interfaces to the driver.

Diagram below illustrates device folders and RX/TX file descriptors and how to
configure them.

    dev-can-linux -u id=0,rx=n,tx=m,x   # Example driver start command
    │                                   # Asking for n rx and m tx channels
    │                                   # for port 0.
    |                                   # `x` specifies extended MIDs to be used
    |                                   # read/write, this is ignored for devctl
    |                                   # functionality.
    │                                   # Note n and m can be zero also.
    │
    ├── /dev/can0/  # When you start the driver these device directories
    │   │           # appear in /dev; can0, can1, etc, as many as there
    │   │           # are physical CAN ports on the device you are using.
    │   │           # In this example diagram we illustrate with a 2 port
    │   │           # device.
    │   │
    │   ├── rx0     # Has message ID filter configurable through canctl or
    │   │   │       # via software functions in dev-can-linux/commands.h
    │   │   │       # Filter applies to both character IO and raw CAN messages.
    │   │   │
    │   │   ├── client connection to read  1    e.g. canread or candump
    │   │   ├── client connection to read  2         or custom application
    │   │   ├── ...
    │   │   └── "      "          "  "     N
    │   │
    │   ├── tx0     # Has configurable message ID for character mode IO
    │   │   │       # transmission. This ID isn't applicable or RAW mode.
    │   │   │
    │   │   ├── client connection to write 1    e.g. cansend or custom
    │   │   ├── client connection to write 2         application
    │   │   ├── ...
    │   │   └── "      "          "  "     N
    │   │
    │   ├── rx1
    │   │   ├── client connection to read  1    RX channels can't be opened to
    │   │   ├── client connection to read  2    write; you'll get an error
    │   │   ├── ...
    │   │   └── "      "          "  "     N
    │   ├── tx1
    │   │   ├── client connection to write 1    TX channels can't be opened to
    │   │   ├── ...                             read; you'll get an error
    │   │   └── "      "          "  "     N
    │   │
    │   ├── rx{n-1} # As example driver start command had -u0,rx{n},... there
    │   │   │       # will be "n-1" RX file descriptors created, all with
    │   │   │       # their own configurable message ID filter.
    │   │   │
    │   │   ├── client connection to read  1
    │   │   ├── ...
    │   │   └── "      "          "  "     N
    │   │
    │   └── tx{m-1} # As example driver start command had -u0,tx{m},... there
    │       │       # will be "m-1" TX file descriptors created, all with
    │       │       # their own configurable transmit message ID.
    │       │
    │       ├── client connection to write 1
    │       ├── ...
    │       └── "      "          "  "     N
    │
    └── /dev/can1/  Second device associated to second board port.
        │           Example driver start command didn't specify anything for
        │           this device, so the default behaviour of creating a single
        │           RX and TX file decriptor will be performed.
        |           This channel will have the default standard MIDs for read
        |           and write functionality; not applicable for devctl or direct
        |           send/receive operations.
        │
        ├── rx0
        │   ├── client connection to read  1
        │   ├── client connection to read  2
        │   ├── ...
        │   └── "      "          "  "     N
        └── tx0
            ├── client connection to write 1
            ├── client connection to write 2
            ├── ...
            └── "      "          "  "     N

Each RX file descriptor can have any number of _client connection to read_ and
they will all have their own receive queues and they will all receive all the
available messages. Each have the possibility to be configured independently.

All TX file descriptors transmit to the same underlying device port and
similarly can also be configured independently.

All RX file descriptor clients have the possibility of receiving the TX echo
back messages (unless they have their filters configured to mask them).


## Note about MIDs

Message IDs or MIDs are slightly different on QNX compared to Linux. The form of
the ID depends on whether or not the driver is using extended MIDs:

- In standard 11-bit MIDs, bits 18–28 define the MID.
- In extended 29-bit MIDs, bits 0–28 define the MID.


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


## PCIe MSI and MSI-X Capability Devices

For PCIe devices that support capability 0x05 (MSI) and/or 0x11 (MSI-X), both
`pci-server` driver and `dev-can-linux` driver needs to have the environment
variable `PCI_CAP_MODULE_DIR` defined.

To check if your device supports this capability, just run the driver with high
verbose configurations `dev-can-linux -vvvvv` and you should see the following
during the detection process:

    read capability[#]: 0x05
    read capability[#]: 0x11

For the `dev-can-linux` driver you can simply define this variable in your
console, however for `pci-server` driver it is usually defined in the image.
Furthermore, your environment must contain the PCIe module shared libraries
`pci_cap-0x05.so*` and/or `pci_cap-0x11.so*` installed.

As an example take a look at our QNX 7.1 emulation image setup scripts within the
[workspace](https://github.com/Deniz-Eren/workspace) submodule.

The scripts you should check are, firstly
[workspace/emulation/qnx710/image/parts/ifs.build](https://github.com/Deniz-Eren/workspace/blob/main/emulation/qnx710/image/parts/ifs.build)
defines file `/proc/boot/pci_server.cfg` embedded in the image contains the
environment variable `PCI_CAP_MODULE_DIR`, specifying where the capability
modules are located for `pci-server` driver to find them:

    [buscfg]
    DO_BUS_CONFIG=no

    [envars]
    PCI_CAP_MODULE_DIR=/proc/boot/lib/dll/pci/
    PCI_DEBUG_MODULE=pci_debug2.so
    PCI_HW_MODULE=pci_hw-Intel_x86.so

You can also see in the `ifs.build` file the PCIe 0x05 (MSI) and/or 0x11 (MSI-X)
capability module dynamic libraries are installed in the `lib/dll/pci/` path
which mounts to `/proc/boot/lib/dll/pci/`

    lib/dll/pci/pci_cap-0x05.so=lib/dll/pci/pci_cap-0x05.so
    lib/dll/pci/pci_cap-0x05.so.2.3=lib/dll/pci/pci_cap-0x05.so.2.3
    lib/dll/pci/pci_cap-0x05.so.2.3.sym=lib/dll/pci/pci_cap-0x05.so.2.3.sym
    lib/dll/pci/pci_cap-0x11.so=lib/dll/pci/pci_cap-0x11.so
    lib/dll/pci/pci_cap-0x11.so.2.2=lib/dll/pci/pci_cap-0x11.so.2.2
    lib/dll/pci/pci_cap-0x11.so.2.2.sym=lib/dll/pci/pci_cap-0x11.so.2.2.sym

Next in file
[workspace/emulation/qnx710/image/parts/system.build](https://github.com/Deniz-Eren/workspace/blob/main/emulation/qnx710/image/parts/system.build)
we define the same environment variable for dev-can-linux to find the PCI
modules:

    etc/profile = {
    export TERM=qansi
    export PATH=/opt/bin:/proc/boot:/system/xbin
    export LD_LIBRARY_PATH=/opt/lib:/proc/boot:/system/lib:/system/lib/dll
    export PCI_CAP_MODULE_DIR=/proc/boot/lib/dll/pci/
    export SYSNAME=QNXTEST
    export TZ=AEST-10
    export PS1=\[$SYSNAME]\#
    }

This shows the file `etc/profile` defining the needed environment variables so
that the user console have them defined for dev-can-linux to find the PCI
modules.

For advanced user, if you wish to disable the 0x11 (MSI-X) capability to force
the driver to use 0x05 (MSI) capability instead (if it's available) or if you
wish to disable both, simply specific device and capability number with `-e`
option program options to the `dev-can-linux` driver.

To force MSI to be used, disable MSI-X:

    dev-can-linux -e vid:did,0x11

To force a regular IRQ to be used disable both MSI and MSI-X:

    dev-can-linux -e vid:did,0x11 -e vid:did,0x05

Note it is NOT recommended to disable capabilities, however this ability is
provided for advanced users to adopt at their discretion.


## Hardware Test Status

Actual hardware tested currently are:

- Advantech PCIe (vendor: 13fe, device: c302) running on UNO2484G and UNO1483G

Please report back findings from any hardware you test so we can update this
list, your help is much appreciated.
