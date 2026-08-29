//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

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
