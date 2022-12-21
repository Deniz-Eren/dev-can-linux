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
optk="dev-can-linux"    # default slay command target
optr="10"               # default run duration
opts=""
optt="Release"          # default build type
optv=""

while getopts b:k:r:s:t:v opt; do
    case ${opt} in
    b )
        optb=$OPTARG
        ;;
    k )
        optk=$OPTARG
        ;;
    r )
        optr=$OPTARG
        ;;
    s )
        opts=$OPTARG
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
        echo "  -k what process to slay when stopping testing"
        echo "     (default: dev-can-linux)"
        echo "  -r run duration in seconds (default: 10)"
        echo "  -s if not empty then copy debug symbols for libc.so"
        echo "  -t build type (default: Release)" 
        echo "  -v for verbose mode"
        echo " Environment variable QNX_PREFIX_CMD is placed as prefix to"
        echo " running dev-can-linux executable"
        echo ""
        exit
        ;;
    esac
done

docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && /root/dev-can-linux/dev/setup-profile.sh \
    && mkdir -p $optb \
    && cd $optb \
    && cmake -DCMAKE_BUILD_TYPE=$optt \
        -DDISABLE_COVERAGE_HTML_GEN=ON \
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
        \"tar -xf $optb/dev-can-linux-*.tar.gz -C /opt/\""

if [ ! -z "$opts" ]; then
    docker exec --user root --workdir /root dev_env bash -c \
        "sshpass -p 'root' ssh \
            -o 'StrictHostKeyChecking=no' \
            -o 'UserKnownHostsFile=/dev/null' \
            -o 'LogLevel=ERROR' \
            -p6022 root@localhost \
            \"cp /proc/boot/libc.so.5* /opt/lib/ ; \
                ln -s /opt/bin/dev-can-linux \
                    /opt/bin/dev-can-linux$opts \""
fi

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
