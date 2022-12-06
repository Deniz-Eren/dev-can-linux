#!/bin/sh
#
# \file     post_startup.sh
# \brief    Bash script included in the image filesystem (IFS) that runs during
#           boot up of the QNX OS test image.
#
# \details  This file is the post generation hand customise output of
#           "mkqnximage" command run on a QNX licensed machine.
#           Important! not intended to be a secure production solution; created
#           only for controlled testing purposes.
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

# Configure locations of PAM files.  Note, pamconf must end with a slash
setconf pamlib /system/pam/lib
setconf pamconf /system/pam/config/

if [ ! -e /pps/slogger2 ]; then
    mkdir /pps/slogger2
fi

pinger &

echo Process count:`pidin arg | wc -l`

exit 0
