# Test Platform

To build the CANTEST image, first ensure you have setup your
[Development](../../dev/) environment.

Then login to your development environment:

    podman exec --interactive --tty --user root \
        --workdir /root qnx710-dev /bin/bash --login

Next navigate to the test image folder and run:

    cd dev-can-linux/tests/image
    ./builddisk.sh

The image file _output/disk-raw_ should be created.
