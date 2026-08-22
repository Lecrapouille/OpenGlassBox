//==============================================================================
// SimForth: A Forth for SimTaDyn.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of SimForth.
//
// SimForth is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SimForth.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

//! \file main.hpp
//! \brief Google Test/Mock harness shared by the OpenGlassBox unit tests.

#ifndef MAIN_HPP
#  define MAIN_HPP

#  include <gmock/gmock.h>
#  include <gtest/gtest.h>

#  include "OpenGlassBox/Config.hpp"

using namespace ::testing;
using namespace ogb;

// Set interpreter quiet
#  define QUIET(interpreter) \
     forth::Options opt; \
     opt.show_stack = false; \
     opt.quiet = true; \
     interpreter.setOptions(opt);

#endif // MAIN_HPP
