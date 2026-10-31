###############################################################################
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
# Copyright (c) 2020-2026 Quentin Quadrat
#
# This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

###############################################################################
# The demo: a city builder showing the simulation. Paths are relative to demo/.
#
TARGETS += OpenGlassBox-demo
OpenGlassBox-demo.DESCRIPTION := A demo using OpenGlassBox
OpenGlassBox-demo.SOURCES := src/**/*.cpp
OpenGlassBox-demo.INCLUDES := src
OpenGlassBox-demo.DEPENDS := OpenGlassBox imgui implot ImGuiFileDialog
OpenGlassBox-demo.PROJECT_INFO := 1
OpenGlassBox-demo.DATA := data
OpenGlassBox-demo.CXXFLAGS := -Wno-format-nonliteral -Wno-conversion \
    -Wno-sign-conversion -Wno-float-equal -Wno-float-conversion -Wno-shadow \
    -Wno-unused-parameter -Wno-unused-result -Wno-double-promotion \
    -Wno-cast-qual -Wno-unused-variable -Wno-old-style-cast \
    -Wno-null-dereference -Wno-switch-enum

ifneq ($(TARGET_OS),Darwin)
    OpenGlassBox-demo.CXXFLAGS += -Wno-useless-cast
endif

###############################################################################
# GLFW and GLEW everywhere; OpenGL is platform-specific. The web build uses
# the GLFW and WebGL 2 that Emscripten ships, and embeds demo/data (the
# simulations live in there).
#
ifeq ($(TARGET_OS),Emscripten)
    ifdef EXAEQUOS
        PKG_CONFIG_STATIC := 1
        OpenGlassBox-demo.PKG_CONFIG := exa-wayland
        OpenGlassBox-demo.LDFLAGS := -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3
    else
        OpenGlassBox-demo.LDFLAGS := -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 \
            -sMAX_WEBGL_VERSION=2 -sFULL_ES3 \
            --shell-file $(abspath $(HERE)src/Host/shell.html) \
            --preload-file $(abspath $(HERE)data)@demo/data \
            -sFORCE_FILESYSTEM=1 -sASYNCIFY -sALLOW_MEMORY_GROWTH=1 \
            -sGL_ENABLE_GET_PROC_ADDRESS
    endif
else ifeq ($(TARGET_OS),Windows)
    # Static glfw3/glew and MinGW runtimes: no MSYS2 DLLs beside the .exe.
    # Requires mingw-w64-x86_64-glfw and mingw-w64-x86_64-glew (.a archives).
    PKG_CONFIG_STATIC := 1
    OpenGlassBox-demo.PKG_CONFIG := glew
    OpenGlassBox-demo.DEFINES := GLEW_STATIC
    OpenGlassBox-demo.LDFLAGS := -static
    OpenGlassBox-demo.LIBS := -lopengl32
else ifeq ($(TARGET_OS),Darwin)
    OpenGlassBox-demo.PKG_CONFIG := glew
    OpenGlassBox-demo.LIBS := -framework CoreFoundation -framework OpenGL
    # OpenGlassBox-demo.app, with demo/data in its resources.
    OpenGlassBox-demo.BUNDLE := 1
    OpenGlassBox-demo.BUNDLE_ID := com.lecrapouille.OpenGlassBox-demo
    OpenGlassBox-demo.ICON := data/OpenGlassBox.icns
else
    OpenGlassBox-demo.PKG_CONFIG := glew gl
endif
