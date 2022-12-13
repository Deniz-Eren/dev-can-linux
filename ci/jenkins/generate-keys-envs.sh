#!/bin/bash
#
# \file     generate-keys-envs.sh
# \brief    Bash script that generates the SSH keys needed by Jenkins.
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

if [ -f .env ]; then
    echo "error: env and key files already exists"
    exit -1
fi

#
# Added Docker group to .env
#

DOCKER_GROUP=$(getent group docker | cut -d: -f3)

echo -n DOCKER_GROUP= >> .env
echo $DOCKER_GROUP >> .env

#
# Generate private/public key pair .jenkins_agent & .jenkins_agent.pub and add
# public key to .env
#

ssh-keygen -q -t ed25519 -o -a 100 -N '' -f .jenkins_agent

echo -n JENKINS_AGENT_SSH_PUBKEY= >> .env
cat .jenkins_agent.pub >> .env
