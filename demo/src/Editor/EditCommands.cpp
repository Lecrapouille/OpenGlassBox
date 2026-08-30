//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Editor/EditCommands.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "OpenGlassBox/Zone.hpp"

#include <algorithm>
#include <cstdio>

namespace ogb::editor
{

// ----------------------------------------------------------------------------
//! \brief Look up a Path, returning nullptr rather than throwing: a command can
//! be replayed after the world was rebuilt, and the name may be gone.
// ----------------------------------------------------------------------------
static Path* findPath(Simulation const& simulation,
                      std::string const& cityName,
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
static City* findCity(Simulation const& simulation, std::string const& cityName)
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
static Node*
snapToNode(Path const& path, Vector3f const& position, float radius)
{
    Node* best = nullptr;
    float bestDistance = radius;

    for (auto const& node : path.getNodes())
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
//! \brief Cut a segment in two at the junction the caller wants there.
//! \param[out] cut what the cut created, or nothing to undo when the offset
//! landed on an end of the segment and the crossroads was there already.
//! \return the junction, cut or found.
// ----------------------------------------------------------------------------
static Node& cutSegment(City& city,
                        Path& path,
                        Segment& segment,
                        float offset,
                        SegmentCut& cut)
{
    cut = SegmentCut();
    cut.type = segment.getTypeName().str();

    size_t const segmentId = segment.getId();
    size_t const fromId = segment.getFrom().getId();
    size_t const toId = segment.getTo().getId();

    Node& junction = city.splitSegment(path, segment, offset);

    // Landing on an end means the crossroads was already there and the graph is
    // unchanged, which leaves an undo nothing to sew back.
    if ((junction.getId() == fromId) || (junction.getId() == toId))
    {
        cut = SegmentCut();
        return junction;
    }

    cut.junctionId = junction.getId();
    cut.firstId = segmentId;
    for (Segment const* incident : junction.getSegments())
    {
        if (incident->getId() != segmentId)
            cut.secondId = incident->getId();
    }

    return junction;
}

// ----------------------------------------------------------------------------
//! \brief Sew the two halves of a cut segment back into one.
//!
//! Every undo of an edit that cut a segment comes through here. It is only
//! harmless while nothing came to lean on the junction in the meantime: a
//! building, or a road drawn from it. When something did, the junction stays,
//! which leaves the player a crossroads they did not ask for rather than
//! taking away what they did.
// ----------------------------------------------------------------------------
static void sewSegment(Simulation const& simulation,
                       City& city,
                       Path& path,
                       SegmentCut const& cut)
{
    if (cut.junctionId == NO_ID)
        return;

    Node* junction = path.findNode(cut.junctionId);
    Segment const* first = path.findSegment(cut.firstId);
    Segment const* second = path.findSegment(cut.secondId);
    if ((junction == nullptr) || (first == nullptr) || (second == nullptr))
        return;

    if (!junction->getBuildings().empty() ||
        (junction->getSegments().size() != 2u))
        return;
    if (!first->getBuildings().empty() || !second->getBuildings().empty())
        return;

    SegmentType const* type = nullptr;
    try
    {
        type = &simulation.getRuleset().getSegmentType(cut.type);
    }
    catch (...)
    {
        return;
    }

    Node const& a =
        (&first->getFrom() == junction) ? first->getTo() : first->getFrom();
    Node const& b =
        (&second->getFrom() == junction) ? second->getTo() : second->getFrom();
    size_t const fromId = a.getId();
    size_t const toId = b.getId();
    Vector3f const fromPosition = a.getPosition();
    Vector3f const toPosition = b.getPosition();

    // The two halves go with the junction, and either end may be left an
    // orphan and swept away, so both are named back into existence before the
    // segment is laid again with the identifier it had.
    city.removeNode(path, *junction);
    Node& from = path.addNode(fromId, fromPosition);
    Node& to = path.addNode(toId, toPosition);
    path.addSegment(cut.firstId, *type, from, to);
}

// ----------------------------------------------------------------------------
//! \brief Take a node back, as long as nothing came to lean on it. Used both by
//! the undos and by a command that gives up half way through.
// ----------------------------------------------------------------------------
static void dropIfOrphan(City& city, Path& path, size_t id)
{
    Node* node = path.findNode(id);
    if ((node != nullptr) && !node->hasSegments() &&
        node->getBuildings().empty())
    {
        city.removeNode(path, *node);
    }
}

// ----------------------------------------------------------------------------
Node* NodeRef::resolve(Simulation const& simulation) const
{
    Path const* p = findPath(simulation, city, path);
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
AddSegmentCommand::AddSegmentCommand(std::string city,
                                     std::string path,
                                     std::string segmentType,
                                     Vector3f from,
                                     Vector3f to,
                                     float snapRadius)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_segmentType(std::move(segmentType)),
      m_from(from),
      m_to(to),
      m_snapRadius(snapRadius)
{
}

// ----------------------------------------------------------------------------
bool AddSegmentCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return false;

    Path* path = findPath(simulation, m_city, m_path);
    if (path == nullptr)
    {
        // A city that never had a road of that kind has no graph to put one in.
        // Found it from the ruleset rather than refuse the segment.
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

    m_cuts.clear();

    // On the first run the engine picks the identifiers; the redos hand back
    // the ones it picked.
    auto reuseOrCreate = [path](size_t id, Vector3f const& position) -> Node&
    {
        return (id == NO_ID) ? path->addNode(position)
                             : path->addNode(id, position);
    };

    // An end of the road may land on a node, in the middle of another road, or
    // on empty ground. Landing on a road makes a T junction there: without the
    // cut the two only looked joined, and no agent could turn from one into the
    // other.
    auto resolveEnd =
        [&](Vector3f const& position, size_t& id, bool& created) -> Node*
    {
        Node* node = snapToNode(*path, position, m_snapRadius);
        if (node != nullptr)
        {
            created = false;
            id = node->getId();
            return node;
        }

        float offset = 0.0f;
        Segment* under = path->findSegmentAt(position, m_snapRadius, offset);
        if (under != nullptr)
        {
            SegmentCut cut;
            Node& junction = cutSegment(*city, *path, *under, offset, cut);
            if (cut.junctionId != NO_ID)
                m_cuts.push_back(cut);
            created = false;
            id = junction.getId();
            return &junction;
        }

        created = true;
        node = &reuseOrCreate(id, position);
        id = node->getId();
        return node;
    };

    // Resolving the ends already cut roads and made nodes, so giving up here
    // has to leave the graph as it was found rather than a crossroads short of
    // a road to justify it.
    auto giveUp = [&]()
    {
        for (auto it = m_cuts.rbegin(); it != m_cuts.rend(); ++it)
        {
            sewSegment(simulation, *city, *path, *it);
        }
        m_cuts.clear();
        if (m_createdFrom)
            dropIfOrphan(*city, *path, m_fromId);
        if (m_createdTo)
            dropIfOrphan(*city, *path, m_toId);
        return false;
    };

    Node* from = resolveEnd(m_from, m_fromId, m_createdFrom);
    Node* to = resolveEnd(m_to, m_toId, m_createdTo);

    if (from == to)
        return giveUp();

    // Refuse a duplicate: the graph is simple, and a second segment between the
    // same two nodes would only split the traffic in a way nothing accounts
    // for.
    if (from->findSegmentTo(*to) != nullptr)
        return giveUp();

    // A road drawn over another makes a crossroads there, so the roads it runs
    // over are cut and it is laid one piece at a time between the junctions.
    // Splitting keeps every segment already found: the graph holds them in a
    // deque and a cut only shortens one and adds another.
    std::vector<Crossing> const crossings =
        path->findCrossings(from->getPosition(), to->getPosition());

    std::vector<Node*> chain;
    chain.reserve(crossings.size() + 2u);
    chain.push_back(from);

    for (Crossing const& crossing : crossings)
    {
        SegmentCut cut;
        Node& junction = cutSegment(
            *city, *path, *crossing.segment, crossing.segmentOffset, cut);

        if (cut.junctionId != NO_ID)
            m_cuts.push_back(cut);
        chain.push_back(&junction);
    }
    chain.push_back(to);

    std::vector<size_t> const reuse = m_pieceIds;
    m_pieceIds.clear();
    for (size_t i = 1u; i < chain.size(); ++i)
    {
        Node& a = *chain[i - 1u];
        Node& b = *chain[i];

        // Two crossings on the same spot, or a junction that turned out to be
        // an end of the road being drawn, leave nothing to lay in between.
        if ((&a == &b) || (a.findSegmentTo(b) != nullptr))
            continue;

        size_t const index = m_pieceIds.size();
        size_t const id = (index < reuse.size()) ? reuse[index] : NO_ID;
        Segment const& piece = (id == NO_ID)
                                   ? path->addSegment(*type, a, b)
                                   : path->addSegment(id, *type, a, b);
        m_pieceIds.push_back(piece.getId());
    }

    return !m_pieceIds.empty();
}

// ----------------------------------------------------------------------------
void AddSegmentCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    for (size_t id : m_pieceIds)
    {
        Segment* segment = path->findSegment(id);
        if (segment != nullptr)
        {
            city->removeSegment(*path, *segment);
        }
    }

    // The roads that were cut come back whole, once the pieces that made the
    // crossroads worth having are gone. Last cut sewn first, so that a road cut
    // twice is put back one half at a time.
    for (auto it = m_cuts.rbegin(); it != m_cuts.rend(); ++it)
    {
        sewSegment(simulation, *city, *path, *it);
    }
    m_cuts.clear();

    // Only take back the end points this command brought into existence, and
    // only while nothing else leans on them.
    if (m_createdFrom)
        dropIfOrphan(*city, *path, m_fromId);
    if (m_createdTo)
        dropIfOrphan(*city, *path, m_toId);
}

// ----------------------------------------------------------------------------
std::string AddSegmentCommand::label() const
{
    return "lay " + m_segmentType;
}

// ----------------------------------------------------------------------------
void AddSegmentCommand::onWorldRebuilt()
{
    // The nodes and the segments were created by this command, so their
    // identifiers belong to a world that is gone. Reusing them would silently
    // address whatever the rebuild put at those numbers.
    m_pieceIds.clear();
    m_cuts.clear();
    m_fromId = NO_ID;
    m_toId = NO_ID;
    m_createdFrom = false;
    m_createdTo = false;
}

// =============================================================================
// SPLIT WAY
// =============================================================================

// ----------------------------------------------------------------------------
SplitSegmentCommand::SplitSegmentCommand(std::string city,
                                         std::string path,
                                         size_t segmentId,
                                         float offset)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_segmentId(segmentId),
      m_offset(offset)
{
}

// ----------------------------------------------------------------------------
bool SplitSegmentCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return false;

    Segment* segment = path->findSegment(m_segmentId);
    if (segment == nullptr)
        return false;

    cutSegment(*city, *path, *segment, m_offset, m_cut);

    // An offset on an end asked for a crossroads that was already there.
    return m_cut.junctionId != NO_ID;
}

// ----------------------------------------------------------------------------
void SplitSegmentCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    sewSegment(simulation, *city, *path, m_cut);
    m_cut = SegmentCut();
}

// ----------------------------------------------------------------------------
std::string SplitSegmentCommand::label() const
{
    return "add a node";
}

// =============================================================================
// MOVE NODE
// =============================================================================

// ----------------------------------------------------------------------------
MoveNodeCommand::MoveNodeCommand(std::string city,
                                 std::string path,
                                 size_t nodeId,
                                 Vector3f from,
                                 Vector3f to)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_nodeId(nodeId),
      m_from(from),
      m_to(to)
{
}

// ----------------------------------------------------------------------------
void MoveNodeCommand::place(Simulation const& simulation,
                            Vector3f const& position) const
{
    City* city = findCity(simulation, m_city);
    Path const* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    Node* node = path->findNode(m_nodeId);
    if (node == nullptr)
        return;

    city->moveNode(*node, position);
}

// ----------------------------------------------------------------------------
bool MoveNodeCommand::redo(Simulation& simulation)
{
    Path const* path = findPath(simulation, m_city, m_path);
    if ((path == nullptr) || (path->findNode(m_nodeId) == nullptr))
        return false;

    // A move of no distance is not worth an entry in the history: a click on a
    // node that does not drag it anywhere is how a player looks at one.
    if (length(m_to - m_from) < 1e-3f)
        return false;

    place(simulation, m_to);
    return true;
}

// ----------------------------------------------------------------------------
void MoveNodeCommand::undo(Simulation& simulation)
{
    place(simulation, m_from);
}

// ----------------------------------------------------------------------------
std::string MoveNodeCommand::label() const
{
    return "move a node";
}

// =============================================================================
// ADD UNIT
// =============================================================================

// ----------------------------------------------------------------------------
AddBuildingCommand::AddBuildingCommand(std::string city,
                                       std::string path,
                                       std::string buildingType,
                                       size_t segmentId,
                                       float offset)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_buildingType(std::move(buildingType)),
      m_segmentId(segmentId),
      m_offset(offset)
{
}

// ----------------------------------------------------------------------------
AddBuildingCommand::AddBuildingCommand(std::string city,
                                       std::string path,
                                       std::string buildingType,
                                       size_t nodeId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_buildingType(std::move(buildingType)),
      m_nodeId(nodeId)
{
}

// ----------------------------------------------------------------------------
bool AddBuildingCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return false;

    BuildingType const* type = nullptr;
    try
    {
        type = &simulation.getRuleset().getBuildingType(m_buildingType);
    }
    catch (...)
    {
        return false;
    }

    Building const* building = nullptr;

    if (m_segmentId == NO_ID)
    {
        Node* node = path->findNode(m_nodeId);
        if (node == nullptr)
            return false;
        building = &city->addBuilding(*type, *node);
    }
    else
    {
        Segment* segment = path->findSegment(m_segmentId);
        if (segment == nullptr)
            return false;

        // Cutting the segment turns the spot into a junction, which is what
        // makes the building an address: agents stop at nodes.
        Node& junction = cutSegment(*city, *path, *segment, m_offset, m_cut);

        building = &city->addBuilding(*type, junction);
    }

    m_buildingId = building->getId();
    return true;
}

// ----------------------------------------------------------------------------
void AddBuildingCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    for (auto const& building : city->getBuildings())
    {
        if (building->getId() == m_buildingId)
        {
            city->removeBuilding(*building);
            break;
        }
    }

