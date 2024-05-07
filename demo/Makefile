
###################################################
# Project definition
#
PROJECT = OpenGlassBox
TARGET = $(PROJECT)
DESCRIPTION = An Open Source Implementation of the SimCity 2013 Simulation Engine GlassBox
STANDARD = --std=c++14
BUILD_TYPE = debug

###################################################
# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I. -I$(P)/include -I$(P)

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P) Display

###################################################
# Project defines
#
DEFINES += -DVIRTUAL= -DDESIRED_GRID_SIZE=30u
DEFINES += -DDATADIR=\"$(DATADIR):$(abspath data)/:data/\"

###################################################
# Set Libraries. For knowing which libraries
# is needed please read the external/README.md file.
#
PKG_LIBS = sdl2 SDL2_image

###################################################
# Compile against the OpenGlassBox shared library
#
THIRDPART_LIBS += $(abspath $(P)/$(BUILD)/libopenglassbox.a)

###################################################
# MacOS X
#
ifeq ($(ARCHI),Darwin)
BUILD_MACOS_APP_BUNDLE = 1
APPLE_IDENTIFIER = lecrapouille
MACOS_BUNDLE_ICON = data/$(PROJECT).icns
LINKER_FLAGS += -framework CoreFoundation
endif

###################################################
# Compile the demo as standalone application.
#
OBJS += DearImGui.o SDLHelper.o Window.o DataPath.o
OBJS += Debug.o Draw.o Listeners.o Demo.o main.o

###################################################
# Compile the demo as standalone application.
#
all: $(TARGET)
	@cp $(BUILD)/$(TARGET) ../$(BUILD)
ifeq ($(ARCHI),Darwin)
	@cp -r $(BUILD)/$(TARGET)$(EXT) ../$(BUILD)
endif

###################################################
# Sharable informations between all Makefiles
include $(M)/Makefile.footer
