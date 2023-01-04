##
# \file     CMakeProfilingHelper.cmake
# \brief    CMake code profile helper module file
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

function( code_profiling_flags )
    if( CMAKE_BUILD_TYPE MATCHES Profiling )
        if( "${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" OR
            "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang" )

            # Currently not handled or needed

        elseif( "${CMAKE_C_COMPILER_ID}" MATCHES "(QNX)?QCC|qcc" OR
                "${CMAKE_CXX_COMPILER_ID}" MATCHES "(QNX)?QCC|qcc" )

            set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finstrument-functions -O0 -g"
                PARENT_SCOPE )

            set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
                -finstrument-functions -O0 -g"
                PARENT_SCOPE )

        elseif( CMAKE_COMPILER_IS_GNUCXX )

            # Currently not handled or needed

        else()
            message( FATAL_ERROR "Code profiling requires Clang or GCC!" )
        endif()
    endif()
endfunction( code_profiling_flags )
