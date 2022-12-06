#!/bin/sh
#
# \file     startup.sh
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

# By default set umask so that services don't accidentally create group/world writable files.
umask 022

echo "---> Starting slogger2"
slogger2 -U 21:21 -P /pps/slogger2
waitfor /dev/slog

echo "---> Starting PCI Services"
pci-server --config=/proc/boot/pci_server.cfg
waitfor /dev/pci

echo "---> Starting disk drivers"
devb-ahci cam user=20:20 blk cache=64M,auto=partition,vnode=2000,ncache=2000,commit=low ;
waitfor /dev/hd0 10

echo "---> Mounting file systems"
mount_fs.sh

devc-con -e -n4
waitfor /dev/con2

reopen /dev/con2

syslogd -f /system/etc/syslog.conf
random -t -p -U 22:22 -s /data/var/random/rnd-seed  
waitfor /dev/random
pipe -U 24:24
waitfor /dev/pipe
devc-pty -U 37:37
dumper -U 30:30 -d /data/var/dumper

echo "---> Starting Networking"
/system/etc/rc.d/rc.net

echo "---> Starting sshd"
if [ ! -f /data/var/ssh/ssh_host_rsa_key ]; then
    ssh-keygen -q -t rsa -N '' -f /data/var/ssh/ssh_host_rsa_key
fi
if [ ! -f /data/var/ssh/ssh_host_ed25519_key ]; then
    ssh-keygen -q -t ed25519 -N '' -f /data/var/ssh/ssh_host_ed25519_key
fi

/system/xbin/sshd -f /system/etc/ssh/sshd_config

echo "---> Starting misc"
# Starting qconn by default isn't a secure thing to do; only done here for this
# VM test image for convenience
qconn

mq -U 27:27
mqueue
pps -U 32:32 -p /data/var/pps
io-audio -d audiopci cap_name=defaultc,play_name=defaultp -U 35:35


# Execute the post startup script.  This is a separate script to allow for common behavior of
# both slm and script based startup.
/proc/boot/post_startup.sh
