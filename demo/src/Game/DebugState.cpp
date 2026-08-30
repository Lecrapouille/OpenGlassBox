// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Game/DebugState.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb::game
{

// ----------------------------------------------------------------------------
Agent* Selection::resolveAgent(Simulation const& simulation) const
{
    if (kind != Kind::Agent)
        return nullptr;

    auto const it = simulation.getCities().find(city);
    if (it == simulation.getCities().end())
        return nullptr;

    for (auto const& agent : it->second->getAgents())
    {
        if (agent->getId() == agentId)
            return agent.get();
    }

    return nullptr;
}

// ----------------------------------------------------------------------------
bool DebugState::anyLayerVisible(Simulation const& simulation) const
{
    for (auto const& [_, layer] : simulation.getLayers())
    {
        if (isLayerVisible(layer->getTypeName().str()))
            return true;
    }

    return false;
}

// ----------------------------------------------------------------------------
bool DebugState::drawsZones(Simulation const& simulation) const
{
    switch (zoneDisplay)
    {
        case ZoneDisplay::Always:
            return true;
        case ZoneDisplay::Never:
            return false;
        case ZoneDisplay::Auto:
        default:
            return !anyLayerVisible(simulation);
    }
}
} // namespace ogb::game