    mergeBack(simulation);
}

// ----------------------------------------------------------------------------
void AddBuildingCommand::mergeBack(Simulation const& simulation)
{
    City* city = findCity(simulation, m_city);
    Path* path = findPath(simulation, m_city, m_path);
    if ((city == nullptr) || (path == nullptr))
        return;

    sewSegment(simulation, *city, *path, m_cut);
    m_cut = SegmentCut();
}

// ----------------------------------------------------------------------------
std::string AddBuildingCommand::label() const
{
    return "build " + m_buildingType;
}

// ----------------------------------------------------------------------------
void AddBuildingCommand::onWorldRebuilt()
{
    m_buildingId = NO_ID;
    m_cut = SegmentCut();
}

// =============================================================================
// REMOVE UNIT
// =============================================================================

// ----------------------------------------------------------------------------
RemoveBuildingCommand::RemoveBuildingCommand(std::string city,
                                             std::string path,
                                             size_t id,
                                             std::string buildingType,
                                             bool byBuildingId)
    : m_city(std::move(city)),
      m_path(std::move(path)),
      m_id(id),
      m_buildingType(std::move(buildingType)),
      m_byBuildingId(byBuildingId)
{
}

// ----------------------------------------------------------------------------
bool RemoveBuildingCommand::redo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return false;

    Building* target = nullptr;
    if (m_byBuildingId)
    {
        for (auto const& building : city->getBuildings())
        {
            if (building->getId() == m_id)
            {
                target = building.get();
                break;
            }
        }
    }
    else
    {
        Path const* path = findPath(simulation, m_city, m_path);
        if (path == nullptr)
            return false;
        Node const* node = path->findNode(m_id);
        if (node == nullptr)
            return false;
        for (auto* building : node->getBuildings())
        {
            if (building->getTypeName() == m_buildingType)
            {
                target = building;
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
        Path const* path = target->getPath();
        if (path != nullptr)
            m_path = path->getTypeName().str();
    }
    else if (target->getNode() != nullptr)
    {
        m_id = target->getNode()->getId();
        m_onNode = true;
    }

    city->removeBuilding(*target);
    return true;
}

// ----------------------------------------------------------------------------
void RemoveBuildingCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    try
    {
        BuildingType const& type =
            simulation.getRuleset().getBuildingType(m_buildingType);
        if (m_onNode)
        {
            Path const* path = findPath(simulation, m_city, m_path);
            if (path == nullptr)
                return;
            Node* node = path->findNode(m_id);
            if (node == nullptr)
                return;
            city->addBuilding(type, *node);
        }
        else if (m_segmentId != NO_ID)
        {
            Path* path = findPath(simulation, m_city, m_path);
            if (path == nullptr)
                return;
            Segment* segment = path->findSegment(m_segmentId);
            if (segment == nullptr)
                return;
            city->addBuilding(type, *path, *segment, m_offset);
        }
        else
        {
            city->addBuilding(type, m_position);
        }
    }
    catch (...)
    {
        // do nothing
    }
}

