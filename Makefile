PROJECT = GlassBox
TARGET = $(PROJECT)
DESCRIPTION = An Interpretation of SimCity2013 Game Engine
BUILD_TYPE = release

P := .
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P)/src $(P)/src/Core $(P)/src/Display
INCLUDES += -I$(P)/src
OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o
OBJS += Rule.o RuleCommand.o RuleValue.o
OBJS += Window.o main.o

PKG_LIBS = sdl2

all: $(TARGET)

include $(M)/Makefile.footer
