# Linux Kernel

Copied (and modification kept to a minimum) source files from the Linux Kernel
source repository are located here; the same directory structure and file names
are preserved.

## Support

Current version aims to support all Linux PCI SJA1000 drivers for QNX 7.1 RTOS.

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

See [Test Platform](../../tests/image/).

### Integrity

### Minimalist

### Explicitness

## Structure

To read the file chart presented in text format below, please interpret the words "items", "replaced", "added" and "unchanged" in the context of the text in those files, as follows:

- Items: can be structures and/or functions.
- Replaced: original names, arguments and member variables (or just their names) have been kept the same but their contents and functionality has been completed replaced with QNX application specific functionality.
- Added: completely new structure and/or functions added.
- Unchanged: left as-is from the original Linux Kernel source-code; there could be minor cosmetic changes, which can be reviewed using the documented _Meld_ comparison process.
- Removed: items not applicable or needed; completely removed and not replaced and no new items added in it's place.

To elaborate, this is a textual analysis only, intended to provide some guidance on how to rebase changes from the Linux Kernel and what where to focus. Otherwise the actual functionality of timers, delays, hardware interface function calls, etc, have all been redirected to QNX equivalent functions; e.g. "unchanged" refers to the textual contents of the file being unchanged.

File modification chart:

    src/kernel/
    ├── arch
    │   └── x86
    │       └── include
    │           └── asm
    │               └── div64.h <------
    ├── drivers
    │   └── net
    │       └── can
    │           ├── dev.c <------------
    │           └── sja1000
    │               ├── adv_pci.c <---- added driver (not from Linux)
    │               ├── ems_pci.c <---- unchanged
    │               ├── kvaser_pci.c <- unchanged
    │               ├── peak_pci.c <--- unchanged
    │               ├── plx_pci.c <---- unchanged
    │               ├── sja1000.c <----
    │               └── sja1000.h <----
    ├── include
    │   ├── asm-generic
    │   │   ├── bitops
    │   │   │   ├── fls64.h <----------
    │   │   │   └── __fls.h <----------
    │   │   └── div64.h <--------------
    │   ├── linux
    │   │   ├── bitops.h <-------------
    │   │   ├── can
    │   │   │   ├── dev.h <------------
    │   │   │   ├── led.h <------------
    │   │   │   ├── platform
    │   │   │   │   └── sja1000.h <----
    │   │   │   └── skb.h <------------
    │   │   ├── compiler.h <-----------
    │   │   ├── delay.h <--------------
    │   │   ├── device.h <-------------
    │   │   ├── interrupt.h <----------
    │   │   ├── io.h <-----------------
    │   │   ├── irqreturn.h <----------
    │   │   ├── kernel.h <-------------
    │   │   ├── log2.h <---------------
    │   │   ├── mod_devicetable.h <----
    │   │   ├── module.h <-------------
    │   │   ├── netdevice.h <----------
    │   │   ├── pci.h <----------------
    │   │   ├── pci_ids.h <------------
    │   │   ├── skbuff.h <------------- 2 items replaced; others removed
    │   │   ├── slab.h <---------------
    │   │   └── types.h <-------------- all removed; project types added
    │   └── uapi
    │       └── linux
    │           ├── can
    │           │   ├── error.h <------
    │           │   └── netlink.h <----
    │           ├── can.h <------------
    │           ├── if.h <-------------
    │           ├── pci.h <------------
    │           └── pci_regs.h <-------
    ├── net
    │   └── core
    │       └── skbuff.c <-------------
    └── README.md <-------------------- This document.


## Method
