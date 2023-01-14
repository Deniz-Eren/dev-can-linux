#!/bin/bash
#
# \file     start-emulation.sh
# \brief    Bash script for Jenkins integration testing.
#
# Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

SSH_PORT="6022"     # default SSH port number

while getopts i:p: opt; do
    case ${opt} in
    i )
        IMAGE_DIR=$OPTARG
        ;;
    p )
        SSH_PORT=$OPTARG
        ;;
    \?)
        echo "Usage: start-emulation.sh [options]"
        echo "  -i local file full path to store images"
        echo "  -p ssh port number"
        echo ""
        exit
        ;;
    esac
done

docker run -d --name=qemu_env \
    --network host \
    --device=/dev/kvm \
    localhost/dev-can-linux-qemu tail -f /dev/null

docker cp $IMAGE_DIR qemu_env:/root/Images

docker exec -d --user root --workdir /root qemu_env \
    qemu-system-x86_64 \
        -k en-us \
        -drive id=disk,file=/root/Images/disk-raw,format=raw,if=none \
        -device ahci,id=ahci \
        -device ide-hd,drive=disk,bus=ahci.1 \
        -boot d \
        -object can-bus,id=c0 \
        -object can-bus,id=c1 \
        -object can-bus,id=c2 \
        -device mioe3680_pci,canbus0=c0,canbus1=c1 \
        -device kvaser_pci,canbus=c2 \
        -object can-host-socketcan,id=h0,if=c0,canbus=c0,if=vcan1000 \
        -object can-host-socketcan,id=h1,if=c1,canbus=c1,if=vcan1001 \
        -object can-host-socketcan,id=h2,if=c2,canbus=c2,if=vcan1002 \
        -m size=4096 \
        -nic user,hostfwd=tcp::$SSH_PORT-:22 \
        -smp 2 \
        -enable-kvm \
        -nographic

# Wait until emulator SSH port is ready
docker exec --user root --workdir /root dev_env \
    bash -c "source .profile \
        && until sshpass -p 'root' ssh \
            -o 'StrictHostKeyChecking=no' \
            -o 'UserKnownHostsFile=/dev/null' \
            -o 'LogLevel=ERROR' \
            -p$SSH_PORT root@localhost \"uname -a\" \
            2> /dev/null; \
            do sleep 1; done"
