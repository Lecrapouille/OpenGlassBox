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
# Set third party DearImGui
#
SRC_FILES += $(P)/external/imgui/imgui.cpp
SRC_FILES += $(P)/external/imgui/imgui_draw.cpp
SRC_FILES += $(P)/external/imgui/imgui_tables.cpp
SRC_FILES += $(P)/external/imgui/imgui_widgets.cpp
SRC_FILES += $(P)/external/imgui/backends/imgui_impl_sdl2.cpp
SRC_FILES += $(P)/external/imgui/backends/imgui_impl_sdlrenderer2.cpp

INCLUDES := $(P)/external/imgui $(P)/external/imgui/backends
PKG_LIBS := sdl2 SDL2_image

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
DEFINES += -DVIRTUAL= -DDESIRED_GRID_SIZE=30u
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