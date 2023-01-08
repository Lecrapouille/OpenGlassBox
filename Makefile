
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
P := .
M := $(P)/.makefile
include $(M)/Makefile.header

###################################################
# Inform Makefile where to find header files
#
INCLUDES += -I$(P)/include -I$(P)/external -I$(P)

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P)/src $(P)/src/Core $(P)/external

###################################################
# Project defines
#
DEFINES += -DVIRTUAL= -DDESIRED_GRID_SIZE=30u

###################################################
# Set Libraries. For knowing which libraries
# is needed please read the external/README.md file.
#
PKG_LIBS = sdl2 SDL2_image

###################################################
# Make the list of compiled files for the library
#
LIB_OBJS += Simulation.o Map.o City.o Unit.o Path.o Agent.o Resource.o Resources.o
LIB_OBJS += ScriptParser.o MapCoordinatesInsideRadius.o MapRandomCoordinates.o
LIB_OBJS += Rule.o RuleCommand.o RuleValue.o Dijkstra.o

###################################################
# Compile the project as static and shared libraries.
#
.PHONY: all
all: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE) demo

###################################################
# Compile the demo as standalone application.
#
.PHONY: demo
demo: | $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET)
	@$(call print-from,"Compiling scenarios",$(PROJECT),demo)
	$(MAKE) -C demo all

###################################################
# Install project. You need to be root.
#
ifeq ($(ARCHI),Linux)
.PHONY: install
install: $(STATIC_LIB_TARGET) $(SHARED_LIB_TARGET) $(PKG_FILE) demo
	@$(call INSTALL_BINARY)
	@$(call INSTALL_DOCUMENTATION)
	@$(call INSTALL_PROJECT_LIBRARIES)
	@$(call INSTALL_PROJECT_HEADERS)
endif

###################################################
# Compile and launch unit tests and generate the code coverage html report.
#
.PHONY: unit-tests
unit-tests:
	@$(call print-simple,"Compiling unit tests")
	@$(MAKE) -C tests coverage

###################################################
# Compile and launch unit tests and generate the code coverage html report.
#
.PHONY: check
check: unit-tests

###################################################
# Clean the whole project.
#
.PHONY: veryclean
veryclean: clean
	@rm -fr cov-int $(PROJECT).tgz *.log foo 2> /dev/null
	@(cd tests && $(MAKE) -s clean)
	@$(call print-simple,"Cleaning","$(PWD)/doc/html")
	@rm -fr $(THIRDPART)/*/ doc/html 2> /dev/null
	@$(MAKE) -C demo clean

###################################################
# Sharable informations between all Makefiles

include $(M)/Makefile.footer
