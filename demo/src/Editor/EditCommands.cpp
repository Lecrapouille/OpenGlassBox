//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Editor/EditCommands.hpp"
#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <algorithm>
#include <cstdio>

namespace ogb {
namespace editor {


// ----------------------------------------------------------------------------
//! \brief Look up a Path, returning nullptr rather than throwing: a command can
//! be replayed after the world was rebuilt, and the name may be gone.
// ----------------------------------------------------------------------------
static Path* findPath(Simulation& simulation, std::string const& cityName,
                      std::string const& pathName)
{
    auto const& cities = simulation.getCities();
    auto it = cities.find(cityName);
    if (it == cities.end())
        return nullptr;

    auto& paths = it->second->getPaths();
    auto pathIt = paths.find(pathName);
    if (pathIt == paths.end())
        return nullptr;

    return pathIt->second.get();
}

// ----------------------------------------------------------------------------
static City* findCity(Simulation& simulation, std::string const& cityName)
{
    auto const& cities = simulation.getCities();
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

    for (auto const& node: path.getNodes())
    {
        float const distance = length(node->getPosition() - position);
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
    return (p == nullptr) ? nullptr : p->findNode(id);
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
AddSegmentCommand::AddSegmentCommand(std::string city, std::string path,
                             std::string segmentType, Vector3f from, Vector3f to,
                             float snapRadius)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_segmentType(std::move(segmentType)),
      m_from(from),
      m_to(to),
      m_snapRadius(snapRadius)
{}

// ----------------------------------------------------------------------------
bool AddSegmentCommand::redo(Simulation& simulation)
{
    Path* path = findPath(simulation, m_city, m_path);
    if (path == nullptr)
    {
        // A city that never had a road of that kind has no graph to put one in.
        // Found it from the ruleset rather than refuse the segment.
        City* city = findCity(simulation, m_city);
        if (city == nullptr)
            return false;
        try
        {
            path = &city->addPath(simulation.getRuleset().getPathType(m_path));
        }
        catch (...)
        {
            return false;
        }
    }

    SegmentType const* type = nullptr;
    try
    {
        type = &simulation.getRuleset().getSegmentType(m_segmentType);
    }
    catch (...)
    {
        return false;
    }

    // A zero length segment has no direction and an infinite curvature; the
    // router would divide by its length.
    if (length(m_to - m_from) < 1e-3f)
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
    m_fromId = from->getId();

    Node* to = snapToNode(*path, m_to, m_snapRadius);
    m_createdTo = (to == nullptr);
    if (to == nullptr)
    {
        to = &reuseOrCreate(m_toId, m_to);
    }
    m_toId = to->getId();

    if (from == to)
        return false;

    // Refuse a duplicate: the graph is simple, and a second segment between the
    // same two nodes would only split the traffic in a way nothing accounts for.
    if (from->findSegmentTo(*to) != nullptr)
        return false;

    m_segmentId = ((m_segmentId == NO_ID) ? path->addSegment(*type, *from, *to)
                                  : path->addSegment(m_segmentId, *type, *from, *to))
                  .getId();

    return true;
}

// ----------------------------------------------------------------------------
void AddSegmentCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    Segment* segment = path->findSegment(m_segmentId);
    if (segment != nullptr)
    {
        city->removeSegment(*path, *segment);
    }

    // Only take back the end points this command brought into existence, and
    // only while nothing else leans on them.
    auto dropIfCreated = [city, path](bool created, uint32_t id) {
        if (!created)
            return;

        Node* node = path->findNode(id);
        if ((node != nullptr) && !node->hasSegments() && node->getUnits().empty())
        {
            city->removeNode(*path, *node);
        }
    };
    dropIfCreated(m_createdFrom, m_fromId);
    dropIfCreated(m_createdTo, m_toId);
}

// ----------------------------------------------------------------------------
std::string AddSegmentCommand::label() const
{
    return "lay " + m_segmentType;
}

// ----------------------------------------------------------------------------
void AddSegmentCommand::onWorldRebuilt()
{
    // The nodes and the segment were created by this command, so their
    // identifiers belong to a world that is gone. Reusing them would silently
    // address whatever the rebuild put at those numbers.
    m_segmentId = NO_ID;
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
                               std::string unitType, uint32_t segmentId,
                               float offset)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_unitType(std::move(unitType)),
      m_segmentId(segmentId),
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
        type = &simulation.getRuleset().getUnitType(m_unitType);
    }
    catch (...)
    {
        return false;
    }

