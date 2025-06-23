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

#ifndef OPENGLCPPWRAPPER_DEARIMGUI_HPP
#  define OPENGLCPPWRAPPER_DEARIMGUI_HPP

#  ifndef IMGUI_DEFINE_MATH_OPERATORS
#    define IMGUI_DEFINE_MATH_OPERATORS
#  endif // IMGUI_DEFINE_MATH_OPERATORS

#  include "imgui.h"
#  include "imgui_impl_sdl2.h"
#  include "imgui_impl_sdlrenderer2.h"

#  include <vector>
#  include <string>

namespace ImGui
{

bool Combo(const char* label, int* currIndex, std::vector<std::string>& values);
bool ListBox(const char* label, int* currIndex, std::vector<std::string>& values);

} // namespace ImGui

#endif // OPENGLCPPWRAPPER_DEARIMGUI_HPP
