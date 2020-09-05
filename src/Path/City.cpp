#include "City.hpp"
#include "Map.hpp"
#include "Unit.hpp"
#include "Path.hpp"
#include "Agent.hpp"

City::City(std::string const& id, uint32_t gridSizeX, uint32_t gridSizeY)
    : m_id(id),
      m_gridSizeX(gridSizeX),
      m_gridSizeY(gridSizeY)
{}

void City::update()
{
    size_t i = m_agents.size();
    while (i--) {
        m_agents[i]->move();
    }

    i = m_units.size();
    while (i--) {
        m_units[i]->executeRules();
    }

    for (auto& map: m_maps) {
        map.second->executeRules();
    }
}

Map& City::addMap(std::string const& id)
{
    auto map = std::make_shared<Map>(id, m_gridSizeX, m_gridSizeY);
    m_maps[id] = map;
    return *map;
}

Map& City::getMap(std::string const& id)
{
    return *m_maps.at(id);
}

Path& City::addPath(std::string const& id)
{
    auto path = std::make_shared<Path>(id, *this);
    m_paths[id] = path;
    return *path;
}

Path& City::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

Unit& City::addUnit(std::string const& id, Node& node)
{
    auto unit = std::make_shared<Unit>(id,/*m_nextUnitId++,*/ node);
    m_units.push_back(unit);
    return *unit;
}

Agent& City::addAgent(Node& node, Unit& owner,
                      Resources const& resources,
                      std::string const& searchTarget)
{
    auto agent =
       std::make_shared<Agent>(m_nextAgentId++, node, owner,
                               resources, searchTarget);
    m_agents.push_back(agent);
    return *agent;
}