    Unit* unit = nullptr;

    if (m_segmentId == NO_ID)
    {
        Node* node = path->findNode(m_nodeId);
        if (node == nullptr)
            return false;
        unit = &city->addUnit(*type, *node);
    }
    else
    {
        Segment* segment = path->findSegment(m_segmentId);
        if (segment == nullptr)
            return false;

        // Cutting the segment turns the spot into a junction, which is what
        // makes the building an address: agents stop at nodes.
        m_segmentType = segment->getTypeName().str();
        uint32_t const fromId = segment->getFrom().getId();
        uint32_t const toId = segment->getTo().getId();

        Node& junction = city->splitSegment(*path, *segment, m_offset);

        m_junctionId = NO_ID;
        m_secondHalfId = NO_ID;
        if ((junction.getId() != fromId) && (junction.getId() != toId))
        {
            m_junctionId = junction.getId();
            for (Segment const* incident: junction.getSegments())
            {
                if (incident->getId() != m_segmentId)
                    m_secondHalfId = incident->getId();
            }
        }

        unit = &city->addUnit(*type, junction);
    }

    m_unitId = unit->getId();
    return true;
}

// ----------------------------------------------------------------------------
void AddUnitCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    for (auto& it: city->getUnits())
    {
        if (it->getId() == m_unitId)
        {
            city->removeUnit(*it);
            break;
        }
    }

    mergeBack(simulation);
}

