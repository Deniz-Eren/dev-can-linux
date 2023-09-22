# \file     CMakeLists.txt
# \brief    CMake listing file for driver IO message tests; basic single
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

add_executable( driver-io-tests ${C_SOURCE_FILES}
    driver-io-tests.cpp )

target_include_directories( driver-io-tests PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include/uapi>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/arch/x86/include> )

if( CMAKE_BUILD_TYPE MATCHES Profiling )
    target_link_libraries( driver-io-tests PRIVATE
        -lpci
        ${GTEST_LIBRARIES}
        ${QNX_PROFILING_LIBRARY} )
else()
    target_link_libraries( driver-io-tests PRIVATE
        -lpci
        ${GTEST_LIBRARIES} )
endif()

add_custom_target( ssh-driver-io-tests ALL
    COMMAND ${CMAKE_SOURCE_DIR}/dev-qnx/cmake/Modules/MakeSSHCommand.sh
        -p ${SSH_PORT}
        -s ${CMAKE_CURRENT_BINARY_DIR}/driver-io-tests
        -e "DRIVER_TEST_DEVICE0_RX0=\\\${DRIVER_TEST_DEVICE0_RX0} \
            DRIVER_TEST_DEVICE0_TX0=\\\${DRIVER_TEST_DEVICE0_TX0} \
            DRIVER_TEST_DEVICE1_RX0=\\\${DRIVER_TEST_DEVICE1_RX0} \
            DRIVER_TEST_DEVICE1_TX0=\\\${DRIVER_TEST_DEVICE1_TX0}"
        -r ${CMAKE_BINARY_DIR}
        -o ${CMAKE_CURRENT_BINARY_DIR}/ssh-driver-io-tests.sh
    BYPRODUCTS ssh-driver-io-tests.sh
    DEPENDS driver-io-tests )

add_test( NAME ssh-driver-io-tests
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/ssh-driver-io-tests.sh )

code_coverage_run( ssh-driver-io-tests )

# TODO: implement profiling for unit tests
#valgrind_profiling_run( ssh-driver-io-tests )
