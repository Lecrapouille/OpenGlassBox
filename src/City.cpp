#include "City.hpp"
#include "Map.hpp"
#include "Unit.hpp"
#include "Path.hpp"
#include "Agent.hpp"
#include "Config.hpp"

City::City(std::string const& id, uint32_t gridSizeX, uint32_t gridSizeY)
    : m_id(id),
      m_gridSizeX(gridSizeX),
      m_gridSizeY(gridSizeY)
{}

void City::update()
{
    size_t i = m_agents.size();
    while (i--) {
        m_agents[i]->move(*this);
    }

    i = m_units.size();
    while (i--) {
        m_units[i]->executeRules();
    }

    for (auto& map: m_maps) {
        map.second->executeRules();
    }
}

void City::world2mapPosition(Vector3f worldPos, uint32_t& u, uint32_t& v)
{
    float x = worldPos.x / config::GRID_SIZE;
    float y = worldPos.y / config::GRID_SIZE;

    if (x < 0.0f)
        u = 0u;
    else if (uint32_t(x) >= m_gridSizeX)
        u = m_gridSizeX - 1u;
    else
        u = uint32_t(x);

    if (y < 0.0f)
        v = 0u;
    else if (uint32_t(y) >= m_gridSizeY)
        v = m_gridSizeY - 1u;
    else
        v = uint32_t(y);
}

Map& City::addMap(std::string const& id)
{
    auto ptr = std::make_unique<Map>(id, m_gridSizeX, m_gridSizeY);
    Map& map = *ptr;
    m_maps[id] = std::move(ptr);
    return map;
}

Map& City::getMap(std::string const& id)
{
    return *m_maps.at(id);
}

Path& City::addPath(std::string const& id)
{
    auto ptr = std::make_unique<Path>(id);
    Path& path = *ptr;
    m_paths[id] = std::move(ptr);
    return path;
}

Path& City::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

Unit& City::addUnit(std::string const& id, Node& node)
{
    auto ptr = std::make_unique<Unit>(id,/*m_nextUnitId++,*/ node);
    Unit& unit = *ptr;
    m_units.push_back(std::move(ptr));
    return unit;
}

Agent& City::addAgent(Node& node, Unit& owner,
                      Resources const& resources,
                      std::string const& searchTarget)
{
    auto ptr =
       std::make_unique<Agent>(m_nextAgentId++, node, owner,
                               resources, searchTarget);
    Agent& agent = *ptr;
    m_agents.push_back(std::move(ptr));
    return agent;
}

void City::removeAgent(Agent& agent)
{
    //m_agents.erase(std::remove(m_agents.begin(), m_agents.end(), &agent));
}
