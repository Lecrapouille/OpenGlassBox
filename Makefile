PROJECT = OpenGlassBox
TARGET = $(PROJECT)
DESCRIPTION = An Open Source Implementation of the SimCity 2013 Simulation Engine GlassBox
BUILD_TYPE = debug

P := .
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P)/src $(P)/src/Core $(P)/src/Display $(P)/external
INCLUDES += -I$(P)/src -I$(P)/external -I$(P)

# Core
OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o
OBJS += ScriptParser.o MapCoordinatesInsideRadius.o Rule.o RuleCommand.o RuleValue.o
OBJS += Dijkstra.o
# Renderer
OBJS += DearImGui.o SDLHelper.o Window.o
# Game
OBJS += Debug.o Draw.o main.o

DEFINES += -DVIRTUAL= -DDESIRED_GRID_SIZE=10u
PKG_LIBS = sdl2 SDL2_image

all: $(TARGET)

include $(M)/Makefile.footer