// ----------------------------------------------------------------------------
void AddUnitCommand::mergeBack(Simulation& simulation)
{
    if (m_junctionId == NO_ID)
        return;

    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    Node* junction = path->findNode(m_junctionId);
    Segment* first = path->findSegment(m_segmentId);
    Segment* second = path->findSegment(m_secondHalfId);
    if ((junction == nullptr) || (first == nullptr) || (second == nullptr))
        return;

    // Sewing the halves back is only harmless while nothing else came to lean
    // on them: another building, or a road drawn from the junction.
    if (!junction->getUnits().empty() || (junction->getSegments().size() != 2u))
        return;
    if (!first->getUnits().empty() || !second->getUnits().empty())
        return;

    SegmentType const* type = nullptr;
    try
    {
        type = &simulation.getRuleset().getSegmentType(m_segmentType);
    }
    catch (...)
    {
        return;
    }

    Node const& a = (&first->getFrom() == junction) ? first->getTo() : first->getFrom();
    Node const& b = (&second->getFrom() == junction) ? second->getTo() : second->getFrom();
    uint32_t const fromId = a.getId();
    uint32_t const toId = b.getId();
    Vector3f const fromPosition = a.getPosition();
    Vector3f const toPosition = b.getPosition();

    // The two halves go with the junction, and either end may be left an
    // orphan and swept away, so both are named back into existence before the
    // segment is laid again with the identifier it had.
    city->removeNode(*path, *junction);
    Node& from = path->addNode(fromId, fromPosition);
    Node& to = path->addNode(toId, toPosition);
    path->addSegment(m_segmentId, *type, from, to);

    m_junctionId = NO_ID;
    m_secondHalfId = NO_ID;
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
    m_junctionId = NO_ID;
    m_secondHalfId = NO_ID;
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
        for (auto& it: city->getUnits())
        {
            if (it->getId() == m_id)
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
        Node* node = path->findNode(m_id);
        if (node == nullptr)
            return false;
        for (auto* unit: node->getUnits())
        {
            if (unit->getTypeName() == m_unitType)
            {
                target = unit;
                break;
            }
        }
    }

    if (target == nullptr)
        return false;

    m_onNode = (target->getNode() != nullptr);
    m_position = target->getPosition();
    if (target->getSegment() != nullptr)
    {
        m_segmentId = target->getSegment()->getId();
        m_offset = target->getSegmentOffset();
        Path* path = target->getPath();
        if (path != nullptr)
            m_path = path->getTypeName().str();
    }
    else if (target->getNode() != nullptr)
    {
        m_id = target->getNode()->getId();
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
        UnitType const& type = simulation.getRuleset().getUnitType(m_unitType);
        if (m_onNode)
        {
            Path* path = findPath(simulation, m_city, m_path);
            if (path == nullptr)
                return;
            Node* node = path->findNode(m_id);
            if (node == nullptr)
                return;
            city->addUnit(type, *node);
        }
        else if (m_segmentId != NO_ID)
        {
            Path* path = findPath(simulation, m_city, m_path);
            if (path == nullptr)
                return;
            Segment* segment = path->findSegment(m_segmentId);
            if (segment == nullptr)
                return;
            city->addUnit(type, *path, *segment, m_offset);
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
RemoveSegmentCommand::RemoveSegmentCommand(std::string city, std::string path,
                                   uint32_t segmentId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_segmentId(segmentId)
{}

// ----------------------------------------------------------------------------
bool RemoveSegmentCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return false;

    Segment* segment = path->findSegment(m_segmentId);
    if (segment == nullptr)
        return false;

    // Copy what it takes to build the segment again before it is freed.
    m_segmentType = segment->getTypeName().str();
    m_fromId = segment->getFrom().getId();
    m_toId = segment->getTo().getId();
    m_fromPosition = segment->getFromPosition();
    m_toPosition = segment->getToPosition();
    m_captured = true;

    city->removeSegment(*path, *segment);

    return true;
}

// ----------------------------------------------------------------------------
void RemoveSegmentCommand::undo(Simulation& simulation)
{
    Path* path = findPath(simulation, m_city, m_path);
    if ((path == nullptr) || !m_captured)
        return;

    SegmentType const* type = nullptr;
    try
    {
        type = &simulation.getRuleset().getSegmentType(m_segmentType);
    }
    catch (...)
    {
        return;
    }

    // The extremities survive a segment removal, but a command undone further
    // down the stack may since have taken them away.
    Node& from = path->addNode(m_fromId, m_fromPosition);
    Node& to = path->addNode(m_toId, m_toPosition);
    path->addSegment(m_segmentId, *type, from, to);
}

// ----------------------------------------------------------------------------
std::string RemoveSegmentCommand::label() const
{
    return m_captured ? ("bulldoze " + m_segmentType) : std::string("bulldoze road");
}

// =============================================================================
// REMOVE NODE
// =============================================================================

// ----------------------------------------------------------------------------
RemoveNodeCommand::RemoveNodeCommand(std::string city, std::string path,
                                     uint32_t nodeId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_nodeId(nodeId)
{}

// ----------------------------------------------------------------------------
bool RemoveNodeCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return false;

    Node* node = path->findNode(m_nodeId);
    if (node == nullptr)
        return false;

    m_position = node->getPosition();
    m_segments.clear();
    for (Segment* segment: node->getSegments())
    {
        SegmentSnapshot snapshot;
        snapshot.id = segment->getId();
        snapshot.type = segment->getTypeName().str();
        snapshot.fromId = segment->getFrom().getId();
        snapshot.toId = segment->getTo().getId();
        snapshot.fromPosition = segment->getFromPosition();
        snapshot.toPosition = segment->getToPosition();
        m_segments.push_back(std::move(snapshot));
    }
    m_captured = true;
    city->removeNode(*path, *node);
    return true;
}

// ----------------------------------------------------------------------------
void RemoveNodeCommand::undo(Simulation& simulation)
{
    Path* path = findPath(simulation, m_city, m_path);
    if ((path == nullptr) || !m_captured)
        return;

    path->addNode(m_nodeId, m_position);
    for (SegmentSnapshot const& snapshot: m_segments)
    {
        SegmentType const* type = nullptr;
        try
        {
            type = &simulation.getRuleset().getSegmentType(snapshot.type);
        }
        catch (...)
        {
            continue;
        }

        Node& from = path->addNode(snapshot.fromId, snapshot.fromPosition);
        Node& to = path->addNode(snapshot.toId, snapshot.toPosition);
        path->addSegment(snapshot.id, *type, from, to);
    }
}

// ----------------------------------------------------------------------------
std::string RemoveNodeCommand::label() const
{
    return "bulldoze node";
}

// =============================================================================
// PAINT RESOURCE
// =============================================================================

// ----------------------------------------------------------------------------
PaintResourceCommand::PaintResourceCommand(std::string city, std::string layer,
                                           int32_t u0, int32_t v0,
                                           int32_t u1, int32_t v1,
                                           uint32_t amount)
    : m_city(std::move(city)),
      m_layer(std::move(layer)),
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

    auto it = city->getLayers().find(m_layer);
    if (it == city->getLayers().end())
        return false;

    Layer& layer = *it->second;

    // Painting is bounded by what the city administers, not by the layer, which
    // now spans the whole world.
    CellRegion const region = city->getRegion();
    int32_t const u0 = std::max(m_u0, region.u0);
    int32_t const v0 = std::max(m_v0, region.v0);
    int32_t const u1 = std::min(m_u1, region.getMaxU() - 1);
    int32_t const v1 = std::min(m_v1, region.getMaxV() - 1);
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
                m_previous.push_back(layer.getResource({ u, v }));
            }
            layer.setResource({ u, v }, m_amount);
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

    auto it = city->getLayers().find(m_layer);
    if (it == city->getLayers().end())
        return;

    Layer& layer = *it->second;

    CellRegion const region = city->getRegion();
    int32_t const u0 = std::max(m_u0, region.u0);
    int32_t const v0 = std::max(m_v0, region.v0);
    int32_t const u1 = std::min(m_u1, region.getMaxU() - 1);
    int32_t const v1 = std::min(m_v1, region.getMaxV() - 1);

    size_t i = 0u;
    for (int32_t v = v0; (v <= v1) && (i < m_previous.size()); ++v)
    {
        for (int32_t u = u0; (u <= u1) && (i < m_previous.size()); ++u)
        {
            layer.setResource({ u, v }, m_previous[i++]);
        }
    }
}

// ----------------------------------------------------------------------------
std::string PaintResourceCommand::label() const
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "paint %s to %u", m_layer.c_str(),
                  m_amount);
    return buffer;
}