// ----------------------------------------------------------------------------
std::string RemoveBuildingCommand::label() const
{
    return "demolish " + m_buildingType;
}

// =============================================================================
// REMOVE WAY
// =============================================================================

// ----------------------------------------------------------------------------
RemoveSegmentCommand::RemoveSegmentCommand(std::string city,
                                           std::string path,
                                           size_t segmentId)
    : m_city(std::move(city)), m_path(std::move(path)), m_segmentId(segmentId)
{
}

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
    return m_captured ? ("bulldoze " + m_segmentType)
                      : std::string("bulldoze road");
}

// =============================================================================
// REMOVE NODE
// =============================================================================

// ----------------------------------------------------------------------------
RemoveNodeCommand::RemoveNodeCommand(std::string city,
                                     std::string path,
                                     size_t nodeId)
    : m_city(std::move(city)), m_path(std::move(path)), m_nodeId(nodeId)
{
}

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
    for (Segment const* segment : node->getSegments())
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
    for (SegmentSnapshot const& snapshot : m_segments)
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
PaintResourceCommand::PaintResourceCommand(std::string city,
                                           std::string layer,
                                           int32_t u0,
                                           int32_t v0,
                                           int32_t u1,
                                           int32_t v1,
                                           uint32_t amount)
    : m_city(std::move(city)),
      m_layer(std::move(layer)),
      m_u0(std::min(u0, u1)),
      m_v0(std::min(v0, v1)),
      m_u1(std::max(u0, u1)),
      m_v1(std::max(v0, v1)),
      m_amount(amount)
{
}

