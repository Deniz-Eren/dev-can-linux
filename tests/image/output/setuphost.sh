#!/bin/bash
#
# \file     setuphost.sh
# \brief    Bash script that prepares the host Linux system with needed
#           functionality before running Docker-compose.
#
# \details  Currently everything is containerised except for the starting up of
#           the needed Linux kernel modules; Virtual Local CAN Interface (vcan)
#           and Kernel Virtual Machine (kvm).
#           Note you will need an Intel processor with VT (virtualization
#           technology) extensions, or an AMD processor with SVM extensions
#           (also called AMD-V). You may need to enable these extensions in your
#           BIOS if they are disabled by default.
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

modprobe vcan kvm
