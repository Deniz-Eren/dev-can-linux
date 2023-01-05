# Emulation

Before you can start the emulation, you must make sure you have built the
CANTEST QNX image; see section [Test Platform](../image/). The emulator VM boots
that image up.

To start the QEmu VM with CAN-bus hardware emulation, first ensure the needed
host Linux modules are running:

    cd dev-can-linux/tests/emulation/
    sudo ./setuphost.sh

This will startup some Linux modules; one of which is KVM. For KVM to work you
must have hardware virtualisation settings enabled in you BIOS.

Next ensure your host will accept X session from the Docker container using the
server access control command:

    xhost +

Now we can startup the emulation using Docker-Compose:

    cd dev-can-linux/tests/emulation/
    docker-compose up -d

QEmu window should appear.
