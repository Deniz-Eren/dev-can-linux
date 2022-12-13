#!/bin/bash
#
# \file     setup-profile.sh
# \brief    Bash script that sources QNX environment variables.
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

if ! grep -q /root/qnx710/qnxsdp-env.sh /root/.profile; then
cat << END >> /root/.profile

# Source QNX environment
if [ -f /root/qnx710/qnxsdp-env.sh ]; then
    source /root/qnx710/qnxsdp-env.sh

    export QNX_BASE
    export QNX_TARGET
    export QNX_HOST
    export MAKEFLAGS
    export QNX_CONFIGURATION
    export QNX_CONFIGURATION_EXCLUSIVE
fi
END
fi
