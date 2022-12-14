# syntax=docker/dockerfile:1
#
# \file     qemu.Dockerfile
# \brief    Dockerfile script that describes the Linux container needed to run
#           the QNX OS test setup within a QEmu VM capable of emulating needed
#           CAN-bus hardware for testing CAN-bus messaging.
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

FROM ubuntu:22.04

RUN export TZ=Australia/Sydney \
    && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
    && echo $TZ > /etc/timezone \
    && apt-get update \
    && apt-get dist-upgrade -y \
    && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install --no-install-recommends --assume-yes \
        apt-utils \
        lsb-release \
        wget \
        ca-certificates \
        iproute2 \
        less \
        net-tools \
        qemu-system-x86 \
        qemu-system-gui \
    && apt-get autoremove -y \
    && apt-get autoclean -y

ENV NVIDIA_DRIVER_CAPABILITIES all

WORKDIR /root
