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

# Set the toolchain file to QNX 7.1
set( CMAKE_TOOLCHAIN_FILE
        ${CMAKE_SOURCE_DIR}/cmake/Toolchain/qnx710-x86_64.toolchain.cmake )

project( dev-can-linux )

# Append module paths to CMAKE_MODULE_PATH
list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules )

# Read configuration items from config/ files
file( STRINGS "${PROJECT_SOURCE_DIR}/config/PROGRAM_VERSION" PROGRAM_VERSION )
file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_HZ" CONFIG_HZ )
file( STRINGS "${PROJECT_SOURCE_DIR}/config/CONFIG_QNX_INTERRUPT_ATTACH_EVENT"
        CONFIG_QNX_INTERRUPT_ATTACH_EVENT )

add_compile_definitions( PROGRAM_VERSION="${PROGRAM_VERSION}" )
add_compile_definitions( CONFIG_HZ=${CONFIG_HZ} )
add_compile_definitions(
        CONFIG_QNX_INTERRUPT_ATTACH_EVENT=${CONFIG_QNX_INTERRUPT_ATTACH_EVENT} )

# Always enable the following
add_compile_definitions( CONFIG_CAN_CALC_BITTIMING )
add_compile_definitions( CONFIG_X86_64 ) # 32-bit alternative is CONFIG_X86_32

project( dev-can-linux
         VERSION ${PROGRAM_VERSION}
         LANGUAGES CXX )

# Compiler settings
set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED True )

# Enable output of compile commands during generation
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

install( FILES ${CMAKE_SOURCE_DIR}/README.md
        DESTINATION share/dev-can-linux/
        COMPONENT ${CMAKE_PROJECT_NAME} )

install( FILES ${CMAKE_SOURCE_DIR}/LICENSE
        DESTINATION share/dev-can-linux/
        COMPONENT ${CMAKE_PROJECT_NAME} )

aux_source_directory(
    ${CMAKE_SOURCE_DIR}/src/ C_SOURCE_FILES )

aux_source_directory(
    ${CMAKE_SOURCE_DIR}/src/kernel/drivers/net/can/sja1000 C_SOURCE_FILES )

aux_source_directory(
    ${CMAKE_SOURCE_DIR}/src/kernel/drivers/net/can/dev C_SOURCE_FILES )

add_executable( dev-can-linux ${C_SOURCE_FILES} )

target_include_directories( dev-can-linux PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/include/uapi>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/kernel/arch/x86/include> )

target_link_libraries( dev-can-linux -lpci )

install( TARGETS dev-can-linux
        DESTINATION bin/
        COMPONENT ${CMAKE_PROJECT_NAME} )

include( CMakeCodeCoverageRunHelper )
include( CMakeBuildTypeHelper )

code_coverage_flags()

# Package settings
set( CPACK_GENERATOR "TGZ" )
set( CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}${BUILD_TYPE_NAME}" )
set( CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF )
set( CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR} )
set( CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR} )
set( CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH} )
set( CPACK_PACKAGE_VERSION ${PROJECT_VERSION} )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md" )
set( CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE" )

include( CPack )
