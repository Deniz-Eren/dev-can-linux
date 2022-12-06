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

FROM docker.io/library/ubuntu:20.04
RUN apt-get update \
    && apt-get dist-upgrade -y \
    && export DEBIAN_FRONTEND=noninteractive \
    && apt-get install --no-install-recommends -y \
            bash \
            bc \
            bsdmainutils \
            bzip2 \
            ca-certificates \
            ccache \
            clang \
            dbus \
            debianutils \
            diffutils \
            exuberant-ctags \
            findutils \
            g++ \
            gcc \
            gcovr \
            genisoimage \
            gettext \
            git \
            hostname \
            libaio-dev \
            libasan5 \
            libasound2-dev \
            libattr1-dev \
            libbrlapi-dev \
            libbz2-dev \
            libc6-dev \
            libcacard-dev \
            libcap-ng-dev \
            libcapstone-dev \
            libcurl4-gnutls-dev \
            libdaxctl-dev \
            libdrm-dev \
            libepoxy-dev \
            libfdt-dev \
            libffi-dev \
            libfuse3-dev \
            libgbm-dev \
            libgcrypt20-dev \
            libglib2.0-dev \
            libglusterfs-dev \
            libgnutls28-dev \
            libgtk-3-dev \
            libibumad-dev \
            libibverbs-dev \
            libiscsi-dev \
            libjemalloc-dev \
            libjpeg-turbo8-dev \
            liblttng-ust-dev \
            liblzo2-dev \
            libncursesw5-dev \
            libnfs-dev \
            libnuma-dev \
            libpam0g-dev \
            libpcre2-dev \
            libpixman-1-dev \
            libpmem-dev \
            libpng-dev \
            libpulse-dev \
            librbd-dev \
            librdmacm-dev \
            libsasl2-dev \
            libsdl2-dev \
            libsdl2-image-dev \
            libseccomp-dev \
            libselinux1-dev \
            libslirp-dev \
            libsnappy-dev \
            libspice-protocol-dev \
            libspice-server-dev \
            libssh-dev \
            libsystemd-dev \
            libtasn1-6-dev \
            libubsan1 \
            libudev-dev \
            libusb-1.0-0-dev \
            libusbredirhost-dev \
            libvdeplug-dev \
            libvirglrenderer-dev \
            libvte-2.91-dev \
            libxen-dev \
            libzstd-dev \
            llvm \
            locales \
            make \
            multipath-tools \
            ncat \
            nettle-dev \
            ninja-build \
            openssh-client \
            perl-base \
            pkgconf \
            python3 \
            python3-numpy \
            python3-opencv \
            python3-pillow \
            python3-pip \
            python3-setuptools \
            python3-sphinx \
            python3-sphinx-rtd-theme \
            python3-venv \
            python3-wheel \
            python3-yaml \
            rpm2cpio \
            sed \
            sparse \
            systemtap-sdt-dev \
            tar \
            tesseract-ocr \
            tesseract-ocr-eng \
            texinfo \
            xfslibs-dev \
            zlib1g-dev \
            iproute2 \
            net-tools \
    && apt-get autoremove -y \
    && apt-get autoclean -y \
    && pip3 install meson==0.56.0 \
    && git clone https://github.com/qemu/qemu.git \
    && cd qemu \
    && ./configure --enable-kvm \
    && make -j8 install \
    && cd .. \
    && rm -r qemu
WORKDIR /root
