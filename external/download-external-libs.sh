#!/bin/bash -e
###############################################################################
### This script is called by (cd .. && make download-external-libs). It will
### git clone thirdparts needed for this project but does not compile them.
### It replaces git submodules that I dislike.
###############################################################################

### Bloat-free Graphical User interface for C++ with minimal dependencies 
### License: MIT
cloning ocornut/imgui -b docking
(cd imgui && git fetch --unshallow && git reset --hard f9b873662baac2388a4ca78c967e53eb5d83d2a1)

### ImGuiSDL: SDL2 based renderer for Dear ImGui
### License: MIT
cloning Tyyppi77/imgui_sdl
cp imgui_sdl/imgui_sdl.cpp imgui/imgui_sdl.cpp
cp imgui_sdl/imgui_sdl.h imgui/imgui_sdl.h