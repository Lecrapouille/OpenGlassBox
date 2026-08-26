# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)-tests
TARGET_DESCRIPTION := Unit tests for $(PROJECT_NAME)
COMPILATION_MODE := test
CXX_STANDARD := --std=c++17
include $(M)/project/Makefile

###################################################
# Compile the unit tests
#
SRC_FILES := $(call rwildcard,$(P)/src,*.cpp) \
    $(P)/demo/src/Save/CitySave.cpp \
    $(call rwildcard,$(P)/tests,*.cpp)
INCLUDES := $(P)/include $(P)/demo/src
VPATH := $(P)/src $(P)/demo/src/Save
USER_CXXFLAGS += -Wno-unused-function -Wno-undef \
  -Wno-keyword-macro -Wno-float-equal \
  -Wno-gnu-zero-variadic-macro-arguments -Wno-null-dereference
PKG_LIBS += gtest gmock

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
