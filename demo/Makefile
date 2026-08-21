###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)-demo
TARGET_DESCRIPTION := A demo using $(PROJECT_NAME)
DATA_FOLDER_NAME := demo/data
include $(M)/project/Makefile

###################################################
# Set third party Dear ImGui, with the GLFW and
# OpenGL 3 backends.
#
SRC_FILES += $(P)/external/imgui/imgui.cpp
SRC_FILES += $(P)/external/imgui/imgui_draw.cpp
SRC_FILES += $(P)/external/imgui/imgui_tables.cpp
SRC_FILES += $(P)/external/imgui/imgui_widgets.cpp
SRC_FILES += $(P)/external/imgui/misc/cpp/imgui_stdlib.cpp
SRC_FILES += $(P)/external/imgui/backends/imgui_impl_glfw.cpp
SRC_FILES += $(P)/external/imgui/backends/imgui_impl_opengl3.cpp

###################################################
# Set third party ImPlot (charts) and ImGuiFileDialog
# (the File menu).
#
SRC_FILES += $(P)/external/implot/implot.cpp
SRC_FILES += $(P)/external/implot/implot_items.cpp
SRC_FILES += $(P)/external/ImGuiFileDialog/ImGuiFileDialog.cpp

INCLUDES := $(P)/external/imgui $(P)/external/imgui/backends \
    $(P)/external/imgui/misc/cpp $(P)/external/implot \
    $(P)/external/ImGuiFileDialog
PKG_LIBS := glfw3 glew gl

USER_CXXFLAGS += -Wno-useless-cast -Wno-conversion -Wno-sign-conversion -Wno-float-equal
USER_CXXFLAGS += -Wno-float-equal -Wno-float-conversion -Wno-shadow -Wno-unused-parameter
USER_CXXFLAGS += -Wno-unused-result -Wno-double-promotion -Wno-cast-qual
USER_CXXFLAGS += -Wno-unused-variable -Wno-old-style-cast -Wno-null-dereference -Wno-switch-enum

###################################################
# Compile the standalone application
#
SRC_FILES += $(call rwildcard,src,*.cpp)
INCLUDES += $(P)/include $(P)/demo/src
VPATH += $(P)/demo
DEFINES += -DVIRTUAL=
INTERNAL_LIBS += $(call internal-lib,$(PROJECT_NAME))

###################################################
# Create MacOS X bundle application.
#
ifeq ($(OS),Darwin)
    BUILD_MACOS_APP_BUNDLE = 1
    APPLE_IDENTIFIER := lecrapouille
    MACOS_BUNDLE_ICON := $(PROJECT_DATA_DIR)/$(PROJECT_NAME).icns
    LINKER_FLAGS := -framework CoreFoundation
endif

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
