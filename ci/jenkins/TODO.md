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

This command runs valgrind successfully:

    valgrind --tool=memcheck --xml=yes --xml-file=vmemcheck.xml \
        /opt/bin/dev-can-linux -vvvvv

