# syntax=docker/dockerfile:1
#
# \file     qnx.Dockerfile
# \brief    Template Dockerfile script that describes the Linux container
#           needed to run the QNX development environment needed to
#           cross-compile dev-can-linux.
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

# Default template starts with Ubuntu as the base image.
# After setup is complete (see README.md) change this to reference your own
# image:
FROM ubuntu:22.04

RUN export TZ=Australia/Sydney \
    && ln -snf /usr/share/zoneinfo/$TZ /etc/localtime \
    && echo $TZ > /etc/timezone \
    && apt-get update \
    && apt-get dist-upgrade -y \
    && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install --no-install-recommends -y \
        apt-utils \
        lsb-release \
        language-pack-en-base \
        wget \
        ca-certificates \
        iproute2 \
        less \
        net-tools \
        vim \
        git \
        libswt-gtk-4-java \
    && apt-get autoremove -y \
    && apt-get autoclean -y

ENV NVIDIA_DRIVER_CAPABILITIES all

WORKDIR /root
