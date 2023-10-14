# CANREAD (DEV-CAN-LINUX)

CANREAD is an accompanying tool used to read textual bytes from CAN messages.

## Usage

The compiled program is used as follows:

    canread [options]

Options:

    -u subopts - Specify the device RX file descriptors.

                 Suboptions (subopts):

                 #      - Specify ID number of the device to configure;
                          e.g. /dev/can0/ is -u0
                 rx#    - ID of RX file descriptors to connect to

                 Example:
                     canread -u0,rx0

    -n bytes   - Number of bytes to receive before flushing to stdout.
    -w         - Print warranty message and exit.
    -c         - Print license details and exit.
    -?/h       - Print help menu and exit.
