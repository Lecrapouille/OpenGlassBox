//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file InstallRouter.hpp
//! \brief Give every city of a simulation the default router.
//!
//! Kept apart from DijkstraRouter.hpp so that the router only needs to know
//! about a City. A city loaded from a save has no router at all, and an agent
//! without one never leaves the crossroads it was sent from, so this is what a
//! loader calls once the cities are in place.

#ifndef OPEN_GLASSBOX_INSTALL_ROUTER_HPP
#define OPEN_GLASSBOX_INSTALL_ROUTER_HPP

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb
{

// -----------------------------------------------------------------------------
//! \brief Install a router on every city a simulation already holds.
//! \param[in,out] simulation the cities to wire up.
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
