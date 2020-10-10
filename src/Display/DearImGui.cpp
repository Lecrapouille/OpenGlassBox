//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2019 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

// *****************************************************************************
//! \file This file just gathers imgui cpp files and around them with pragma
//! for reducing compilation warnings. Yep this is misery!
// *****************************************************************************

#include "Display/DearImGui.hpp"

#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wstrict-overflow"
#  pragma GCC diagnostic ignored "-Wswitch-default"
#  pragma GCC diagnostic ignored "-Wcast-qual"
#  pragma GCC diagnostic ignored "-Waggregate-return"
#  pragma GCC diagnostic ignored "-Wsign-promo"
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#  pragma GCC diagnostic ignored "-Wunused-function"

#include "Core/Unique.hpp"
#include "external/imgui/imgui_draw.cpp"
#include "external/imgui/imgui_widgets.cpp"
#include "external/imgui/imgui.cpp"
#include "external/imgui/imgui_sdl.cpp"

# pragma GCC diagnostic pop