// =============================================================================
// ADD AREA
// =============================================================================

namespace {

bool regionsOverlap(CellRegion const& a, CellRegion const& b)
{
    return (a.u0 < b.getMaxU()) && (b.u0 < a.getMaxU()) && (a.v0 < b.getMaxV()) && (b.v0 < a.getMaxV());
}

CellRegion makeRegion(int32_t u0, int32_t v0, int32_t u1, int32_t v1)
{
    CellRegion region;
    region.u0 = u0;
    region.v0 = v0;
    region.sizeU = uint32_t(u1 - u0);
    region.sizeV = uint32_t(v1 - v0);
    return region;
}

// ----------------------------------------------------------------------------
//! \brief What is left of \c from once \c cut is taken out of it, as up to four
//! rectangles: the band above, the band below, then the left and right sides of
//! what remains in between.
// ----------------------------------------------------------------------------
std::vector<CellRegion> subtract(CellRegion const& from, CellRegion const& cut)
{
    std::vector<CellRegion> pieces;
    if (!regionsOverlap(from, cut))
    {
        pieces.push_back(from);
        return pieces;
    }

    int32_t const v0 = std::max(from.v0, cut.v0);
    int32_t const v1 = std::min(from.getMaxV(), cut.getMaxV());

    if (from.v0 < v0)
        pieces.push_back(makeRegion(from.u0, from.v0, from.getMaxU(), v0));
    if (v1 < from.getMaxV())
        pieces.push_back(makeRegion(from.u0, v1, from.getMaxU(), from.getMaxV()));
    if (from.u0 < cut.u0)
        pieces.push_back(makeRegion(from.u0, v0, std::min(from.getMaxU(), cut.u0), v1));
    if (cut.getMaxU() < from.getMaxU())
        pieces.push_back(makeRegion(std::max(from.u0, cut.getMaxU()), v0, from.getMaxU(), v1));

    return pieces;
}

} // namespace

