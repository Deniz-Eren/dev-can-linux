#!/bin/sh
#
# \file     mount_fs.sh
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

echo "---> Mounting file systems"

if [ -e /data -a -e /system ]; then
    exit 0
fi

mount -t qnx6 -o sync=optional,mntperms=755 /dev/hd0t178 /system
mount -t qnx6 -o sync=optional,mntperms=755 /dev/hd0t179 /data
mount -t qnx6 /dev/hd0t177 /boot

ln -sPf /data/var/tmp /tmp
/system/xbin/cleanup_tmp