// ----------------------------------------------------------------------------
bool PaintResourceCommand::sameRectangle(int32_t u0,
                                         int32_t v0,
                                         int32_t u1,
                                         int32_t v1) const
{
    return (m_u0 == std::min(u0, u1)) && (m_v0 == std::min(v0, v1)) &&
           (m_u1 == std::max(u0, u1)) && (m_v1 == std::max(v0, v1));
}

// ----------------------------------------------------------------------------
bool PaintResourceCommand::redo(Simulation& simulation)
{
    City const* city = findCity(simulation, m_city);
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
    City const* city = findCity(simulation, m_city);
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
    std::snprintf(
        buffer, sizeof(buffer), "paint %s to %u", m_layer.c_str(), m_amount);
    return buffer;
}

// =============================================================================
// ADD AREA
// =============================================================================

namespace
{

bool regionsOverlap(CellRegion const& a, CellRegion const& b)
{
    return (a.u0 < b.getMaxU()) && (b.u0 < a.getMaxU()) &&
           (a.v0 < b.getMaxV()) && (b.v0 < a.getMaxV());
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
        pieces.push_back(
            makeRegion(from.u0, v1, from.getMaxU(), from.getMaxV()));
    if (from.u0 < cut.u0)
        pieces.push_back(
            makeRegion(from.u0, v0, std::min(from.getMaxU(), cut.u0), v1));
    if (cut.getMaxU() < from.getMaxU())
        pieces.push_back(makeRegion(
            std::max(from.u0, cut.getMaxU()), v0, from.getMaxU(), v1));

    return pieces;
}

} // namespace

