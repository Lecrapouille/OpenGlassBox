// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
