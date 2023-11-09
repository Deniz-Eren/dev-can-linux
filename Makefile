# \file     Makefile
#
# \details  This file is the post generation hand customise output of
#           QNX Momentics IDE run on a QNX licensed machine.
#           Do NOT remove or replace this file using CMake system, because this
#           file is used parallel to the CMake system when developing within QNX
#           Momentics.
#           IMPORTANT! Macro configurations must be handled with configuration
#           files in config/ directory, not here directly! Both CMake and this
#           Makefile refer to the same configuration settings.
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

ARTIFACT = dev-can-linux

#Build architecture/variant string, possible values: x86, armv7le, etc...
PLATFORM ?= x86_64

#Build profile, possible values: release, debug, profile, coverage
BUILD_PROFILE ?= debug

CONFIG_NAME ?= $(PLATFORM)-$(BUILD_PROFILE)
OUTPUT_DIR = build/$(CONFIG_NAME)
TARGET = $(OUTPUT_DIR)/$(ARTIFACT)

#Compiler definitions

CC = qcc -Vgcc_nto$(PLATFORM)
CXX = q++ -Vgcc_nto$(PLATFORM)_cxx
LD = $(CXX)

#User defined include/preprocessor flags and libraries

INCLUDES += -I. -Isrc -Isrc/include \
	-Isrc/kernel \
	-Isrc/kernel/include \
	-Isrc/kernel/include/uapi \
	-Isrc/kernel/arch/x86/include

LIBS += -lpci

#Compiler flags for build profiles
CCFLAGS_release += -O2
CCFLAGS_debug += -g -O0 -fno-builtin
CCFLAGS_coverage += -g -O0 -ftest-coverage -fprofile-arcs -nopipe -Wc,-auxbase-strip,$@
LDFLAGS_coverage += -ftest-coverage -fprofile-arcs
CCFLAGS_profile += -g -O0 -finstrument-functions
LIBS_profile += -lprofilingS

#
# Coverage & Profiling
#

CCFLAGS_coverage +=	-DCOVERAGE_BUILD
CCFLAGS_profile +=	-DPROFILING_BUILD

#
# Project specific configuration macros
#

CCFLAGS += -DPROGRAM_VERSION=\"`cat "config/PROGRAM_VERSION"`\"
CCFLAGS += -DHARMONIZED_LINUX_VERSION=\"`cat "config/HARMONIZED_LINUX_VERSION"`\"
CCFLAGS += -DCONFIG_QNX_INTERRUPT_ATTACH=`cat "config/CONFIG_QNX_INTERRUPT_ATTACH"`
CCFLAGS += -DCONFIG_QNX_INTERRUPT_ATTACH_EVENT=`cat "config/CONFIG_QNX_INTERRUPT_ATTACH_EVENT"`
CCFLAGS += -DCONFIG_QNX_INTERRUPT_MASK_ISR=`cat "config/CONFIG_QNX_INTERRUPT_MASK_ISR"`
CCFLAGS += -DCONFIG_QNX_INTERRUPT_MASK_PULSE=`cat "config/CONFIG_QNX_INTERRUPT_MASK_PULSE"`
CCFLAGS += -DCONFIG_QNX_RESMGR_SINGLE_THREAD=`cat "config/CONFIG_QNX_RESMGR_SINGLE_THREAD"`
CCFLAGS += -DCONFIG_QNX_RESMGR_THREAD_POOL=`cat "config/CONFIG_QNX_RESMGR_THREAD_POOL"`

#
# Linux Kernel configuration macros
#

CCFLAGS +=	-DCONFIG_HZ=`cat "config/CONFIG_HZ"`

# Always enable the following
CCFLAGS +=	-DCONFIG_CAN_CALC_BITTIMING
CCFLAGS +=	-DCONFIG_X86_64 # 32-bit alternative is CONFIG_X86_32

#Generic compiler flags (which include build type flags)
CCFLAGS_all += -Wall -fmessage-length=0
CCFLAGS_all += $(CCFLAGS_$(BUILD_PROFILE))
#Shared library has to be compiled with -fPIC
#CCFLAGS_all += -fPIC
LDFLAGS_all += $(LDFLAGS_$(BUILD_PROFILE))
LIBS_all += $(LIBS_$(BUILD_PROFILE))
DEPS = -Wp,-MMD,$(@:%.o=%.d),-MT,$@

#Macro to expand files recursively: parameters $1 -  directory, $2 - extension, i.e. cpp
rwildcard = $(wildcard $(addprefix $1/*.,$2)) $(foreach d,$(wildcard $1/*),$(call rwildcard,$d,$2))

#Source list
SRCS = $(call rwildcard, src, c cpp)

#Object files list
OBJS = $(addprefix $(OUTPUT_DIR)/,$(addsuffix .o, $(basename $(SRCS))))

#Compiling rule
$(OUTPUT_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<
$(OUTPUT_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(DEPS) -o $@ $(INCLUDES) $(CCFLAGS_all) $(CCFLAGS) $<

#Linking rule
$(TARGET):$(OBJS)
	$(LD) -o $(TARGET) $(LDFLAGS_all) $(LDFLAGS) $(OBJS) $(LIBS_all) $(LIBS)

#Rules section for default compilation and linking
all: $(TARGET)

clean:
	rm -fr $(OUTPUT_DIR)

rebuild: clean all

#Inclusion of dependencies (object files to source and includes)
-include $(OBJS:%.o=%.d)
