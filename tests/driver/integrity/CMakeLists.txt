# \file     CMakeLists.txt
# \brief    CMake listing file for driver integrity level tests; basic single
#           message sending and receiving.
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

add_executable( driver-integrity-tests ${C_SOURCE_FILES}
    driver-flood-tests.cpp
    driver-latency-tests.cpp
    driver-sync-tests.cpp )

target_include_directories( driver-integrity-tests PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include/uapi>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/arch/x86/include> )

if( CMAKE_BUILD_TYPE MATCHES Profiling )
    target_link_libraries( driver-integrity-tests PRIVATE
        -lpci
        ${GTEST_LIBRARIES}
        ${QNX_PROFILING_LIBRARY} )
else()
    target_link_libraries( driver-integrity-tests PRIVATE
        -lpci
        ${GTEST_LIBRARIES} )
endif()

add_custom_target( ssh-driver-integrity-tests ALL
    COMMAND ${CMAKE_SOURCE_DIR}/workspace/cmake/Modules/MakeSSHCommand.sh
        -p ${SSH_PORT}
        -s ${CMAKE_CURRENT_BINARY_DIR}/driver-integrity-tests
        -e ${TESTING_DEVICE_ENV_FILE}
        -r ${CMAKE_BINARY_DIR}
        -o ${CMAKE_CURRENT_BINARY_DIR}/ssh-driver-integrity-tests.sh
    BYPRODUCTS ssh-driver-integrity-tests.sh
    DEPENDS driver-integrity-tests )

add_test( NAME ssh-driver-integrity-tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ssh-driver-integrity-tests.sh )

code_coverage_run( ssh-driver-integrity-tests )

# TODO: implement profiling for unit tests
#valgrind_profiling_run( ssh-driver-integrity-tests )
