# CANDUMP (DEV-CAN-LINUX)

CANDUMP is an accompanying tool used to read raw CAN messages.

## Usage

The compiled program is used as follows:

    candump [options]

Options:

    -u subopts - Specify the device RX file descriptors.

                 Suboptions (subopts):

                 #      - Specify ID number of the device to configure;
                          e.g. /dev/can0/ is -u0
                 rx#    - ID of RX file descriptors to connect to

                 Example:
                     candump -u0,rx0

    -w         - Print warranty message and exit.
    -c         - Print license details and exit.
    -?/h       - Print help menu and exit.

