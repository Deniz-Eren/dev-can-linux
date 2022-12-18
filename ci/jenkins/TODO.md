# TODO Notes

Items in this file are here to remind about items yet to be done or technical
notes that need to be remembered for items that haven't been implemented yet.
Once implemented the notes will either be in comments or README.md files and
should be removed from this file.


## Valgrind

Valgrind (perhaps on QNX) has many strange issues. One of them is that the
debug symbols for libc.so.5 is required, which is named libc.so.5.sym. Both of
these libraries must be located in the same location.
Unsuccessful testing was done with program option _--extra-debuginfo-path=_
with no detectable behaviour change.
Also the system isn't happy with these two files being located in location
_/proc/boot_ for some reason, perhaps because it's not a mounted disk location.
This is true even though both _PATH_ and _LD_LIBRARY_PATH_ contains
_/proc/boot_:

    echo $PATH
    /opt/bin:/proc/boot:/system/xbin
    
    echo $LD_LIBRARY_PATH
    /opt/lib:/proc/boot:/system/lib:/system/lib/dll

To work around this we've made copies of these libraries to disk on the location
_/opt/lib_:

    cp /proc/boot/libc.so.5* /opt/lib/

This command starts up before crashing:

    valgrind --tool=memcheck --xml=yes --xml-file=vmemcheck.xml \
        /opt/bin/dev-can-linux -vvvvv

The crash presents:

    vex amd64->IR: unhandled instruction bytes: 0xFA 0xBA 0x1 0x0 0x0 0x0 0x87 0x10 0x85 0xD2
    vex amd64->IR:   REX=0 REX.W=0 REX.R=0 REX.X=0 REX.B=0
    vex amd64->IR:   VEX=0 VEX.L=0 VEX.nVVVV=0x0 ESC=NONE
    vex amd64->IR:   PFX.66=0 PFX.F2=0 PFX.F3=0
    ==778262== valgrind: Unrecognised instruction at address 0x10ad5b.
    ==778262== Your program just tried to execute an instruction that Valgrind
    ==778262== did not recognise.  There are two possible reasons for this.
    ==778262== 1. Your program has a bug and erroneously jumped to a non-code
    ==778262==    location.  If you are running Memcheck and you just saw a
    ==778262==    warning about a bad jump, it's probably your program's fault.
    ==778262== 2. The instruction is legitimate but Valgrind doesn't handle it,
    ==778262==    i.e. it's Valgrind's fault.  If you think this is the case or
    ==778262==    you are not sure, please let us know and we'll try to fix it.
    ==778262== Either way, Valgrind will now raise a SIGILL signal which will
    ==778262== probably kill your program.
    Illegal instruction

It turned out to be missing _#if CONFIG_QNX_INTERRUPT_ATTACH_EVENT != 1_ in
_/src/fixed.c_. Now we have crash saying _Memory fault_ with core dump to be
investigated.
