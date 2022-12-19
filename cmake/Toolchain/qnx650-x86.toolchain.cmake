# \file     qnx650-x86.toolchain.cmake
# \brief    CMake QNX 6.5 cross-compile toolchain for x86 platform.
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

# the name of the target operating system
set( CMAKE_SYSTEM_NAME qnx650 )

# which compilers to use for C and C++
set( CMAKE_C_COMPILER   $ENV{QNX_HOST}/usr/bin/qcc )
set( CMAKE_CXX_COMPILER $ENV{QNX_HOST}/usr/bin/QCC )

# where is the target environment located
set( CMAKE_FIND_ROOT_PATH
    $ENV{QNX_TARGET}/usr
    $ENV{QNX_TARGET}/x86
    $ENV{QNX_TARGET}/x86/usr )

# adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# search headers and libraries in the target environment
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# set gcov executable
set( QNX_GCOV_EXE $ENV{QNX_HOST}/usr/bin/ntox86-gcov )

# set profiling library
set( QNX_PROFILING_LIBRARY $ENV{QNX_TARGET}/x86/usr/lib/libprofilingS.a )
