#!/bin/bash
#
# \file     integration-testing.sh
# \brief    Bash script for integration testing.
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

docker exec -d --user root --workdir /root qemu_env \
    qemu-system-x86_64 \
        -k en-us \
        -drive id=disk,file=/root/Images/disk-raw,format=raw,if=none \
        -device ahci,id=ahci \
        -device ide-hd,drive=disk,bus=ahci.1 \
        -boot d \
        -object can-bus,id=c0 \
        -object can-bus,id=c1 \
        -device mioe3680_pci,canbus0=c0,canbus1=c1 \
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
            -p6022 root@localhost \"uname -a\" \
            2> /dev/null; \
            do sleep 1; done"

docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && /root/dev-can-linux/dev/setup-profile.sh \
    && mkdir -p /data/home/root \
    && cd /data/home/root \
    && mkdir -p build_qcc_coverage \
    && cd build_qcc_coverage \
    && cmake -DCMAKE_BUILD_TYPE=Coverage \
        /root/dev-can-linux \
    && make -j8 \
    && cpack"

# Copy build directory to the QNX VM
docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && sshpass -p 'root' scp -o 'StrictHostKeyChecking=no' \
        -r -P6022 /data/home/root/build_qcc_coverage \
        root@localhost:/data/home/root/"

docker exec --user root --workdir /root dev_env bash -c \
    "sshpass -p 'root' ssh -o 'StrictHostKeyChecking=no' \
        -p6022 root@localhost \
        \"tar -xf /data/home/root/build_qcc_coverage/dev-can-linux-*-cov.tar.gz \
            -C /opt/\""

docker exec -d --user root --workdir /root dev_env bash -c \
    "sshpass -p 'root' ssh -o 'StrictHostKeyChecking=no' \
        -p6022 root@localhost \
        \"dev-can-linux -vvvvv\""

sleep 10

docker exec --user root --workdir /root dev_env bash -c \
    "sshpass -p 'root' ssh -o 'StrictHostKeyChecking=no' \
        -p6022 root@localhost \
        \"slay -s SIGINT dev-can-linux\" || (exit 0)"

# Copy build directory back from the QNX VM
docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && sshpass -p 'root' scp -o 'StrictHostKeyChecking=no' \
        -r -P6022 \
        root@localhost:/data/home/root/build_qcc_coverage \
        /data/home/root/"

docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && /root/dev-can-linux/dev/setup-profile.sh \
    && cd /data/home/root/build_qcc_coverage \
    && lcov --gcov-tool=\$QNX_HOST/usr/bin/ntox86_64-gcov \
        -t \"test_coverage_results\" -o tests.info \
        -c -d /data/home/root/build_qcc_coverage \
        --base-directory=/root/dev-can-linux \
        --no-external --quiet \
    && genhtml -o /data/home/root/build_qcc_coverage/cov-html \
        tests.info --quiet"

rm -rf $WORKSPACE/test-coverage-qcc

docker cp \
    dev_env:/data/home/root/build_qcc_coverage/cov-html \
        $WORKSPACE/test-coverage-qcc
