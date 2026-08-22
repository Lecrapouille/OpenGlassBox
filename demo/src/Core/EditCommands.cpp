//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/EditCommands.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <cstdio>

namespace ogb {
namespace core {


// ----------------------------------------------------------------------------
//! \brief Look up a Path, returning nullptr rather than throwing: a command can
//! be replayed after the world was rebuilt, and the name may be gone.
// ----------------------------------------------------------------------------
static Path* findPath(Simulation& simulation, std::string const& cityName,
                      std::string const& pathName)
{
    auto const& cities = simulation.cities();
    auto it = cities.find(cityName);
    if (it == cities.end())
        return nullptr;

    auto& paths = it->second->paths();
    auto pathIt = paths.find(pathName);
    if (pathIt == paths.end())
        return nullptr;

    return pathIt->second.get();
}

// ----------------------------------------------------------------------------
static City* findCity(Simulation& simulation, std::string const& cityName)
{
    auto const& cities = simulation.cities();
    auto it = cities.find(cityName);
    return (it == cities.end()) ? nullptr : it->second.get();
}

// ----------------------------------------------------------------------------
//! \brief Closest node of the path within the radius, or nullptr. This is what
//! makes a road drawn by hand connect to the network instead of laying a
//! parallel graph a couple of pixels away.
// ----------------------------------------------------------------------------
static Node* snapToNode(Path& path, Vector3f const& position, float radius)
{
    Node* best = nullptr;
    float bestDistance = radius;

    for (auto const& node: path.nodes())
    {
        float const distance = magnitude(node->position() - position);
        if (distance <= bestDistance)
        {
            bestDistance = distance;
            best = node.get();
        }
    }

    return best;
}

// ----------------------------------------------------------------------------
Node* NodeRef::resolve(Simulation& simulation) const
{
    Path* p = findPath(simulation, city, path);
    return (p == nullptr) ? nullptr : p->node(id);
}

// =============================================================================
// COMMAND STACK
// =============================================================================

// ----------------------------------------------------------------------------
bool CommandStack::push(Simulation& simulation, CommandPtr command)
{
    if (!command->redo(simulation))
        return false;

    m_done.push_back(std::move(command));
    m_undone.clear();

    while (m_done.size() > CAPACITY)
    {
        m_done.pop_front();
    }

    return true;
}

// ----------------------------------------------------------------------------
void CommandStack::undo(Simulation& simulation)
{
    if (m_done.empty())
        return;

    CommandPtr command = std::move(m_done.back());
    m_done.pop_back();
    command->undo(simulation);
    m_undone.push_back(std::move(command));
}

// ----------------------------------------------------------------------------
void CommandStack::redo(Simulation& simulation)
{
    if (m_undone.empty())
        return;

    CommandPtr command = std::move(m_undone.back());
    m_undone.pop_back();

    // A redo that fails leaves the simulation as it was, so the command is
    // dropped rather than put back on a stack it can no longer describe.
    if (command->redo(simulation))
    {
        m_done.push_back(std::move(command));
    }
}

// ----------------------------------------------------------------------------
std::string CommandStack::undoLabel() const
{
    return m_done.empty() ? std::string() : m_done.back()->label();
}

// ----------------------------------------------------------------------------
std::string CommandStack::redoLabel() const
{
    return m_undone.empty() ? std::string() : m_undone.back()->label();
}

// ----------------------------------------------------------------------------
void CommandStack::clear()
{
    m_done.clear();
    m_undone.clear();
}

// ----------------------------------------------------------------------------
void CommandStack::takeHistory(std::deque<CommandPtr>& out)
{
    out = std::move(m_done);
    m_done.clear();
    m_undone.clear();
}

// =============================================================================
// ADD WAY
// =============================================================================

// ----------------------------------------------------------------------------
AddWayCommand::AddWayCommand(std::string city, std::string path,
                             std::string wayType, Vector3f from, Vector3f to,
                             float snapRadius)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_wayType(std::move(wayType)),
      m_from(from),
      m_to(to),
      m_snapRadius(snapRadius)
{}

// ----------------------------------------------------------------------------
bool AddWayCommand::redo(Simulation& simulation)
{
    Path* path = findPath(simulation, m_city, m_path);
    if (path == nullptr)
        return false;

    WayType const* type = nullptr;
    try
    {
        type = &simulation.script().getWayType(m_wayType);
    }
    catch (...)
    {
        return false;
    }

    // A zero length segment has no direction and an infinite curvature; the
    // router would divide by its length.
    if (magnitude(m_to - m_from) < 1e-3f)
        return false;

    // On the first run the engine picks the identifiers; the redos hand back
    // the ones it picked.
    auto reuseOrCreate = [path](uint32_t id, Vector3f const& position) -> Node& {
        return (id == NO_ID) ? path->addNode(position)
                             : path->addNode(id, position);
    };

    Node* from = snapToNode(*path, m_from, m_snapRadius);
    m_createdFrom = (from == nullptr);
    if (from == nullptr)
    {
        from = &reuseOrCreate(m_fromId, m_from);
    }
    m_fromId = from->id();

    Node* to = snapToNode(*path, m_to, m_snapRadius);
    m_createdTo = (to == nullptr);
    if (to == nullptr)
    {
        to = &reuseOrCreate(m_toId, m_to);
    }
    m_toId = to->id();

    if (from == to)
        return false;

    // Refuse a duplicate: the graph is simple, and a second segment between the
    // same two nodes would only split the traffic in a way nothing accounts for.
    if (from->getWayToNode(*to) != nullptr)
        return false;

    m_wayId = ((m_wayId == NO_ID) ? path->addWay(*type, *from, *to)
                                  : path->addWay(m_wayId, *type, *from, *to))
                  .id();

    return true;
}

// ----------------------------------------------------------------------------
void AddWayCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    Way* way = path->way(m_wayId);
    if (way != nullptr)
    {
        city->removeWay(*path, *way);
    }

