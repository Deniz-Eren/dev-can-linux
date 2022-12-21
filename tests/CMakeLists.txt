# \file     CMakeLists.txt
# \brief    CMake listing file for tests
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

if( NOT BUILD_TESTING )
    return()
endif()

enable_testing()

find_package( GTest REQUIRED )

if( GTest_FOUND )
    include_directories( SYSTEM ${GTEST_INCLUDE_DIRS} )
endif()

add_subdirectory( timer )

# TODO: this works because there is only one test binary, need to make this more
#       generic.
code_coverage_gen_html( ssh-timer-tests )
