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

add_subdirectory( driver )
add_subdirectory( queue )
add_subdirectory( timer )

if( CMAKE_BUILD_TYPE MATCHES Coverage AND NOT DISABLE_COVERAGE_HTML_GEN )
    add_custom_target( all-cov-runs ALL
        DEPENDS # list all coverage run targets here:
            ssh-driver-baud-tests-cov-run
            ssh-driver-integrity-tests-cov-run
            ssh-driver-io-tests-cov-run
            ssh-driver-raw-tests-cov-run
            ssh-queue-tests-cov-run
            ssh-timer-tests-cov-run )

    code_coverage_gen_html( all-cov-runs )
endif()
