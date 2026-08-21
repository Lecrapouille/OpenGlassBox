// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef OPEN_GLASSBOX_CONFIG_HPP
#  define OPEN_GLASSBOX_CONFIG_HPP

#include <cstdint>

namespace config
{
    static const uint32_t SCREEN_WIDTH = 800u;
    static const uint32_t SCREEN_HEIGHT = 600u;
    static const uint32_t GRID_SIZE = DESIRED_GRID_SIZE;
    static const float TICKS_PER_SECOND = 20.0f;
}

#endif
