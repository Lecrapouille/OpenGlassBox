###############################################################################
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.


###############################################################################
# Unit tests, run from the root of the project by 'make check'.
#
TARGETS += OpenGlassBox-tests
OpenGlassBox-tests.TYPE := test
OpenGlassBox-tests.SOURCES := *.cpp ../demo/src/Save/CitySave.cpp \
    ../demo/src/Editor/EditCommands.cpp
OpenGlassBox-tests.INCLUDES := ../demo/src
OpenGlassBox-tests.DEPENDS := OpenGlassBox
OpenGlassBox-tests.PKG_CONFIG := gtest gmock
OpenGlassBox-tests.CXXFLAGS := -Wno-unused-function -Wno-undef \
    -Wno-keyword-macro -Wno-float-equal \
    -Wno-gnu-zero-variadic-macro-arguments -Wno-null-dereference
