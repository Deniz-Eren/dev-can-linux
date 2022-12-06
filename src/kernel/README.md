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

    src/kernel/
    ├── arch
    │   └── x86
    │       └── include
    │           └── asm
    │               ├── bug.h <--------
    │               └── div64.h <------
    ├── drivers
    │   └── net
    │       └── can
    │           ├── dev.c <------------
    │           └── sja1000
    │               ├── adv_pci.c <----
    │               ├── ems_pci.c <----
    │               ├── kvaser_pci.c <-
    │               ├── peak_pci.c <---
    │               ├── plx_pci.c <----
    │               ├── sja1000.c <----
    │               └── sja1000.h <----
    ├── include
    │   ├── asm-generic
    │   │   ├── bitops
    │   │   │   ├── fls64.h <----------
    │   │   │   └── __fls.h <----------
    │   │   ├── bug.h <----------------
    │   │   └── div64.h <--------------
    │   ├── linux
    │   │   ├── bitops.h <-------------
    │   │   ├── bug.h <----------------
    │   │   ├── can
    │   │   │   ├── dev.h <------------
    │   │   │   ├── led.h <------------
    │   │   │   ├── platform
    │   │   │   │   └── sja1000.h <----
    │   │   │   └── skb.h <------------
    │   │   ├── compiler.h <-----------
    │   │   ├── delay.h <--------------
    │   │   ├── device.h <-------------
    │   │   ├── i2c-algo-bit.h <-------
    │   │   ├── i2c.h <----------------
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
    │   │   ├── skbuff.h <-------------
    │   │   ├── slab.h <---------------
    │   │   └── types.h <--------------
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
