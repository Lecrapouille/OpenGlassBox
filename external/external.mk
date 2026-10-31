###############################################################################
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.
#
# Third parties, cloned in external/ on the first build and compiled as static
# libraries. Their headers are system headers: their warnings are not shown.

TARGETS += imgui implot ImGuiFileDialog

###############################################################################
# Dear ImGui, with the GLFW and OpenGL 3 backends.
#
imgui.GIT := https://github.com/ocornut/imgui.git
imgui.VERSION := docking
imgui.TYPE := static
imgui.SOURCES := imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp \
    misc/cpp/imgui_stdlib.cpp backends/imgui_impl_glfw.cpp \
    backends/imgui_impl_opengl3.cpp
imgui.PUBLIC_INCLUDES := . backends misc/cpp

# The web build uses the GLFW that Emscripten ships, ExaequOS its own.
ifeq ($(TARGET_OS),Emscripten)
    ifdef EXAEQUOS
        imgui.PKG_CONFIG := glfw
    endif
else
    imgui.PKG_CONFIG := glfw3
endif

###############################################################################
# ImPlot for charts.
#
implot.GIT := https://github.com/epezent/implot.git
implot.TYPE := static
implot.SOURCES := implot.cpp implot_items.cpp
implot.PUBLIC_INCLUDES := .
implot.DEPENDS := imgui

###############################################################################
# ImGuiFileDialog needed for the file menu (save/open).
#
ImGuiFileDialog.GIT := https://github.com/aiekick/ImGuiFileDialog.git
ImGuiFileDialog.TYPE := static
ImGuiFileDialog.SOURCES := ImGuiFileDialog.cpp
ImGuiFileDialog.PUBLIC_INCLUDES := .
ImGuiFileDialog.DEPENDS := imgui