AddZoneCommand::AddZoneCommand(std::string city, std::string zoneType,
                               int32_t u0, int32_t v0, int32_t u1, int32_t v1)
    : m_city(std::move(city)),
      m_zoneType(std::move(zoneType)),
      m_u0(std::min(u0, u1)),
      m_v0(std::min(v0, v1)),
      m_u1(std::max(u0, u1)),
      m_v1(std::max(v0, v1))
{}

bool AddZoneCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return false;

    ZoneType const* type = nullptr;
    try
    {
        type = &simulation.getRuleset().getZoneType(m_zoneType);
    }
    catch (...)
    {
        return false;
    }

    CellRegion const region = makeRegion(m_u0, m_v0, m_u1 + 1, m_v1 + 1);
    if (region.isEmpty())
        return false;

    // The first run records what it re-zoned; a redo runs after an undo put the
    // original Zones back, so the cutting itself has to happen every time.
    bool const capture = m_removed.empty();
    std::vector<SavedZone> overlapped;

    size_t i = city->getZones().size();
    while (i--)
    {
        Zone& zone = *city->getZones()[i];
        if (!regionsOverlap(zone.getRegion(), region))
            continue;

        SavedZone saved;
        saved.type = zone.getTypeName().str();
        saved.u0 = zone.getRegion().u0;
        saved.v0 = zone.getRegion().v0;
        saved.sizeU = zone.getRegion().sizeU;
        saved.sizeV = zone.getRegion().sizeV;
        overlapped.push_back(saved);
        if (capture)
            m_removed.push_back(std::move(saved));
        city->removeZone(zone);
    }

    m_leftovers.clear();
    for (SavedZone const& saved: overlapped)
    {
        ZoneType const* savedType = nullptr;
        try
        {
            savedType = &simulation.getRuleset().getZoneType(saved.type);
        }
        catch (...)
        {
            continue;
        }

        CellRegion const footprint = makeRegion(
            saved.u0, saved.v0, saved.u0 + int32_t(saved.sizeU),
            saved.v0 + int32_t(saved.sizeV));
        for (CellRegion const& piece: subtract(footprint, region))
        {
            if (piece.isEmpty())
                continue;
            m_leftovers.push_back(city->addZone(*savedType, piece).getId());
        }
    }

    Zone& zone = city->addZone(*type, region);
    m_zoneId = zone.getId();
    return true;
}

void AddZoneCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    auto const dropById = [city](uint32_t id) {
        for (auto& it: city->getZones())
        {
            if (it->getId() == id)
            {
                city->removeZone(*it);
                return;
            }
        }
    };

    dropById(m_zoneId);
    for (uint32_t id: m_leftovers)
        dropById(id);
    m_leftovers.clear();

    for (SavedZone const& saved: m_removed)
    {
        try
        {
            ZoneType const& type = simulation.getRuleset().getZoneType(saved.type);
            city->addZone(type, makeRegion(saved.u0, saved.v0,
                                           saved.u0 + int32_t(saved.sizeU),
                                           saved.v0 + int32_t(saved.sizeV)));
        }
        catch (...)
        {}
    }
}

std::string AddZoneCommand::label() const
{
    return "zone " + m_zoneType;
}
} // namespace editor
} // namespace ogb
