PROJECT = GlassBox
TARGET = $(PROJECT)-UnitTest
DESCRIPTION = Unit tests for $(PROJECT)
BUILD_TYPE = debug
USE_COVERAGE = 1

P := ..
M := $(P)/.makefile
include $(M)/Makefile.header

VPATH += $(P):$(P)/src:$(P)/src/Core
INCLUDES += -I$(P) -I$(P)/src

OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o Rule.o
OBJS += MapCoordinatesInsideRadius.o RuleCommand.o RuleValue.o Script.o
OBJS += TestsResource.o TestsResources.o TestsUnit.o TestsPath.o TestsMap.o TestsCity.o
OBJS += TestsAgent.o TestsCoordInsideRadius.o TestsCommand.o TestsRule.o TestsScript.o
OBJS += main.o

DEFINES += -Wno-unused-function -Wno-undef -Wno-keyword-macro -Wno-float-equal
PKG_LIBS += gtest gmock

all: $(TARGET)

.PHONY: unit-tests
.PHONY: check
unit-tests check: $(TARGET)
	@$(call print-to,"Running","$(TARGET)","$(RAPPORT)","")
	$(SANITIZER) ./$(BUILD)/$(TARGET) $(TU_OPTIONS) || (cat $(TARGET).log; return 1)

include $(M)/Makefile.footer
