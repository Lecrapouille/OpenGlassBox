###################################################
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
    $(call rwildcard,$(P)/tests,*.cpp)
INCLUDES := $(P)/include
VPATH := $(P)/src
DEFINES += -DVIRTUAL=virtual -DDESIRED_GRID_SIZE=2u
USER_CXX_FLAGS += -Wno-unused-function -Wno-undef \
  -Wno-keyword-macro -Wno-float-equal \
  -Wno-gnu-zero-variadic-macro-arguments
PKG_LIBS += gtest gmock

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile