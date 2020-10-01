PROJECT = OpenGlassBox
TARGET = $(PROJECT)
DESCRIPTION = An Open Source Implementation of the SimCity 2013 Simulation Engine GlassBox
BUILD_TYPE = release

P := .
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P)/src $(P)/src/Core $(P)/src/Display
INCLUDES += -I$(P)/src
OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o
OBJS += Script.o
OBJS += MapCoordinatesInsideRadius.o Rule.o RuleCommand.o RuleValue.o
OBJS += Window.o main.o

PKG_LIBS = sdl2

all: $(TARGET)

include $(M)/Makefile.footer
