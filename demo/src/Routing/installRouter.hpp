//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file installRouter.hpp
//! \brief Wire the demo router into a City or a Simulation.

#ifndef OPEN_GLASSBOX_DEMO_INSTALL_ROUTER_HPP
#define OPEN_GLASSBOX_DEMO_INSTALL_ROUTER_HPP

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "Routing/DijkstraRouter.hpp"

#include <memory>

namespace ogb
{

// -------------------------------------------------------------------------
//! \brief Give a town the default Dijkstra router.
//! \param[in,out] city receives ownership of the router.
//! \param[in] config read for SimulationConfig::randomSeed.
// -------------------------------------------------------------------------
inline void installDijkstraRouter(City& city, SimulationConfig const& config)
{
    auto router = std::make_unique<Dijkstra>();
    if (config.randomSeed != 0u)
        router->setRandomSeed(config.randomSeed);
    city.setRouter(std::move(router));
}

// -------------------------------------------------------------------------
//! \brief Install a router on every town already held by a simulation.
//! \param[in,out] simulation the towns to wire up.
// -------------------------------------------------------------------------
inline void installDijkstraRouters(Simulation& simulation)
{
    for (auto const& it : simulation.cities())
        installDijkstraRouter(*it.second, simulation.config());
}

} // namespace ogb

#endif
