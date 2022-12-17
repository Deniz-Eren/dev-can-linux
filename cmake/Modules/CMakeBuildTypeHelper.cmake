##
# \file     CMakeBuildTypeHelper.cmake
# \brief    Build type helper CMake module file
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

if( CMAKE_BUILD_TYPE MATCHES Coverage )
    set( BUILD_TYPE_NAME "-cov" )
elseif( CMAKE_BUILD_TYPE MATCHES Debug )
    set( BUILD_TYPE_NAME "-g" )
else()
    set( BUILD_TYPE_NAME "" )
endif()
