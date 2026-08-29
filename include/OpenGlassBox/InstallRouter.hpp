// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

//! \file InstallRouter.hpp
//! \brief Install the default router on every city in a simulation.
//!
//! Kept separate from DijkstraRouter.hpp so the router only needs a City.
//! A city loaded from a save has no router. An Agent without one never moves.
//! Call this after all cities are loaded.

#ifndef OPEN_GLASSBOX_INSTALL_ROUTER_HPP
#define OPEN_GLASSBOX_INSTALL_ROUTER_HPP

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb
{

// -----------------------------------------------------------------------------
//! \brief Install a router on every city in the simulation.
//! \param[in,out] simulation the simulation whose cities get a router.
// -----------------------------------------------------------------------------
inline void installDijkstraRouters(Simulation const& simulation)
{
    for (auto const& it : simulation.getCities())
    {
        installDijkstraRouter(*it.second, simulation.getConfig());
    }
}

} // namespace ogb

#endif
