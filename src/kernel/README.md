# Linux Kernel

Copied (and modification kept to a minimum) source files from the Linux Kernel
source repository are located here; the same directory structure and file names
are preserved.

To check which version of the Linux Kernel the driver is currently harmonized
with, check config file
[config/HARMONIZED_LINUX_VERSION](https://github.com/Deniz-Eren/dev-can-linux/blob/main/config/HARMONIZED_LINUX_VERSION).

## Support

Current version aims to support all Linux PCI SJA1000 drivers for QNX 8.0 and
QNX 7.1 RTOS, for x86_64 and aarch64le target platforms. Additionally QNX 7.1
support exists for armv7le target platform.

## Aim

### Comparability

One major aim of this project is to make sure ongoing updates to the Linux
Kernel can be propagated easily. The idea is to make it as easy as possible to
compare, check or make modifications by simply running the
[Meld](https://meldmerge.org/) command:

    meld ~/_path-to-repos_/linux/ ~/_path-to-repos_/dev-can-linux/src/kernel/

Where, _path-to-repos_ is where you store your cloned working repositories and
paths to _/linux/_ and _/dev-can-linux/_ are where you have cloned the Linux
Kernel and dev-can-linux repositories.

Note, in Meld GUI make sure you *uncheck* the "New" button so that all the files
missing from the Linux Kernel and vice versa are not shown; otherwise the
results get cluttered with irrelevant differences.


### Testability

Testability through automated Continuous Integration is very important to the
ongoing updates to the Linux Kernel we propagate to this project. For this
reason we have setup everything using Docker-compose containerization and have
made Jenkins CI pipelines fundamental to our workflows.

See [Jenkins Pipeline (workspace/ci/jenkins)](https://github.com/Deniz-Eren/workspace/tree/main/ci/jenkins).

Another consideration is that testing drivers often means we must test using
hardware. For this reason, we have adopted the use of QEmu virtualisation, which
has emulated CAN-bus hardware configuration options.

See [QEmu CAN-bus Hardware Emulation (workspace/emulation/qnx710)](https://github.com/Deniz-Eren/workspace/tree/main/emulation/qnx710).

An added challenge posed by implementing for QNX means we are developing in
Linux, then cross-compiling and testing on QNX. For this reason, we have
embedded our test image setup for our QNX target environment into our workflow.
That is, Jenkins pipelines creates the QNX target image, then starts the QEmu
hardware emulation and then runs our tests over local SSH and presents the
results; complete integration, together with extreme ease of setup thanks to the
entire setup being done in containers.

See [Test Platform (workspace/emulation/qnx710/image)](https://github.com/Deniz-Eren/workspace/tree/main/emulation/qnx710/image).


## Structure

To read the file chart presented in text format below, please interpret the
words "items", "replaced", "added" and "unchanged" in the context of the text in
those files, as follows:

- Items: can be structures and/or functions.
- Replaced: original names, arguments and member variables (or just their names)
have been kept the same but their contents and functionality has been completed
replaced with QNX application specific functionality.
- Added: completely new structure and/or functions added.
- Unchanged: left as-is from the original Linux Kernel source-code; there could
be minor cosmetic changes, which can be reviewed using the documented _Meld_
comparison process.
- Removed: items not applicable or needed; completely removed and not replaced
and no new items added in it's place.

To elaborate, this is a textual analysis only, intended to provide some guidance
on how to rebase changes from the Linux Kernel and what where to focus.
Otherwise the actual functionality of timers, delays, hardware interface
function calls, etc, have all been redirected to QNX equivalent functions;
e.g. "unchanged" refers to the textual contents of the file being unchanged.

File modification chart:

    src/kernel/
    ├── arch
    │   └── x86
    │       └── include
    │           └── asm
    │               ├── div64.h <------------ Unchanged
    │               └── io.h <--------------- Unchanged
    ├── drivers
    │   └── net
    │       └── can
    │           ├── dev
    │           │   ├── bittiming.c <-------- Unchanged
    │           │   ├── calc_bittiming.c <--- Unchanged
    │           │   ├── dev.c <-------------- Replaced all schedule delayed work
    │           │   │                         implementation.
    │           │   │                         All CAN network device mallocs are
    │           │   │                         modified.
    │           │   │                         GPIO Termination functionality
    │           │   │                         removed for now.
    │           │   ├── netlink.c <---------- Only can_changelink remains.
    │           │   │                         TODO: can_fill_xstats
    │           │   └── skb.c <-------------- Heavily modified / reduced.
    │           └── sja1000
    │               ├── adv_pci.c <---------- Added driver (not from Linux)
    │               ├── ems_pci.c <---------- Unchanged
    │               ├── kvaser_pci.c <------- Unchanged
    │               ├── peak_pci.c <--------- Unchanged
    │               ├── plx_pci.c <---------- Unchanged
    │               ├── sja1000.c <---------- Only spinlock and priv data have
    │               └── sja1000.h <---------- been replaced.
    ├── include
    │   ├── asm-generic
    │   │   ├── bitops
    │   │   │   ├── fls64.h <---------------- Unchanged
    │   │   │   └── __fls.h <---------------- Unchanged
    │   │   └── div64.h <-------------------- Unchanged
    │   ├── linux
    │   │   ├── bitops.h <------------------- Only kept fls_long()
    │   │   ├── bits.h <--------------------- Minor changes
    │   │   ├── build_bug.h <---------------- Minor changes
    │   │   ├── can
    │   │   │   ├── bittiming.h <------------ Unchanged
    │   │   │   ├── dev.h <------------------ Some removed
    │   │   │   ├── length.h <--------------- Unchanged
    │   │   │   ├── platform
    │   │   │   │   └── sja1000.h <---------- Unchanged
    │   │   │   └── skb.h <------------------ Changed to struct sk_buff
    │   │   ├── compiler.h <----------------- Unchanged
    │   │   ├── compiler_types.h <----------- Kept only __PASTE macro
    │   │   ├── const.h <-------------------- Unchanged
    │   │   ├── delay.h <-------------------- Replaced with QNX udelay()
    │   │   ├── device.h <------------------- Reduced struct device to 1 member
    │   │   ├── err.h <---------------------- Minor changes
    │   │   ├── interrupt.h <---------------- request_irq() and free_irq() are
    │   │   │                                 only stubs for QNX implementation.
    │   │   ├── io.h <----------------------- Replaced all with QNX in*() and
    │   │   │                                 out*() functions.
    │   │   ├── irqreturn.h <---------------- Unchanged
    │   │   ├── kernel.h <------------------- Kept only includes of some headers
    │   │   ├── log2.h <--------------------- Unchanged
    │   │   ├── math64.h <------------------- Minor changes
    │   │   ├── math.h <--------------------- Unchanged
    │   │   ├── minmax.h <------------------- Minor changes
    │   │   ├── mod_devicetable.h <---------- Only kept struct pci_device_id
    │   │   ├── module.h <------------------- Turning macros MODULE_*() to null
    │   │   ├── netdevice.h <---------------- Heavily reduced and key functions
    │   │   │                                 that are needed turned into stubs
    │   │   │                                 that are reimplemented in QNX.
    │   │   ├── pci.h <---------------------- Heavily reduced
    │   │   ├── pci_ids.h <------------------ Unchanged
    │   │   ├── skbuff.h <------------------- 2 items replaced; others removed
    │   │   ├── slab.h <--------------------- All removed; using malloc()/free()
    │   │   ├── stddef.h <------------------- Unchanged
    │   │   ├── types.h <-------------------- All removed; project types added
    │   │   └── units.h <-------------------- Unchanged
    │   ├── uapi
    │   │   └── linux
    │   │       ├── can
    │   │       │   ├── error.h <------------ Unchanged
    │   │       │   └── netlink.h <---------- Unchanged
    │   │       ├── can.h <------------------ Unchanged
    │   │       ├── const.h <---------------- Unchanged
    │   │       ├── if.h <------------------- Only kept enum net_device_flags
    │   │       ├── pci.h <------------------ Minor changes to prevent clash
    │   │       └── pci_regs.h <------------- Unchanged
    │   └── vdso
    │       ├── bits.h <--------------------- Unchanged
    │       ├── const.h <-------------------- Unchanged
    │       └── time64.h <------------------- Unchanged
    └── README.md <-------------------------- This document.

