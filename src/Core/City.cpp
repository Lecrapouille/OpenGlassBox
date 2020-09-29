#include "Core/City.hpp"

City::City(std::string const& name, Vector3f position, uint32_t gridSizeU, uint32_t gridSizeV)
    : m_name(name),
      m_position(position),
      m_gridSizeU(gridSizeU),
      m_gridSizeV(gridSizeV)
{}

City::City(std::string const& name, uint32_t gridSizeU, uint32_t gridSizeV)
    : City(name, Vector3f(0.0f, 0.0f, 0.0f), gridSizeU, gridSizeV)
{}

City::City(std::string const& name)
    : City(name, Vector3f(0.0f, 0.0f, 0.0f), 32u, 32u)
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

Map& City::addMap(MapType const& type)
{
    auto ptr = std::make_unique<Map>(type, m_gridSizeU, m_gridSizeV);
    Map& map = *ptr;
    m_maps[type.name] = std::move(ptr);
    return map;
}

Map& City::getMap(std::string const& id)
{
    return *m_maps.at(id);
}

Path& City::addPath(PathType const& type)
{
    m_paths[type.name] = std::make_unique<Path>(type);
    return *m_paths[type.name];
}

Path& City::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

Unit& City::addUnit(UnitType const& type, Node& node)
{
    m_units.push_back(std::make_unique<Unit>(/*m_nextUnitId++,*/ type, node, *this));
    return *m_units.back();
}

Unit& City::addUnit(UnitType const& type, Path& path, Way& way, float offset)
{
    Node& newNode = path.splitWay(way, offset);
    return addUnit(type, newNode);
}

Agent& City::addAgent(AgentType const& type, Unit& owner, Resources const& resources,
                      std::string const& searchTarget)
{
    m_agents.push_back(std::make_unique<Agent>(m_nextAgentId++, type, owner,
                                               resources, searchTarget));
    return *m_agents.back();
}

void City::removeAgent(Agent& agent)
{
    //m_agents.erase(std::remove(m_agents.begin(), m_agents.end(), &agent));
}
