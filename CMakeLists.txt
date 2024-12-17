# \file     CMakeLists.txt
#
# \details  This file is the main CMake file for the project.
#           IMPORTANT! Macro configurations must be handled with configuration
#           files in config/ directory, not here directly! Both CMake and the
#           QNX Momentics compatible Makefile refer to the same configuration
#           settings.
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

cmake_minimum_required( VERSION 3.22 )

# Default toolchain file is set to QNX 8.0 x86_64 platform.
# Set "cmake -DCMAKE_TOOLCHAIN_FILE=<tool-chain-file>" to build for a different
# platform as required.
# QNX 8.0 options:
#   workspace/cmake/Toolchain/qnx800-x86_64.toolchain.cmake
#   workspace/cmake/Toolchain/qnx800-aarch64le.toolchain.cmake
# QNX 7.1 options:
#   workspace/cmake/Toolchain/qnx710-x86_64.toolchain.cmake
#   workspace/cmake/Toolchain/qnx710-aarch64le.toolchain.cmake
#   workspace/cmake/Toolchain/qnx710-armv7le.toolchain.cmake
if( NOT DEFINED CMAKE_TOOLCHAIN_FILE )
    if( DEFINED ENV{QNX_HOST} )
        set( CMAKE_TOOLCHAIN_FILE
            ${CMAKE_SOURCE_DIR}/workspace/cmake/Toolchain/qnx800-x86_64.toolchain.cmake )
    endif()
endif()

project( dev-can-linux )

# Append module paths to CMAKE_MODULE_PATH
list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/workspace/cmake/Modules )

# Read configuration items from config/ files
file( STRINGS "${PROJECT_SOURCE_DIR}/config/PROGRAM_VERSION" PROGRAM_VERSION )
file( STRINGS "${PROJECT_SOURCE_DIR}/config/HARMONIZED_LINUX_VERSION"
    HARMONIZED_LINUX_VERSION )

file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_HZ" CONFIG_HZ )

file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_QNX_INTERRUPT_MASK_ISR"
    CONFIG_QNX_INTERRUPT_MASK_ISR )

file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_QNX_INTERRUPT_MASK_PULSE"
    CONFIG_QNX_INTERRUPT_MASK_PULSE )

file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_QNX_RESMGR_SINGLE_THREAD"
    CONFIG_QNX_RESMGR_SINGLE_THREAD )

file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_QNX_RESMGR_THREAD_POOL"
    CONFIG_QNX_RESMGR_THREAD_POOL )

file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_IRQ_SCHED_PRIORITY_BOOST"
    CONFIG_IRQ_SCHED_PRIORITY_BOOST )

add_compile_options( -Wall )

add_compile_definitions( PROGRAM_VERSION="${PROGRAM_VERSION}" )
add_compile_definitions( HARMONIZED_LINUX_VERSION="${HARMONIZED_LINUX_VERSION}" )
add_compile_definitions( CONFIG_HZ=${CONFIG_HZ} )

add_compile_definitions(
    CONFIG_QNX_INTERRUPT_MASK_ISR=${CONFIG_QNX_INTERRUPT_MASK_ISR} )

add_compile_definitions(
    CONFIG_QNX_INTERRUPT_MASK_PULSE=${CONFIG_QNX_INTERRUPT_MASK_PULSE} )

add_compile_definitions(
    CONFIG_QNX_RESMGR_SINGLE_THREAD=${CONFIG_QNX_RESMGR_SINGLE_THREAD} )

add_compile_definitions(
    CONFIG_QNX_RESMGR_THREAD_POOL=${CONFIG_QNX_RESMGR_THREAD_POOL} )

add_compile_definitions(
    CONFIG_IRQ_SCHED_PRIORITY_BOOST=${CONFIG_IRQ_SCHED_PRIORITY_BOOST} )

add_compile_definitions( DEVCANLINUX_SYSLOG=1 )
add_compile_definitions( DEVCANLINUX_STDERR=1 )

# Always enable the following
add_compile_definitions( CONFIG_CAN_CALC_BITTIMING )
add_compile_definitions( CONFIG_X86_64 ) # 32-bit alternative is CONFIG_X86_32

project( dev-can-linux
         VERSION ${PROGRAM_VERSION}
         LANGUAGES C CXX )

set( SSH_PORT "6022" CACHE STRING "SSH port number" )

include( CodeCoverageProfiler )

# Compiler settings
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

# Enable output of compile commands during generation
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

install( FILES ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION share/dev-can-linux/
    COMPONENT bin )

install( FILES ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION share/dev-can-linux/
    COMPONENT dev )

install( FILES ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION share/dev-can-linux/
    COMPONENT bin )

install( FILES ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION share/dev-can-linux/
    COMPONENT dev )

# Install headers
install( DIRECTORY ${CMAKE_SOURCE_DIR}/dev-can-linux
    DESTINATION include/
    COMPONENT dev )

list( APPEND C_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/src/config.c
    ${CMAKE_SOURCE_DIR}/src/fixed.c
    ${CMAKE_SOURCE_DIR}/src/interrupt.c
    ${CMAKE_SOURCE_DIR}/src/logs.c
    ${CMAKE_SOURCE_DIR}/src/netif.c
    ${CMAKE_SOURCE_DIR}/src/pci.c
    ${CMAKE_SOURCE_DIR}/src/pci-capability.c
    ${CMAKE_SOURCE_DIR}/src/prints.c
    ${CMAKE_SOURCE_DIR}/src/driver-prints.c
    ${CMAKE_SOURCE_DIR}/src/queue.c
    ${CMAKE_SOURCE_DIR}/src/resmgr.c
    ${CMAKE_SOURCE_DIR}/src/session.c
    ${CMAKE_SOURCE_DIR}/src/timer.c )

aux_source_directory(
    ${CMAKE_SOURCE_DIR}/src/kernel/drivers/net/can/sja1000 C_SOURCE_FILES )

aux_source_directory(
    ${CMAKE_SOURCE_DIR}/src/kernel/drivers/net/can/dev C_SOURCE_FILES )

aux_source_directory(
    ${CMAKE_SOURCE_DIR}/src/kernel/lib/math C_SOURCE_FILES )

add_executable( dev-can-linux
    ${CMAKE_SOURCE_DIR}/src/main.c
    ${C_SOURCE_FILES} )

target_include_directories( dev-can-linux PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include/uapi>
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/arch/x86/include> )

if( CMAKE_BUILD_TYPE MATCHES Profiling )
    target_link_libraries( dev-can-linux -lpci ${QNX_PROFILING_LIBRARY} )
else()
    target_link_libraries( dev-can-linux -lpci )
endif()

install( TARGETS dev-can-linux
    DESTINATION bin/
    COMPONENT bin )

include( CTest )

code_coverage_flags()
code_profiling_flags()

add_subdirectory( tests )
add_subdirectory( tools )

# Package settings
set( CPACK_GENERATOR "TGZ" )
set( CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}-${arch}${BUILD_TYPE_NAME} )
set( CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF )
set( CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH} )
set( CPACK_PACKAGE_VERSION ${PROJECT_VERSION} )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md" )
set( CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE" )
set( CPACK_ARCHIVE_COMPONENT_INSTALL ON )
set( CPACK_ARCHIVE_BIN_FILE_NAME
    ${CMAKE_PROJECT_NAME}-${HARMONIZED_LINUX_VERSION}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME} )

set( CPACK_ARCHIVE_DEV_FILE_NAME
    ${CMAKE_PROJECT_NAME}-${HARMONIZED_LINUX_VERSION}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-dev )

include( CPack )
