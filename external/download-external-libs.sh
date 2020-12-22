#!/bin/bash

### This script will git clone some libraries that OpenGlassBox needs and
### compile them. To avoid pollution, they are not installed into your
### environement. Therefore OpenGlassBox Makefiles have to know where to
### find their files (includes and static/shared libraries).

### $1 is given by ../Makefile and refers to the current architecture.
if [ "$1" == "" ]; then
  echo "Expected one argument. Select the architecture: Linux, Darwin or Windows"
  exit 1
fi
ARCHI="$1"
TARGET=OpenGlassBox

### Delete all previous directories to be sure to have and compile
### fresh code source.
rm -fr imgui_sdl imgui 2> /dev/null

function print-clone
{
    echo -e "\033[35m*** Cloning:\033[00m \033[36m$TARGET\033[00m <= \033[33m$1\033[00m"
}

### Library for creating GUI in OpenGL
### License: MIT
print-clone imgui
git clone https://github.com/ocornut/imgui.git > /dev/null 2> /dev/null
(cd imgui && git reset --hard f9b873662baac2388a4ca78c967e53eb5d83d2a1)
print-clone imgui_sdl
git clone https://github.com/Tyyppi77/imgui_sdl.git --depth=1 > /dev/null 2> /dev/null
cp imgui_sdl/imgui_sdl.cpp imgui/imgui_sdl.cpp
cp imgui_sdl/imgui_sdl.h imgui/imgui_sdl.h
