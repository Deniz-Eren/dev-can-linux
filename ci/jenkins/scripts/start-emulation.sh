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

optd="mioe3680_pci"     # default device selection
opti=""

while getopts d:i: opt; do
    case ${opt} in
    d )
        optd=$OPTARG
        ;;
    i )
        opti=$OPTARG
        ;;
    \?)
        echo "Usage: integration-testing.sh [options]"
        echo "  -d device selection (default: mioe3680_pci)" 
        echo "  -i local file full path to store images"
        echo ""
        exit
        ;;
    esac
done

docker run -d --name=qemu_env \
    --network host \
    --device=/dev/kvm \
    localhost/dev-can-linux-qemu tail -f /dev/null

docker cp $opti qemu_env:/root/Images

docker exec -d --user root --workdir /root qemu_env \
    qemu-system-x86_64 \
        -k en-us \
        -drive id=disk,file=/root/Images/disk-raw,format=raw,if=none \
        -device ahci,id=ahci \
        -device ide-hd,drive=disk,bus=ahci.1 \
        -boot d \
        -object can-bus,id=c0 \
        -object can-bus,id=c1 \
        -device $optd,canbus0=c0,canbus1=c1 \
        -object can-host-socketcan,id=h0,if=c0,canbus=c0,if=vcan0 \
        -object can-host-socketcan,id=h1,if=c1,canbus=c1,if=vcan1 \
        -m size=4096 \
        -nic user,hostfwd=tcp::6022-:22,hostfwd=tcp::8000-:8000 \
        -smp 4 \
        -enable-kvm \
        -nographic
