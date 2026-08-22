//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/World.hpp"
#include "OpenGlassBox/Vector.hpp"
#include <algorithm>
#include <vector>

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
    for (auto& agent: m_agents)
    {
        if (agent->owner() == &unit)
            agent->detachOwner();
    }

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
void City::dropStrandedAgents()
{
    size_t i = m_agents.size();
    while (i--)
    {
        if (!m_agents[i]->stranded())
            continue;

        m_listener->onAgentRemoved(*m_agents[i]);
        std::swap(m_agents[i], m_agents[m_agents.size() - 1u]);
        m_agents.pop_back();
    }
}

// -----------------------------------------------------------------------------
void City::removeWay(Path& path, Way& way)
{
    City* neighbor = m_world.cityAt(way.from().position());
    if ((neighbor == nullptr) || (neighbor == this))
        neighbor = m_world.cityAt(way.to().position());
    if ((neighbor != nullptr) && (neighbor != this) &&
        !m_world.listener().allowWayRemoved(*this, *neighbor, way))
    {
        return;
    }

    // The Agents hold raw pointers on the segment and on the Nodes, so they
    // have to let go before anything is freed. They keep their position: the
    // next tick routes them again over the graph that remains.
    for (auto& agent: m_agents)
    {
        agent->forget(way);
        agent->invalidateRoute();
    }

    size_t i = m_units.size();
    while (i--)
    {
        if (m_units[i]->way() == &way)
            removeUnit(*m_units[i]);
    }

    path.removeWay(way);
    removeOrphanNodes(path);
    dropStrandedAgents();
}

// -----------------------------------------------------------------------------
void City::removeOrphanNodes(Path& path)
{
    size_t i = path.nodes().size();
    while (i--)
    {
        Node& node = *path.nodes()[i];
        if (node.hasWays())
            continue;
        if (!node.units().empty())
            continue;

        for (auto& agent: m_agents)
            agent->forget(node);

        path.removeNode(node);
    }
}

// -----------------------------------------------------------------------------
void City::clear()
{
    while (!m_agents.empty())
    {
        m_listener->onAgentRemoved(*m_agents.back());
        m_agents.pop_back();
    }
    while (!m_units.empty())
        removeUnit(*m_units.back());
    while (!m_areas.empty())
        removeArea(*m_areas.back());

    // The graphs are emptied but kept: which kinds of network exist comes from
    // the ruleset, not from what the player drew. Dropping them would leave the
    // City with no road type to lay a road with.
    for (auto& it: m_paths)
        it.second->clear();

    m_globals = Resources();
}

// -----------------------------------------------------------------------------
void City::removeNode(Path& path, Node& node)
{
    std::vector<Way*> incident = node.ways();

    // Segments first, then the Node: an Agent parked on the Node is only
    // detached from the graph once nothing carries it any more.
    for (auto& agent: m_agents)
    {
        for (Way* way: incident)
            agent->forget(*way);
        agent->forget(node);
        agent->invalidateRoute();
    }

    size_t i = m_units.size();
    while (i--)
    {
        Unit& unit = *m_units[i];
        if (unit.node() == &node)
        {
            removeUnit(unit);
            continue;
        }
        for (Way* way: incident)
        {
            if (unit.way() == way)
            {
                removeUnit(unit);
                break;
            }
        }
    }

    path.removeNode(node);

    // The far ends of the segments that went with it may have become orphans.
    removeOrphanNodes(path);
    dropStrandedAgents();
}

} // namespace ogb
