//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/DebugState.hpp"
#include "OpenGlassBox/Simulation.hpp"

namespace ogb {

// ----------------------------------------------------------------------------
Agent* Selection::resolveAgent(Simulation& simulation) const
{
    if (kind != Kind::Agent)
        return nullptr;

    auto const it = simulation.cities().find(city);
    if (it == simulation.cities().end())
        return nullptr;

    for (auto& agent: it->second->agents())
    {
        if (agent->id() == agentId)
            return agent.get();
    }

    return nullptr;
}

} // namespace ogb
