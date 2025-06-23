###################################################
# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := An open source implementation of the SimCity 2013 simulation engine GlassBox
include $(M)/project/Makefile

###################################################
# Compile shared and static libraries
#
LIB_FILES := $(call rwildcard,src,*.cpp)
INCLUDES := $(P)/include
VPATH := $(P)/src
DEFINES := -DVIRTUAL= -DDESIRED_GRID_SIZE=30u

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
all:: build-demo

.PHONY: build-demo
build-demo: $(TARGET_STATIC_LIB_NAME)
	$(Q)$(MAKE) --no-print-directory --directory=demo all