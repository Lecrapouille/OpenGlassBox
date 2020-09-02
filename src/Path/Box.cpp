#include "Box.hpp"

Box::Box(std::string const& id, uint32_t gridSizeX, uint32_t gridSizeY)
    : m_id(id),
      m_gridSizeX(gridSizeX),
      m_gridSizeY(gridSizeY)
{}

// TODO simulation

void Box::update()
{
    size_t i = m_agents.size();
    while (i--) {
        m_agents[i].move();
    }

    i = m_units.size();
    while (i--) {
        m_units[i].executeRules();
    }

    for (auto& map: m_maps) {
        map.executeRules();
    }
}

Map& Box::addMap(std::string const& id)
{
    std::make_shared<Map> map =
            std::make_shared<Map>(id, this, m_gridSizeX, m_gridSizeY);
    m_maps[id] = map;
    return *map;
}

Map& Box::getMap(std::string const& id)
{
    return *m_maps.at(id);
}

Map& Box::addPath(std::string const& id)
{
    std::make_shared<Path> path = std::make_shared<Path>(id, this);
    m_paths[id] = path;
    return *path;
}

Path& Box::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

Unit& Box::addUnit(std::string const& id, Node const& node)
{
    std::make_shared<Unit> unit =
            std::make_shared<Unit>(id, m_nextUnitId++, node);
    m_units.push_back(unit);
    return *unit;
}

Agent& Box::addAgent(uint32_t id, Node const& node, Unit& owner,
                     ResourceBin const& resources, string const& searchTarget)
{
    std::make_shared<Agent> agent =
            std::make_shared<Agent>(agentType, m_nextAgentId++, node, owner,
                                    resources, searchTarget);
    m_agents.push_back(agent);
    return *agent;
}
