#!/bin/bash
#
# \file     integration-testing.sh
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

optb=""
optd="mioe3680_pci"     # default device selection
opti=""
optk="dev-can-linux"    # default slay command target
optr="10"               # default run duration
optt="Release"          # default build type
optv=""

while getopts b:d:i:k:r:t:v opt; do
    case ${opt} in
    b )
        optb=$OPTARG
        ;;
    d )
        optd=$OPTARG
        ;;
    i )
        opti=$OPTARG
        ;;
    k )
        optk=$OPTARG
        ;;
    r )
        optr=$OPTARG
        ;;
    t )
        optt=$OPTARG
        ;;
    v )
        optv="-vvvvv"
        ;;
    \?)
        echo "Usage: integration-testing.sh [options]"
        echo "  -b build full path to use"
        echo "  -d device selection (default: mioe3680_pci)" 
        echo "  -i local file full path to store images"
        echo "  -k what process to slay when stopping testing"
        echo "     (default: dev-can-linux)"
        echo "  -r run duration in seconds (default: 10)"
        echo "  -t build type (default: Release)" 
        echo "  -v for verbose mode"
        echo " Environment variable QNX_PREFIX_CMD is placed as prefix to"
        echo " running dev-can-linux executable"
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

docker exec --user root --workdir /root dev_env \
    bash -c "source .profile \
        && until sshpass -p 'root' ssh \
            -o 'StrictHostKeyChecking=no' \
            -o 'UserKnownHostsFile=/dev/null' \
            -o 'LogLevel=ERROR' \
            -p6022 root@localhost \"uname -a\" \
            2> /dev/null; \
            do sleep 1; done"

docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && /root/dev-can-linux/dev/setup-profile.sh \
    && mkdir -p $optb \
    && cd $optb \
    && cmake -DCMAKE_BUILD_TYPE=$optt \
        /root/dev-can-linux \
    && make -j8 \
    && cpack"

# Copy build directory to the QNX VM
docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && sshpass -p 'root' scp \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -r -P6022 $optb \
        root@localhost:$optb"

docker exec --user root --workdir /root dev_env bash -c \
    "sshpass -p 'root' ssh \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -p6022 root@localhost \
        \"tar -xf $optb/dev-can-linux-*.tar.gz \
                -C /opt/ ; \
            cp /proc/boot/libc.so.5* /opt/lib/\""

docker exec -d --user root --workdir /root dev_env bash -c \
    "sshpass -p 'root' ssh \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -p6022 root@localhost \
        \"cd $optb ; $QNX_PREFIX_CMD dev-can-linux $optv\""

sleep $optr

docker exec --user root --workdir /root dev_env bash -c \
    "sshpass -p 'root' ssh \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -p6022 root@localhost \
        \"slay -s SIGINT $optk\" || (exit 0)"

# Copy build directory back from the QNX VM
docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && sshpass -p 'root' scp \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -r -P6022 \
        root@localhost:$optb \
        $optb/.."

docker stop qemu_env
docker rm qemu_env
