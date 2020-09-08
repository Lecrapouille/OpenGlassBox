PROJECT = GlassBox
TARGET = $(PROJECT)
DESCRIPTION = An Interpretation of SimCity Game Engine (2012)
BUILD_TYPE = release

P := .
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P)/src
OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o Rule.o main.o

all: $(TARGET)

include $(M)/Makefile.footer