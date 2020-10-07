PROJECT = OpenGlassBox
TARGET = $(PROJECT)
DESCRIPTION = An Open Source Implementation of the SimCity 2013 Simulation Engine GlassBox
BUILD_TYPE = release

P := .
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P)/src $(P)/src/Core $(P)/src/Display
INCLUDES += -I$(P)/src

# Core
OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o
OBJS += ScriptParser.o MapCoordinatesInsideRadius.o Rule.o RuleCommand.o RuleValue.o
OBJS += Dijkstra.o
# Renderer
OBJS += Window.o
# Game
OBJS += main.o

DEFINES += -DVIRTUAL=
PKG_LIBS = sdl2

all: $(TARGET)

include $(M)/Makefile.footer
