//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/City.hpp"

// -----------------------------------------------------------------------------
City::City(std::string const& name, Vector3f position, uint32_t gridSizeU, uint32_t gridSizeV)
    : m_name(name),
      m_position(position),
      m_gridSizeU(gridSizeU),
      m_gridSizeV(gridSizeV)
{}

// -----------------------------------------------------------------------------
City::City(std::string const& name, uint32_t gridSizeU, uint32_t gridSizeV)
    : City(name, Vector3f(0.0f, 0.0f, 0.0f), gridSizeU, gridSizeV)
{}

// -----------------------------------------------------------------------------
City::City(std::string const& name)
    : City(name, Vector3f(0.0f, 0.0f, 0.0f), 32u, 32u)
{}

// -----------------------------------------------------------------------------
void City::update()
{
    // Start from the last element for easy removing of the Agent
    size_t i = m_agents.size();
    while (i--)
    {
        if (m_agents[i]->update(*this))
        {
            // Swap the position of the Agent with the last in the vector and
            // remove the last element of the vector.
            std::swap(m_agents[i], m_agents[m_agents.size() - 1u]);
            m_agents.pop_back();
        }
    }

    i = m_units.size();
    while (i--) {
        m_units[i]->executeRules();
    }

    for (auto& map: m_maps) {
        map.second->executeRules();
    }
}

// -----------------------------------------------------------------------------
// Since Units are attached to a Path Node
// They are directly translated.
void City::translate(Vector3f const direction)
{
    m_position += direction;

    for (auto& it: m_maps)
    {
        it.second->translate(direction);
    }

    for (auto& it: m_paths)
    {
        it.second->translate(direction);
    }

    for (auto& it: m_agents)
    {
        it->translate(direction);
    }
}

// -----------------------------------------------------------------------------
void City::world2mapPosition(Vector3f worldPos, uint32_t& u, uint32_t& v)
{
    float x = (worldPos.x - m_position.x) / config::GRID_SIZE;
    float y = (worldPos.y - m_position.y) / config::GRID_SIZE;

    if (x < 0.0f)
        u = 0u;
    else if (uint32_t(x) >= m_gridSizeU)
        u = m_gridSizeU - 1u;
    else
        u = uint32_t(x);

    if (y < 0.0f)
        v = 0u;
    else if (uint32_t(y) >= m_gridSizeV)
        v = m_gridSizeV - 1u;
    else
        v = uint32_t(y);
}

// -----------------------------------------------------------------------------
Map& City::addMap(MapType const& type)
{
    m_maps[type.name] = std::make_unique<Map>(type, *this);
    return *m_maps[type.name];
}

// -----------------------------------------------------------------------------
Map& City::getMap(std::string const& id)
{
    return *m_maps.at(id);
}

// -----------------------------------------------------------------------------
Path& City::addPath(PathType const& type)
{
    m_paths[type.name] = std::make_unique<Path>(type);
    return *m_paths[type.name];
}

// -----------------------------------------------------------------------------
Path& City::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

// -----------------------------------------------------------------------------
Unit& City::addUnit(UnitType const& type, Node& node)
{
    m_units.push_back(std::make_unique<Unit>(/*m_nextUnitId++,*/ type, node, *this));
    return *m_units.back();
}

// -----------------------------------------------------------------------------
Unit& City::addUnit(UnitType const& type, Path& path, Way& way, float offset)
{
    Node& newNode = path.splitWay(way, offset);
    return addUnit(type, newNode);
}

// -----------------------------------------------------------------------------
Agent& City::addAgent(AgentType const& type, Unit& owner, Resources const& resources,
                      std::string const& searchTarget)
{
    m_agents.push_back(std::make_unique<Agent>(m_nextAgentId++, type, owner,
                                               resources, searchTarget));
    return *m_agents.back();
}