    // Only take back the end points this command brought into existence, and
    // only while nothing else leans on them.
    auto dropIfCreated = [city, path](bool created, uint32_t id) {
        if (!created)
            return;

        Node* node = path->node(id);
        if ((node != nullptr) && !node->hasWays() && node->units().empty())
        {
            city->removeNode(*path, *node);
        }
    };
    dropIfCreated(m_createdFrom, m_fromId);
    dropIfCreated(m_createdTo, m_toId);
}

// ----------------------------------------------------------------------------
std::string AddWayCommand::label() const
{
    return "lay " + m_wayType;
}

// ----------------------------------------------------------------------------
void AddWayCommand::onWorldRebuilt()
{
    // The nodes and the segment were created by this command, so their
    // identifiers belong to a world that is gone. Reusing them would silently
    // address whatever the rebuild put at those numbers.
    m_wayId = NO_ID;
    m_fromId = NO_ID;
    m_toId = NO_ID;
    m_createdFrom = false;
    m_createdTo = false;
}

// =============================================================================
// ADD UNIT
// =============================================================================

// ----------------------------------------------------------------------------
AddUnitCommand::AddUnitCommand(std::string city, std::string path,
                               std::string unitType, uint32_t wayId,
                               float offset)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_unitType(std::move(unitType)),
      m_wayId(wayId),
      m_offset(offset)
{}

// ----------------------------------------------------------------------------
AddUnitCommand::AddUnitCommand(std::string city, std::string path,
                               std::string unitType, uint32_t nodeId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_unitType(std::move(unitType)),
      m_nodeId(nodeId)
{}

// ----------------------------------------------------------------------------
bool AddUnitCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return false;

    UnitType const* type = nullptr;
    try
    {
        type = &simulation.script().getUnitType(m_unitType);
    }
    catch (...)
    {
        return false;
    }

    Unit* unit = nullptr;

    if (m_wayId == NO_ID)
    {
        Node* node = path->node(m_nodeId);
        if (node == nullptr)
            return false;
        unit = &city->addUnit(*type, *node);
    }
    else
    {
        Way* way = path->way(m_wayId);
        if (way == nullptr)
            return false;
        unit = &city->addUnit(*type, *path, *way, m_offset);
    }

    m_unitId = unit->id();
    return true;
}

// ----------------------------------------------------------------------------
void AddUnitCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    for (auto& it: city->units())
    {
        if (it->id() == m_unitId)
        {
            city->removeUnit(*it);
            return;
        }
    }
}

