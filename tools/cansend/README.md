# CANSEND (DEV-CAN-LINUX)

CANSEND is an accompanying tool used to send raw CAN messages.

## Usage

The compiled program is used as follows:

    cansend [options]

Options:

    -u subopts - Specify the device TX file descriptors.

                 Suboptions (subopts):

                 #      - Specify ID number of the device to configure;
                          e.g. /dev/can0/ is -u0
                 tx#    - ID of TX file descriptors to connect to

                 Example:
                     cansend -u0,tx0 ...

    -m subopts - Specify message details.

                 Suboptions (subopts):

                 #      - MID of message in hexadecimal
                 #      - Standard (0) or Extended (1) MID
                 #      - Message data in hexadecimal

                 Example:
                     cansend -m0x1234,1,0xABCD ...

    -w         - Print warranty message and exit.
    -c         - Print license details and exit.
    -?/h       - Print help menu and exit.
