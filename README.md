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
