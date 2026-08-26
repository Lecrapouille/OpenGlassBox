// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//! \file OpenGL.hpp
//! \brief Single include point for GLEW, GLFW, ImGui and ImPlot in the demo.


#ifndef OPEN_GLASSBOX_DEMO_OPENGL_HPP
#  define OPEN_GLASSBOX_DEMO_OPENGL_HPP

// Enables the ImVec2 arithmetic operators used all over the canvas rendering.
// It has to be defined before the first inclusion of imgui.h in every
// translation unit, hence this single header gathering the includes.
#  ifndef IMGUI_DEFINE_MATH_OPERATORS
#    define IMGUI_DEFINE_MATH_OPERATORS
#  endif

// GLEW has to come before any other OpenGL header.
#  include <GL/glew.h>
#  include <GLFW/glfw3.h>

#  include <imgui.h>
#  include <imgui_internal.h>
#  include <imgui_impl_glfw.h>
#  include <imgui_impl_opengl3.h>

#  include <string>

#endif
