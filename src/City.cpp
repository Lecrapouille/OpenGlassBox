//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/Config.hpp"
#include "OpenGlassBox/Vector.hpp"
#include "OpenGlassBox/World.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
namespace ogb
{

City::City(std::string const& name,
           Vector3f const& position,
           uint32_t sizeU,
           uint32_t sizeV,
           World& world)
    : m_name(name),
      m_world(world),
      m_position(position),
      m_gridSizeU(sizeU),
      m_gridSizeV(sizeV)
{
    updateRegion();

    static City::Listener defaultListener;
    setListener(defaultListener);
}

// -----------------------------------------------------------------------------
void City::setRouter(std::unique_ptr<IRouter> router)
{
    m_router = std::move(router);
}

// -----------------------------------------------------------------------------
void City::setListener(City::Listener& listener)
{
    m_listener = &listener;
}

// -----------------------------------------------------------------------------
Config const& City::getConfig() const
{
    return m_world.getConfig();
}

// -----------------------------------------------------------------------------
SimulationClock const& City::getClock() const
{
    return m_world.getClock();
}

// -----------------------------------------------------------------------------
float City::getCellSize() const
{
    return m_world.getCellSize();
}

// -----------------------------------------------------------------------------
Layers const& City::getLayers() const
{
    return m_world.getLayers();
}

// -----------------------------------------------------------------------------
void City::updateRegion()
{
    Cell const origin = m_world.worldToCell(m_position);
    m_region = CellRegion{ origin.u, origin.v, m_gridSizeU, m_gridSizeV };
}

// -----------------------------------------------------------------------------
void City::update(float dt)
{
    // Advance the time averaged traffic before the Agents move, so that they
    // all route on the same picture of the network during this tick.
    for (auto const& it : m_paths)
    {
        it.second->updateTrafficSmoothing(getConfig().traffic.smoothing);
    }

    // Start from the last element for easy removing of the Agent
    size_t i = m_agents.size();
    while (i--)
    {
        if (m_router &&
            m_agents[i]->update(*m_router, getConfig().routing, dt))
        {
            std::swap(m_agents[i], m_agents[m_agents.size() - 1u]);
            m_agents.pop_back();
        }
    }

    i = m_zones.size();
    while (i--)
    {
        m_zones[i]->executeRules();
    }

    i = m_buildings.size();
    while (i--)
    {
        m_buildings[i]->executeRules();
    }
}

// -----------------------------------------------------------------------------
void City::update()
{
    update(getConfig().time.tickDuration());
}

// -----------------------------------------------------------------------------
// Since buildings are attached to a Path Node
// They are directly translated.
void City::translate(Vector3f const& direction)
{
    m_position += direction;
    updateRegion();

    for (auto& it : m_paths)
    {
        it.second->translate(direction);
    }

    for (auto const& it : m_agents)
    {
        it->translate(direction);
    }

    for (auto const& it : m_buildings)
    {
        it->translate(direction);
    }
}

// -----------------------------------------------------------------------------
Cell City::worldToCell(Vector3f const& position) const
{
    // A Building built just outside the region still has to act on cells this City
    // administers, so bring it back inside rather than let it write into the
    // territory of a neighbour.
    return m_region.clamp(m_world.worldToCell(position));
}

// -----------------------------------------------------------------------------
Vector3f City::cellToWorld(Cell cell) const
{
    return m_world.cellToWorld(cell);
}

// -----------------------------------------------------------------------------
Layer& City::addLayer(LayerType const& type)
{
    Layer& layer = m_world.addLayer(type);
    m_listener->onLayerAdded(layer);
    return layer;
}

// -----------------------------------------------------------------------------
Layer& City::getLayer(std::string const& name)
{
    Layer* layer = m_world.findLayer(name);
    if (layer == nullptr)
        throw std::out_of_range(name);
    return *layer;
}

// -----------------------------------------------------------------------------
Path& City::addPath(PathType const& type)
{
    Path& path = *(m_paths[type.name.str()] = std::make_unique<Path>(type));
    m_listener->onPathAdded(path);
    return path;
}

// -----------------------------------------------------------------------------
Path& City::getPath(std::string const& name)
{
    return *m_paths.at(name);
}

// -----------------------------------------------------------------------------
Building& City::addBuilding(BuildingType const& type, Node& node)
{
    m_buildings.push_back(std::make_unique<Building>(type, node, *this));
    Building& building = *(m_buildings.back());
    building.setId(m_nextBuildingId++);
    building.spreadRuleStart();
    m_listener->onBuildingAdded(building);
    return building;
}

// -----------------------------------------------------------------------------
Building& City::addBuilding(BuildingType const& type,
                    Path& /*path*/,
                    Segment& segment,
                    float offset)
{
    m_buildings.push_back(std::make_unique<Building>(type, segment, offset, *this));
    Building& building = *(m_buildings.back());
    building.setId(m_nextBuildingId++);
    building.spreadRuleStart();
    m_listener->onBuildingAdded(building);
    return building;
}

// -----------------------------------------------------------------------------
Building& City::addBuilding(BuildingType const& type, Vector3f const& position)
{
    m_buildings.push_back(std::make_unique<Building>(type, position, *this));
    Building& building = *(m_buildings.back());
    building.setId(m_nextBuildingId++);
    building.spreadRuleStart();
    m_listener->onBuildingAdded(building);
    return building;
}

// -----------------------------------------------------------------------------
Zone& City::addZone(ZoneType const& type, CellRegion const& footprint)
{
    m_zones.push_back(
        std::make_unique<Zone>(m_nextZoneId++, type, footprint, *this));
    Zone& zone = *(m_zones.back());
    m_listener->onZoneAdded(zone);
    return zone;
}

// -----------------------------------------------------------------------------
Agent& City::addAgent(AgentType const& type,
                      Building& owner,
                      Resources const& resources,
                      Name const& searchTarget)
{
    m_agents.push_back(std::make_unique<Agent>(
        m_nextAgentId++, type, owner, resources, searchTarget));
    Agent& agent = *(m_agents.back());
    m_listener->onAgentAdded(agent);
    return agent;
}

// -----------------------------------------------------------------------------
void City::removeBuilding(Building& building)
{
    // The Agents point at the building both as the one that sent them out and
    // as the destination of their itinerary. Both have to go before it does.
    for (auto const& agent : m_agents)
    {
        agent->forget(building);
    }

    m_listener->onBuildingRemoved(building);
    building.detach();

    m_buildings.erase(std::remove_if(m_buildings.begin(),
                                 m_buildings.end(),
                                 [&building](std::unique_ptr<Building> const& it)
                                 { return it.get() == &building; }),
                  m_buildings.end());
}

// -----------------------------------------------------------------------------
void City::removeZone(Zone& zone)
{
    m_listener->onZoneRemoved(zone);
    m_zones.erase(std::remove_if(m_zones.begin(),
                                 m_zones.end(),
                                 [&zone](std::unique_ptr<Zone> const& it)
                                 { return it.get() == &zone; }),
                  m_zones.end());
}

// -----------------------------------------------------------------------------
void City::removeStuckAgents()
{
    size_t i = m_agents.size();
    while (i--)
    {
        if (!m_agents[i]->isStuck())
            continue;

        m_listener->onAgentRemoved(*m_agents[i]);
        std::swap(m_agents[i], m_agents[m_agents.size() - 1u]);
        m_agents.pop_back();
    }
}

// -----------------------------------------------------------------------------
void City::removeSegment(Path& path, Segment& segment)
{
    City* neighbor = m_world.findCityAt(segment.getFrom().getPosition());
    if ((neighbor == nullptr) || (neighbor == this))
        neighbor = m_world.findCityAt(segment.getTo().getPosition());
    if ((neighbor != nullptr) && (neighbor != this) &&
        !m_world.getListener().allowSegmentRemoved(*this, *neighbor, segment))
    {
        return;
    }

    // The Agents hold raw pointers on the segment and on the Nodes, so they
    // have to let go before anything is freed. They keep their position: the
    // next tick routes them again over the graph that remains.
    for (auto const& agent : m_agents)
    {
        agent->forget(segment);
        agent->invalidateRoute();
    }

    size_t i = m_buildings.size();
    while (i--)
    {
        if (m_buildings[i]->getSegment() == &segment)
            removeBuilding(*m_buildings[i]);
    }

    path.removeSegment(segment);
    removeIsolatedNodes(path);
    removeStuckAgents();
}

// -----------------------------------------------------------------------------
Node& City::splitSegment(Path& path, Segment& segment, float offset)
{
    if (offset <= 0.0f)
        return segment.getFrom();
    if (offset >= 1.0f)
        return segment.getTo();

    // Where the buildings stand has to be read before the segment is shortened,
    // and moveOntoSegment() mutates the very list being walked.
    std::vector<std::pair<Building*, float>> anchored;
    anchored.reserve(segment.getBuildings().size());
    for (Building* building : segment.getBuildings())
        anchored.emplace_back(building, building->getSegmentOffset());

    // The Agents hold the segment and an offset along it, both of which mean
    // something else once it is half as long. They keep their position and the
    // next tick routes them over the graph as it now is.
    for (auto const& agent : m_agents)
    {
        agent->forget(segment);
        agent->invalidateRoute();
    }

    Node& junction = path.splitSegment(segment, offset);

    // splitSegment() keeps the first half in place and creates the second one.
    Segment* second = nullptr;
    for (Segment* incident : junction.getSegments())
    {
        if (incident != &segment)
            second = incident;
    }

    for (auto const& it : anchored)
    {
        if (it.second <= offset)
            it.first->moveOntoSegment(segment, it.second / offset);
        else if (second != nullptr)
            it.first->moveOntoSegment(*second, (it.second - offset) / (1.0f - offset));
    }

    return junction;
}

// -----------------------------------------------------------------------------
void City::removeIsolatedNodes(Path& path) const
{
    size_t i = path.getNodes().size();
    while (i--)
    {
        Node& node = *path.getNodes()[i];
        if (node.hasSegments())
            continue;
        if (!node.getBuildings().empty())
            continue;

        for (auto const& agent : m_agents)
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
    while (!m_buildings.empty())
        removeBuilding(*m_buildings.back());
    while (!m_zones.empty())
        removeZone(*m_zones.back());

    // The graphs are emptied but kept: which kinds of network exist comes from
    // the ruleset, not from what the player drew. Dropping them would leave the
    // City with no road type to lay a road with.
    for (auto& it : m_paths)
        it.second->clear();

    m_globals = Resources();
}

// -----------------------------------------------------------------------------
void City::removeNode(Path& path, Node& node)
{
    std::vector<Segment*> incident = node.getSegments();

    // Segments first, then the Node: an Agent parked on the Node is only
    // detached from the graph once nothing carries it any more.
    for (auto const& agent : m_agents)
    {
        for (Segment const* segment : incident)
            agent->forget(*segment);
        agent->forget(node);
        agent->invalidateRoute();
    }

    size_t i = m_buildings.size();
    while (i--)
    {
        Building& building = *m_buildings[i];
        if (building.getNode() == &node)
        {
            removeBuilding(building);
            continue;
        }
        for (Segment const* segment : incident)
        {
            if (building.getSegment() == segment)
            {
                removeBuilding(building);
                break;
            }
        }
    }

    path.removeNode(node);

    // The far ends of the segments that went with it may have become orphans.
    removeIsolatedNodes(path);
    removeStuckAgents();
}

} // namespace ogb
