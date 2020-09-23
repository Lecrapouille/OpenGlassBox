#include "Core/City.hpp"
#include "Core/Map.hpp"
#include "Core/Unit.hpp"
#include "Core/Path.hpp"
#include "Core/Agent.hpp"
#include "Core/Config.hpp"

City::City(std::string const& name, uint32_t gridSizeX, uint32_t gridSizeY)
    : m_name(name),
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

Map& City::addMap(MapType const& type)
{
    auto ptr = std::make_unique<Map>(type, m_gridSizeX, m_gridSizeY);
    Map& map = *ptr;
    m_maps[type.m_type] = std::move(ptr);
    return map;
}

Map& City::getMap(std::string const& id)
{
    return *m_maps.at(id);
}

Path& City::addPath(PathType const& type)
{
    m_paths[type.m_name] = std::make_unique<Path>(type);
    return *m_paths[type.m_name];
}

Path& City::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

Unit& City::addUnit(UnitType const& type, Node& node)
{
    m_units.push_back(std::make_unique<Unit>(type,/*m_nextUnitId++,*/ node, *this));
    return *m_units.back();
}

Agent& City::addAgent(AgentType const& type, Unit& owner, Resources const& resources)
{
    m_agents.push_back(std::make_unique<Agent>(m_nextAgentId++, type, owner, resources));
    return *m_agents.back();
}

void City::removeAgent(Agent& agent)
{
    //m_agents.erase(std::remove(m_agents.begin(), m_agents.end(), &agent));
}
