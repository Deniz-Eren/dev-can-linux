# \file     CMakeLists.txt
#
# \details  CMake file for candump tool
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

add_executable( candump
    ${CMAKE_CURRENT_SOURCE_DIR}/src/candump.c )

target_include_directories( candump PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/> )

if( CMAKE_BUILD_TYPE MATCHES Profiling )
    target_link_libraries( candump ${QNX_PROFILING_LIBRARY} )
else()
    target_link_libraries( candump )
endif()

install( TARGETS candump
    DESTINATION bin/
    COMPONENT bin )