// ----------------------------------------------------------------------------
std::string AddUnitCommand::label() const
{
    return "build " + m_unitType;
}

// ----------------------------------------------------------------------------
void AddUnitCommand::onWorldRebuilt()
{
    m_unitId = NO_ID;
}

// =============================================================================
// REMOVE UNIT
// =============================================================================

// ----------------------------------------------------------------------------
RemoveUnitCommand::RemoveUnitCommand(std::string city, std::string path,
                                     uint32_t id, std::string unitType,
                                     bool byUnitId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_id(id),
      m_unitType(std::move(unitType)),
      m_byUnitId(byUnitId)
{}

// ----------------------------------------------------------------------------
bool RemoveUnitCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return false;

    Unit* target = nullptr;
    if (m_byUnitId)
    {
        for (auto& it: city->units())
        {
            if (it->id() == m_id)
            {
                target = it.get();
                break;
            }
        }
    }
    else
    {
        Path* path = findPath(simulation, m_city, m_path);
        if (path == nullptr)
            return false;
        Node* node = path->node(m_id);
        if (node == nullptr)
            return false;
        for (auto* unit: node->units())
        {
            if (unit->type() == m_unitType)
            {
                target = unit;
                break;
            }
        }
    }

    if (target == nullptr)
        return false;

    m_onNode = (target->node() != nullptr);
    m_position = target->position();
    if (target->way() != nullptr)
    {
        m_wayId = target->way()->id();
        m_offset = target->wayOffset();
        Path* path = target->path();
        if (path != nullptr)
            m_path = path->type();
    }
    else if (target->node() != nullptr)
    {
        m_id = target->node()->id();
        m_onNode = true;
    }

    city->removeUnit(*target);
    return true;
}

// ----------------------------------------------------------------------------
void RemoveUnitCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    try
    {
        UnitType const& type = simulation.script().getUnitType(m_unitType);
        if (m_onNode)
        {
            Path* path = findPath(simulation, m_city, m_path);
            if (path == nullptr)
                return;
            Node* node = path->node(m_id);
            if (node == nullptr)
                return;
            city->addUnit(type, *node);
        }
        else if (m_wayId != NO_ID)
        {
            Path* path = findPath(simulation, m_city, m_path);
            if (path == nullptr)
                return;
            Way* way = path->way(m_wayId);
            if (way == nullptr)
                return;
            city->addUnit(type, *path, *way, m_offset);
        }
        else
        {
            city->addUnit(type, m_position);
        }
    }
    catch (...)
    {}
}

// ----------------------------------------------------------------------------
std::string RemoveUnitCommand::label() const
{
    return "demolish " + m_unitType;
}

// =============================================================================
// REMOVE WAY
// =============================================================================

// ----------------------------------------------------------------------------
RemoveWayCommand::RemoveWayCommand(std::string city, std::string path,
                                   uint32_t wayId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_wayId(wayId)
{}

// ----------------------------------------------------------------------------
bool RemoveWayCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return false;

    Way* way = path->way(m_wayId);
    if (way == nullptr)
        return false;

    // Copy what it takes to build the segment again before it is freed.
    m_wayType = way->type();
    m_fromId = way->from().id();
    m_toId = way->to().id();
    m_fromPosition = way->position1();
    m_toPosition = way->position2();
    m_captured = true;

    city->removeWay(*path, *way);

    return true;
}

// ----------------------------------------------------------------------------
void RemoveWayCommand::undo(Simulation& simulation)
{
    Path* path = findPath(simulation, m_city, m_path);
    if ((path == nullptr) || !m_captured)
        return;

    WayType const* type = nullptr;
    try
    {
        type = &simulation.script().getWayType(m_wayType);
    }
    catch (...)
    {
        return;
    }

    // The extremities survive a segment removal, but a command undone further
    // down the stack may since have taken them away.
    Node& from = path->addNode(m_fromId, m_fromPosition);
    Node& to = path->addNode(m_toId, m_toPosition);
    path->addWay(m_wayId, *type, from, to);
}

// ----------------------------------------------------------------------------
std::string RemoveWayCommand::label() const
{
    return m_captured ? ("bulldoze " + m_wayType) : std::string("bulldoze road");
}

