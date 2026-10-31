###############################################################################
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

PROJECT_NAME := OpenGlassBox
PROJECT_VERSION := 0.2.0
CXX_STANDARD := c++17

# -O2 with debug symbols and assertions kept. A city the size of Chicago runs
# its rules over three hundred thousand cells per tick, which an -O0 build
# cannot keep up with, and the stack traces of the demo still name their lines.
# Use COMPILATION_MODE=debug to step through the code, =release to ship.
COMPILATION_MODE ?= normal

###############################################################################
# The simulation engine, as static and shared libraries.
#
TARGETS := OpenGlassBox
OpenGlassBox.TYPE := library
OpenGlassBox.DESCRIPTION := An open source implementation of the SimCity 2013 simulation engine GlassBox
OpenGlassBox.SOURCES := src/**/*.cpp
OpenGlassBox.PUBLIC_INCLUDES := include

###############################################################################
# The other targets, each declared in the directory of its sources.
#
SUBMAKEFILES := external/external.mk utils/utils.mk demo/demo.mk tests/tests.mk

###############################################################################
# Generic Makefile rules
#
M ?= .makefile
include $(M)/MyMakefile.mk