AddZoneCommand::AddZoneCommand(std::string city,
                               std::string zoneType,
                               int32_t u0,
                               int32_t v0,
                               int32_t u1,
                               int32_t v1)
    : m_city(std::move(city)),
      m_zoneType(std::move(zoneType)),
      m_u0(std::min(u0, u1)),
      m_v0(std::min(v0, v1)),
      m_u1(std::max(u0, u1)),
      m_v1(std::max(v0, v1))
{
}

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
    for (SavedZone const& saved : overlapped)
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

        CellRegion const footprint =
            makeRegion(saved.u0,
                       saved.v0,
                       saved.u0 + int32_t(saved.sizeU),
                       saved.v0 + int32_t(saved.sizeV));
        for (CellRegion const& piece : subtract(footprint, region))
        {
            if (piece.isEmpty())
                continue;
            m_leftovers.push_back(city->addZone(*savedType, piece).getId());
        }
    }

    Zone const& zone = city->addZone(*type, region);
    m_zoneId = zone.getId();
    return true;
}

void AddZoneCommand::undo(Simulation& simulation)
{
    City* city = findCity(simulation, m_city);
    if (city == nullptr)
        return;

    auto const dropById = [city](size_t id)
    {
        for (auto const& zone : city->getZones())
        {
            if (zone->getId() == id)
            {
                city->removeZone(*zone);
                return;
            }
        }
    };

    dropById(m_zoneId);
    for (size_t id : m_leftovers)
        dropById(id);
    m_leftovers.clear();

    for (SavedZone const& saved : m_removed)
    {
        try
        {
            ZoneType const& type =
                simulation.getRuleset().getZoneType(saved.type);
            city->addZone(type,
                          makeRegion(saved.u0,
                                     saved.v0,
                                     saved.u0 + int32_t(saved.sizeU),
                                     saved.v0 + int32_t(saved.sizeV)));
        }
        catch (...)
        {
            // do nothing
        }
    }
}

std::string AddZoneCommand::label() const
{
    return "zone " + m_zoneType;
}
} // namespace ogb::editor
