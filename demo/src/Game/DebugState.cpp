// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Game/DebugState.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb {
namespace game {


// ----------------------------------------------------------------------------
Agent* Selection::resolveAgent(Simulation& simulation) const
{
    if (kind != Kind::Agent)
        return nullptr;

    auto const it = simulation.getCities().find(city);
    if (it == simulation.getCities().end())
        return nullptr;

    for (auto& agent: it->second->getAgents())
    {
        if (agent->getId() == agentId)
            return agent.get();
    }

    return nullptr;
}
} // namespace game
} // namespace ogb