// =============================================================================
// PAINT RESOURCE
// =============================================================================

// ----------------------------------------------------------------------------
PaintResourceCommand::PaintResourceCommand(std::string city, std::string map,
                                           int32_t u0, int32_t v0,
                                           int32_t u1, int32_t v1,
                                           uint32_t amount)
    : m_city(std::move(city)),
      m_map(std::move(map)),
      m_u0(std::min(u0, u1)),
      m_v0(std::min(v0, v1)),
      m_u1(std::max(u0, u1)),
      m_v1(std::max(v0, v1)),
      m_amount(amount)
{}

// ----------------------------------------------------------------------------
bool PaintResourceCommand::sameRectangle(int32_t u0, int32_t v0, int32_t u1,
                                         int32_t v1) const
{
    return (m_u0 == std::min(u0, u1)) && (m_v0 == std::min(v0, v1)) &&
           (m_u1 == std::max(u0, u1)) && (m_v1 == std::max(v0, v1));
}

// ----------------------------------------------------------------------------
bool PaintResourceCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return false;

    auto it = city->maps().find(m_map);
    if (it == city->maps().end())
        return false;

    Map& map = *it->second;

    // Painting is bounded by what the city administers, not by the map, which
    // now spans the whole world.
    MapRegion const region = city->region();
    int32_t const u0 = std::max(m_u0, region.u0);
    int32_t const v0 = std::max(m_v0, region.v0);
    int32_t const u1 = std::min(m_u1, region.u1() - 1);
    int32_t const v1 = std::min(m_v1, region.v1() - 1);
    if ((u0 > u1) || (v0 > v1))
        return false;

    // Only the first run records the previous content: a redo must not capture
    // the values its own undo just put back.
    bool const capture = m_previous.empty();

    for (int32_t v = v0; v <= v1; ++v)
    {
        for (int32_t u = u0; u <= u1; ++u)
        {
            if (capture)
            {
                m_previous.push_back(map.getResource(u, v));
            }
            map.setResource(u, v, m_amount);
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
void PaintResourceCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    auto it = city->maps().find(m_map);
    if (it == city->maps().end())
        return;

    Map& map = *it->second;

    MapRegion const region = city->region();
    int32_t const u0 = std::max(m_u0, region.u0);
    int32_t const v0 = std::max(m_v0, region.v0);
    int32_t const u1 = std::min(m_u1, region.u1() - 1);
    int32_t const v1 = std::min(m_v1, region.v1() - 1);

    size_t i = 0u;
    for (int32_t v = v0; (v <= v1) && (i < m_previous.size()); ++v)
    {
        for (int32_t u = u0; (u <= u1) && (i < m_previous.size()); ++u)
        {
            map.setResource(u, v, m_previous[i++]);
        }
    }
}

// ----------------------------------------------------------------------------
std::string PaintResourceCommand::label() const
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "paint %s to %u", m_map.c_str(),
                  m_amount);
    return buffer;
}

// =============================================================================
// ADD AREA
// =============================================================================

AddAreaCommand::AddAreaCommand(std::string city, std::string areaType,
                               int32_t u0, int32_t v0, int32_t u1, int32_t v1)
    : m_city(std::move(city)),
      m_areaType(std::move(areaType)),
      m_u0(std::min(u0, u1)),
      m_v0(std::min(v0, v1)),
      m_u1(std::max(u0, u1)),
      m_v1(std::max(v0, v1))
{}

bool AddAreaCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return false;

    AreaType const* type = nullptr;
    try
    {
        type = &simulation.script().getAreaType(m_areaType);
    }
    catch (...)
    {
        return false;
    }

    MapRegion region;
    region.u0 = m_u0;
    region.v0 = m_v0;
    region.sizeU = uint32_t(m_u1 - m_u0 + 1);
    region.sizeV = uint32_t(m_v1 - m_v0 + 1);

    Area& area = city->addArea(*type, region);
    m_areaId = area.id();
    return true;
}

void AddAreaCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    for (auto& it: city->areas())
    {
        if (it->id() == m_areaId)
        {
            city->removeArea(*it);
            return;
        }
    }
}

std::string AddAreaCommand::label() const
{
    return "zone " + m_areaType;
}
} // namespace core
} // namespace ogb
