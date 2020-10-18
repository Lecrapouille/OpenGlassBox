//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "main.hpp"

// TODO: use ImGUI to log events

void GlassBox::onCityAdded(City& city)
{
    std::cout << "City " << city.name() << " added" << std::endl;

    // TODO: not the faster algorithm but quick to write
    m_cityNames.clear();
    for (auto const& it: m_simulation.cities())
    {
        m_cityNames.push_back(it.second->name());
    }
}

void GlassBox::onCityRemoved(City& city)
{
    std::cout << "City " << city.name() << " removed" << std::endl;

    m_cityNames.clear();
    for (auto const& it: m_simulation.cities())
    {
        m_cityNames.push_back(it.second->name());
    }
}

void GlassBox::onMapAdded(Map& map)
{
    std::cout << "Map " << map.type() << " added" << std::endl;
}

void GlassBox::onMapRemoved(Map& map)
{
    std::cout << "Map " << map.type() << " removed" << std::endl;
}

void GlassBox::onPathAdded(Path& path)
{
    std::cout << "Path " << path.type() << " added" << std::endl;
}

void GlassBox::onPathRemoved(Path& path)
{
    std::cout << "Path " << path.type() << " removed" << std::endl;
}

void GlassBox::onUnitAdded(Unit& unit)
{
    std::cout << "Unit " << unit.type() << " "
              << unit.id() << " added" << std::endl;
}

void GlassBox::onUnitRemoved(Unit& unit)
{
    std::cout << "Unit " << unit.type() << " "
              << unit.id() << " removed" << std::endl;
}

void GlassBox::onAgentAdded(Agent& agent)
{
    std::cout << "Agent " << agent.type() << " "
              << agent.id() << " added" << std::endl;
}

void GlassBox::onAgentRemoved(Agent& agent)
{
    std::cout << "Agent " << agent.type() << " "
              << agent.id() << " removed" << std::endl;
}
