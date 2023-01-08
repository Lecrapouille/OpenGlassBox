PROJECT = OpenGlassBox
TARGET = $(PROJECT)-UnitTest
DESCRIPTION = Unit tests for $(PROJECT)
BUILD_TYPE = debug
STANDARD = --std=c++14
USE_COVERAGE = 1

P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P) $(P)/src
INCLUDES += -I$(P) -I$(P)/include

OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o Rule.o
OBJS += MapCoordinatesInsideRadius.o MapRandomCoordinates.o RuleCommand.o RuleValue.o
OBJS += ScriptParser.o Dijkstra.o
OBJS += TestsResource.o TestsResources.o TestsUnit.o TestsPath.o TestsMap.o TestsCity.o
OBJS += TestsAgent.o TestsCoordInsideRadius.o TestsValue.o TestsCommand.o TestsScriptParser.o
OBJS += TestsSimulation.o
OBJS += main.o

DEFINES += -DVIRTUAL=virtual -DDESIRED_GRID_SIZE=2u
DEFINES += -Wno-unused-function -Wno-undef -Wno-keyword-macro -Wno-float-equal -Wno-gnu-zero-variadic-macro-arguments
PKG_LIBS += gtest gmock

all: $(TARGET)

.PHONY: unit-tests
.PHONY: check
unit-tests check: $(TARGET)
	@$(call print-to,"Running","$(TARGET)","$(RAPPORT)","")
	$(SANITIZER) ./$(BUILD)/$(TARGET) $(TU_OPTIONS) || (cat $(TARGET).log; return 1)

include $(M)/Makefile.footer
