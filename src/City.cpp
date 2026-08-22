//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/World.hpp"
#include <algorithm>

// -----------------------------------------------------------------------------
namespace ogb {

City::City(std::string const& name, Vector3f position, uint32_t sizeU,
           uint32_t sizeV, World& world)
    : m_name(name),
      m_world(world),
      m_position(position),
      m_gridSizeU(sizeU),
      m_gridSizeV(sizeV)
{
    static City::Listener listener;
    setListener(listener);

    // Seed zero keeps the non-reproducible seeding done by Dijkstra itself.
    if (config().randomSeed != 0u)
    {
        m_dijkstra.setRandomSeed(config().randomSeed);
    }
}

// -----------------------------------------------------------------------------
void City::setListener(City::Listener& listener)
{
    m_listener = &listener;
}

// -----------------------------------------------------------------------------
SimulationConfig const& City::config() const
{
    return m_world.config();
}

// -----------------------------------------------------------------------------
SimulationConfig& City::config()
{
    return m_world.config();
}

// -----------------------------------------------------------------------------
float City::gridCellSize() const
{
    return m_world.cellSize();
}

// -----------------------------------------------------------------------------
Maps& City::maps()
{
    return m_world.maps();
}

// -----------------------------------------------------------------------------
Maps const& City::maps() const
{
    return m_world.maps();
}

// -----------------------------------------------------------------------------
MapRegion City::region() const
{
    int32_t u0, v0;
    m_world.world2mapPosition(m_position, u0, v0);

    return MapRegion{ u0, v0, m_gridSizeU, m_gridSizeV };
}

// -----------------------------------------------------------------------------
void City::update(float dt)
{
    // Advance the time averaged traffic before the Agents move, so that they
    // all route on the same picture of the network during this tick.
    for (auto& it: m_paths)
    {
        it.second->smoothFlows(config().trafficSmoothing);
    }

    // Start from the last element for easy removing of the Agent
    size_t i = m_agents.size();
    while (i--)
    {
        if (m_agents[i]->update(m_dijkstra, dt))
        {
            std::swap(m_agents[i], m_agents[m_agents.size() - 1u]);
            m_agents.pop_back();
        }
    }

    i = m_areas.size();
    while (i--) {
        m_areas[i]->executeRules();
    }

    i = m_units.size();
    while (i--) {
        m_units[i]->executeRules();
    }
}

// -----------------------------------------------------------------------------
// Since Units are attached to a Path Node
// They are directly translated.
void City::translate(Vector3f const direction)
{
    m_position += direction;

    for (auto& it: m_paths)
    {
        it.second->translate(direction);
    }

    for (auto& it: m_agents)
    {
        it->translate(direction);
    }

    for (auto& it: m_units)
    {
        it->translate(direction);
    }
}

// -----------------------------------------------------------------------------
void City::world2mapPosition(Vector3f worldPos, int32_t& u, int32_t& v) const
{
    m_world.world2mapPosition(worldPos, u, v);

    // A Unit built just outside the region still has to act on cells this City
    // administers, so bring it back inside rather than let it write into the
    // territory of a neighbour.
    region().clamp(u, v);
}

// -----------------------------------------------------------------------------
Map& City::addMap(MapType const& type)
{
    Map& map = m_world.addMap(type);
    m_listener->onMapAdded(map);
    return map;
}

// -----------------------------------------------------------------------------
Map& City::getMap(std::string const& id)
{
    return m_world.getMap(id);
}

// -----------------------------------------------------------------------------
Path& City::addPath(PathType const& type)
{
    Path& path = *(m_paths[type.name] = std::make_unique<Path>(type));
    m_listener->onPathAdded(path);
    return path;
}

// -----------------------------------------------------------------------------
Path& City::getPath(std::string const& id)
{
    return *m_paths.at(id);
}

// -----------------------------------------------------------------------------
Unit& City::addUnit(UnitType const& type, Node& node)
{
    m_units.push_back(std::make_unique<Unit>(type, node, *this));
    Unit& unit = *(m_units.back());
    unit.setId(m_nextUnitId++);
    m_listener->onUnitAdded(unit);
    return unit;
}

// -----------------------------------------------------------------------------
Unit& City::addUnit(UnitType const& type, Path& /*path*/, Way& way, float offset)
{
    m_units.push_back(std::make_unique<Unit>(type, way, offset, *this));
    Unit& unit = *(m_units.back());
    unit.setId(m_nextUnitId++);
    m_listener->onUnitAdded(unit);
    return unit;
}

// -----------------------------------------------------------------------------
Unit& City::addUnit(UnitType const& type, Vector3f const& position)
{
    m_units.push_back(std::make_unique<Unit>(type, position, *this));
    Unit& unit = *(m_units.back());
    unit.setId(m_nextUnitId++);
    m_listener->onUnitAdded(unit);
    return unit;
}

// -----------------------------------------------------------------------------
Area& City::addArea(AreaType const& type, MapRegion const& footprint)
{
    m_areas.push_back(std::make_unique<Area>(m_nextAreaId++, type, footprint, *this));
    Area& area = *(m_areas.back());
    m_listener->onAreaAdded(area);
    return area;
}

// -----------------------------------------------------------------------------
Agent& City::addAgent(AgentType const& type, Unit& owner, Resources const& resources,
                      std::string const& searchTarget)
{
    m_agents.push_back(std::make_unique<Agent>(m_nextAgentId++, type, owner, resources,
                                               searchTarget));
    Agent& agent = *(m_agents.back());
    agent.setConfig(config());
    m_listener->onAgentAdded(agent);
    return agent;
}

// -----------------------------------------------------------------------------
void City::removeUnit(Unit& unit)
{
    m_listener->onUnitRemoved(unit);
    unit.detach();

    m_units.erase(std::remove_if(m_units.begin(), m_units.end(),
                                 [&unit](std::unique_ptr<Unit> const& it) {
                                     return it.get() == &unit;
                                 }),
                  m_units.end());
}

// -----------------------------------------------------------------------------
void City::removeArea(Area& area)
{
    m_listener->onAreaRemoved(area);
    m_areas.erase(std::remove_if(m_areas.begin(), m_areas.end(),
                                 [&area](std::unique_ptr<Area> const& it) {
                                     return it.get() == &area;
                                 }),
                  m_areas.end());
}

// -----------------------------------------------------------------------------
//! \brief Destroy every Agent for which the predicate holds. Factored out
//! because the two demolition entry points differ only by that predicate.
// -----------------------------------------------------------------------------
template<class Predicate>
static void removeAgentsIf(Agents& agents, City::Listener& listener,
                           Predicate predicate)
{
    size_t i = agents.size();
    while (i--)
    {
        if (predicate(*agents[i]))
        {
            listener.onAgentRemoved(*agents[i]);
            std::swap(agents[i], agents[agents.size() - 1u]);
            agents.pop_back();
        }
    }
}

// -----------------------------------------------------------------------------
void City::removeWay(Path& path, Way& way)
{
    size_t i = m_units.size();
    while (i--)
    {
        if (m_units[i]->way() == &way)
            removeUnit(*m_units[i]);
    }

    removeAgentsIf(m_agents, *m_listener,
                   [&way](Agent const& agent) { return agent.uses(way); });

    path.removeWay(way);
}

// -----------------------------------------------------------------------------
void City::removeNode(Path& path, Node& node)
{
    size_t i = m_units.size();
    while (i--)
    {
        if (m_units[i]->node() == &node)
        {
            removeUnit(*m_units[i]);
        }
    }

    removeAgentsIf(m_agents, *m_listener, [&node](Agent const& agent) {
        if (agent.uses(node))
            return true;

        // Also catch the Agents halfway along a segment that is about to go
        // down with the node.
        for (auto const& way: node.ways())
        {
            if (agent.uses(*way))
                return true;
        }

        return false;
    });

    path.removeNode(node);
}

} // namespace ogb
