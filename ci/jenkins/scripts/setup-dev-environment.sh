#!/bin/bash
#
# \file     setup-dev-environment.sh
# \brief    Bash script for setup of Jenkins QNX dev environment.
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

optr=""
opti=""

while getopts i:r: opt; do
    case ${opt} in
    i )
        opti=$OPTARG
        ;;
    r )
        optr=$OPTARG
        ;;
    \?)
        echo "Usage: setup-dev-environment.sh [options]"
        echo "  -i local file full path to store images"
        echo "  -r local file full path to dev-can-linux Git repository"
        echo ""
        exit
        ;;
    esac
done

cd $optr/dev

docker build \
    -f $optr/dev/qnx.Dockerfile \
    -t localhost/dev-can-linux-dev .

docker run -d --name=dev_env \
    --network host \
    localhost/dev-can-linux-dev tail -f /dev/null

docker cp $optr dev_env:/root/

cd $optr/tests/emulation

docker build \
    -f $optr/tests/emulation/qemu.Dockerfile \
    -t localhost/dev-can-linux-qemu .

docker exec --user root --workdir /root dev_env bash -c \
    "source .profile \
    && /root/dev-can-linux/dev/setup-profile.sh \
    && cd /root/dev-can-linux/tests/image \
    && ./builddisk.sh"

rm -rf /home/jenkins/Images

docker cp dev_env:/root/dev-can-linux/tests/image/output $opti

