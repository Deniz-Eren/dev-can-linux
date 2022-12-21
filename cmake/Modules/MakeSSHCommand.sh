#!/bin/bash
#
# \file     MakeSSHCommand.sh
# \brief    Bash script that generates a ssh command script.
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

opto=""
optr=""
opts=""

while getopts o:r:s: opt; do
    case ${opt} in
    o )
        SCRIPT_OUTPUT_PATH=$OPTARG
        ;;
    r )
        BUILD_ROOT=$OPTARG
        ;;
    s )
        FILE_SRC_PATH=$OPTARG
        ;;
    \?)
        echo "Usage: MakeSSHCommand.sh [options]"
        echo "  -n script name to create"
        echo "  -r project root directory path"
        echo "  -s source and destination directory path of executable"
        echo ""
        echo "Example make script:"
        echo "  MakeSSHCommand.sh -o dev-can-linux.sh \\"
        echo "      -r /root/dev-can-linux"
        echo "      -s /root/dev-can-linux/build/dev-can-linux"
        echo ""
        echo "Then test script:"
        echo "  # ./dev-can-linux.sh -V"
        echo "  dev-can-linux v1.0.0"
        echo ""
        exit
        ;;
    esac
done

FILE_DST_PATH=$FILE_SRC_PATH

FILENAME=`basename $FILE_SRC_PATH`
DIRNAME=`dirname $FILE_SRC_PATH`

cat << END > $SCRIPT_OUTPUT_PATH
sshpass -p 'root' ssh \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -p6022 root@localhost \
        "mkdir -p $DIRNAME"

sshpass -p 'root' scp \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -r -P6022 $FILE_SRC_PATH \
        root@localhost:$DIRNAME/

PROGRAM_ARGS=\$@

sshpass -p 'root' ssh \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -p6022 root@localhost \
        "$FILE_DST_PATH \$PROGRAM_ARGS"

EXITCODE=\$?

# Perform and file copy here
sshpass -p 'root' scp \
        -o 'StrictHostKeyChecking=no' \
        -o 'UserKnownHostsFile=/dev/null' \
        -o 'LogLevel=ERROR' \
        -r -P6022 root@localhost:$BUILD_ROOT $BUILD_ROOT/..

exit \$EXITCODE
END

chmod +x $SCRIPT_OUTPUT_PATH
