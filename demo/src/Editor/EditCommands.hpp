//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file EditCommands.hpp
//! \brief Undoable commands that mutate the city through the editor.


#ifndef OPEN_GLASSBOX_DEMO_EDIT_COMMANDS_HPP
#  define OPEN_GLASSBOX_DEMO_EDIT_COMMANDS_HPP

#  include "OpenGlassBox/Path.hpp"
#  include "OpenGlassBox/Simulation.hpp"
#  include "OpenGlassBox/Vector.hpp"

#  include <cstdint>
#  include <deque>
#  include <memory>
#  include <string>
#  include <vector>


namespace ogb {
namespace editor {


//! \brief Stands for "no identifier yet". Identifiers are handed out by the
//! engine on the first run of a command and replayed by the redos, so they need
//! a value that cannot be mistaken for a real one before that first run.
enum : uint32_t { NO_ID = 0xFFFFFFFFu };

// ============================================================================
//! \brief Where a piece of graph lives, by name and identifier rather than by
//! address.
//!
//! An edit command cannot hold a \c Node* : undoing a demolition recreates the
//! node at a fresh address, and every command stacked above would then point
//! into freed memory. Naming the node instead makes the reference survive the
//! round trip, which is why Path::addNode takes an explicit identifier.
// ============================================================================
struct NodeRef
{
    std::string city;
    std::string path;
    uint32_t id = 0u;

    //! \brief Resolve to the live node, or nullptr when it no longer exists.
    Node* resolve(Simulation& simulation) const;
};

// ============================================================================
//! \brief An action the player can take back.
//!
//! Every edit goes through one of these, so that undo is a property of the
//! editor rather than something each tool has to remember to implement.
// ============================================================================
class ICommand
{
public:

    virtual ~ICommand() = default;

    //! \brief Apply the action. Called once when the command is pushed, and
    //! again on every redo.
    //! \return false when the action could not be applied, in which case the
    //! command is dropped instead of being stacked.
    virtual bool redo(Simulation& simulation) = 0;

    //! \brief Bring the simulation back to the state it had before redo().
    virtual void undo(Simulation& simulation) = 0;

    //! \brief One line naming the action, shown in the history.
    virtual std::string label() const = 0;

    //! \brief Told that the world was rebuilt from scratch, so the identifiers
    //! the engine handed out for this command's own creations no longer mean
    //! anything and have to be asked for again.
    //!
    //! Identifiers that address something the command did not create are kept:
    //! rebuilding the demo world is deterministic, so a segment keeps the
    //! identifier it had.
    virtual void onWorldRebuilt() {}
};

using CommandPtr = std::unique_ptr<ICommand>;

// ============================================================================
//! \brief The undo and redo stacks.
// ============================================================================
class CommandStack
{
public:

    //! \brief Apply a command and, if it succeeded, make it the new top of the
    //! undo stack. Pushing invalidates the redo stack, as usual.
    //! \return whether the command was applied.
    bool push(Simulation& simulation, CommandPtr command);

    void undo(Simulation& simulation);
    void redo(Simulation& simulation);

    bool canUndo() const { return !m_done.empty(); }
    bool canRedo() const { return !m_undone.empty(); }

    std::string undoLabel() const;
    std::string redoLabel() const;

    //! \brief Forget the whole history. Called when the world is rebuilt, as
    //! the recorded actions no longer mean anything.
    void clear();

    //! \brief Hand the undo stack over to the caller and empty both stacks.
    //!
    //! A hot reload rebuilds the world from a fresh script, which invalidates
    //! everything the commands point at, but the commands themselves describe
    //! what the player did in terms the new world understands. Taking them out
    //! lets the caller apply them again on top of it.
    void takeHistory(std::deque<CommandPtr>& out);

    //! \brief The undo stack, oldest first, for the history panel.
    std::deque<CommandPtr> const& history() const { return m_done; }

    //! \brief How many commands are stacked above the ones already undone.
    size_t pendingRedos() const { return m_undone.size(); }

private:

    //! \brief Beyond this many commands the oldest are forgotten. A city
    //! editing session is long and each command holds a copy of what it
    //! overwrote, which for a resource brush is a whole rectangle.
    static constexpr size_t CAPACITY = 200u;

    std::deque<CommandPtr> m_done;
    std::deque<CommandPtr> m_undone;
};

// ============================================================================
//! \brief One half of a segment that was cut in two to make a junction.
//!
//! Every edit that cuts a segment records this, and undoing it sews the halves
//! back. See Path::findCrossings() for why a road being drawn cuts the ones it
//! runs over.
// ============================================================================
struct SegmentCut
{
    //! \brief The crossroads the cut created.
    uint32_t junctionId = NO_ID;
    //! \brief The segment that was cut. It kept its identifier and is now the
    //! half on the side it ran from.
    uint32_t firstId = NO_ID;
    //! \brief The half the cut created, running from the junction to the far
    //! end the segment used to reach.
    uint32_t secondId = NO_ID;
    //! \brief Type of the segment, needed to lay it again as one.
    std::string type;
};

// ============================================================================
//! \brief Lay a road: create the nodes that do not exist yet, cut the roads it
//! runs over, and lay the pieces joining all of them.
//!
//! The end points are given as positions, and snapping to an existing node is
//! resolved when the command runs, so that a redo reuses the same node the
//! original edit did.
//!
//! A road drawn across another comes out as several segments meeting at
//! junctions, which is what lets an agent turn there. Undoing takes the pieces
//! back and sews the roads that were cut.
// ============================================================================
class AddSegmentCommand: public ICommand
{
public:

    AddSegmentCommand(std::string city, std::string path, std::string segmentType,
                  Vector3f from, Vector3f to, float snapRadius);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override;

private:

    std::string m_city;
    std::string m_path;
    std::string m_segmentType;
    Vector3f m_from;
    Vector3f m_to;
    float m_snapRadius;

    //! \brief Identifiers handed out by the first run, replayed by the redos so
    //! that the commands stacked above keep pointing at the right things.
    //! One per piece the road came out in, from the first end to the other.
    std::vector<uint32_t> m_pieceIds;
    uint32_t m_fromId = NO_ID;
    uint32_t m_toId = NO_ID;
    //! \brief The roads this one cut on its way, in the order it met them.
    std::vector<SegmentCut> m_cuts;
    //! \brief Whether the end points were created by this command, and so have
    //! to be taken back by the undo.
    bool m_createdFrom = false;
    bool m_createdTo = false;
};

// ============================================================================
//! \brief Cut a road in two at a crossroads, without building anything on it.
//!
//! This is the crossing a player asks for by hand: on a network whose lines do
//! not meet where they cross, and wherever a junction is wanted that no road
//! being drawn would have made. See Path::findCrossings().
// ============================================================================
class SplitSegmentCommand: public ICommand
{
public:

    SplitSegmentCommand(std::string city, std::string path, uint32_t segmentId,
                        float offset);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override { m_cut = SegmentCut(); }

private:

    std::string m_city;
    std::string m_path;
    uint32_t m_segmentId;
    float m_offset;
    SegmentCut m_cut;
};

// ============================================================================
//! \brief Move a crossroads, and with it the roads and buildings hanging off
//! it.
//!
//! The node is named by identifier and the two positions are recorded, so
//! undoing is the same move the other way and survives the road being cut or
//! built on in the meantime.
// ============================================================================
class MoveNodeCommand: public ICommand
{
public:

    MoveNodeCommand(std::string city, std::string path, uint32_t nodeId,
                    Vector3f from, Vector3f to);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;

private:

    //! \brief Put the node at that position, or do nothing when it is gone.
    void place(Simulation& simulation, Vector3f const& position);

private:

    std::string m_city;
    std::string m_path;
    uint32_t m_nodeId;
    Vector3f m_from;
    Vector3f m_to;
};

// ============================================================================
//! \brief Place a building on a road.
//!
//! Dropping one in the middle of a segment cuts it in two and puts the
//! building on the junction, which is what makes it an address agents stop at
//! rather than a shape drawn beside the road. The undo has to put the two
//! halves back together.
// ============================================================================
class AddBuildingCommand: public ICommand
{
public:

    AddBuildingCommand(std::string city, std::string path, std::string buildingType,
                   uint32_t segmentId, float offset);

    //! \brief Place the building on an existing node.
    AddBuildingCommand(std::string city, std::string path, std::string buildingType,
                   uint32_t nodeId);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override;

private:

    //! \brief Put the two halves of the cut segment back into one, as long as
    //! nothing was built on them in the meantime.
    void mergeBack(Simulation& simulation);

private:

    std::string m_city;
    std::string m_path;
    std::string m_buildingType;
    uint32_t m_segmentId = NO_ID;
    float m_offset = 0.5f;
    uint32_t m_nodeId = NO_ID;
    uint32_t m_buildingId = NO_ID;

    //! \brief What the cut created, so that the undo can sew the segment back.
    //! Its junction is what carries the building.
    SegmentCut m_cut;
};

// ============================================================================
//! \brief Demolish a building.
// ============================================================================
class RemoveBuildingCommand: public ICommand
{
public:

    RemoveBuildingCommand(std::string city, std::string path, uint32_t id,
                      std::string buildingType, bool byBuildingId = false);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;

private:

    std::string m_city;
    std::string m_path;
    uint32_t m_id;
    std::string m_buildingType;
    bool m_byBuildingId = false;
    uint32_t m_segmentId = NO_ID;
    float m_offset = 0.5f;
    Vector3f m_position;
    bool m_onNode = true;
};

// ============================================================================
//! \brief Demolish a stretch of road.
//!
//! Undoing gives back the segment, but not the Agents that were travelling on
//! it: they are gone for good, the same way they are when a rule stops
//! producing them. The road network is the state worth restoring.
// ============================================================================
class RemoveSegmentCommand: public ICommand
{
public:

    RemoveSegmentCommand(std::string city, std::string path, uint32_t segmentId);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override { m_captured = false; }

private:

    std::string m_city;
    std::string m_path;
    uint32_t m_segmentId;
    std::string m_segmentType;
    uint32_t m_fromId = 0u;
    uint32_t m_toId = 0u;
    Vector3f m_fromPosition;
    Vector3f m_toPosition;
    bool m_captured = false;
};

// ============================================================================
//! \brief Demolish a node and, if any, its incident segments.
//!
//! Undoing restores the node and the segments. Buildings and Agents that sat on
//! them are gone, the same way they are after a road demolition.
// ============================================================================
class RemoveNodeCommand: public ICommand
{
public:

    RemoveNodeCommand(std::string city, std::string path, uint32_t nodeId);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override { m_captured = false; m_segments.clear(); }

private:

    struct SegmentSnapshot
    {
        uint32_t id = 0u;
        std::string type;
        uint32_t fromId = 0u;
        uint32_t toId = 0u;
        Vector3f fromPosition;
        Vector3f toPosition;
    };

    std::string m_city;
    std::string m_path;
    uint32_t m_nodeId;
    Vector3f m_position;
    std::vector<SegmentSnapshot> m_segments;
    bool m_captured = false;
};

// ============================================================================
//! \brief Paint an amount of resource over a rectangle of Layer cells.
//!
//! The previous content of the rectangle is copied before being overwritten,
//! which is what makes the brush undoable.
// ============================================================================
class PaintResourceCommand: public ICommand
{
public:

    PaintResourceCommand(std::string city, std::string layer, int32_t u0,
                         int32_t v0, int32_t u1, int32_t v1, uint32_t amount);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    //! \brief The cells of the rebuilt world hold whatever the new script put
    //! there, so what this command overwrote has to be sampled again.
    void onWorldRebuilt() override { m_previous.clear(); }

    //! \brief Whether the given rectangle is the one this command paints, used
    //! to keep a dragged stroke from stacking one command per frame.
    bool sameRectangle(int32_t u0, int32_t v0, int32_t u1, int32_t v1) const;

private:

    std::string m_city;
    std::string m_layer;
    int32_t m_u0;
    int32_t m_v0;
    int32_t m_u1;
    int32_t m_v1;
    uint32_t m_amount;
    //! \brief What the cells held before the brush went over them, row major.
    std::vector<uint32_t> m_previous;
};

// ============================================================================
//! \brief Paint a Zone over a rectangle of cells.
//!
//! Zones do not overlap: painting Commercial over a corner of a Residential
//! rectangle re-zones exactly the cells painted. The part of the old Zone that
//! survives is put back as up to four rectangles, which is what lets a zone be
//! converted piece by piece instead of being wiped out whole.
// ============================================================================
class AddZoneCommand: public ICommand
{
public:

    AddZoneCommand(std::string city, std::string zoneType, int32_t u0,
                   int32_t v0, int32_t u1, int32_t v1);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override
    {
        m_zoneId = NO_ID;
        m_removed.clear();
        m_leftovers.clear();
    }

private:

    struct SavedZone
    {
        std::string type;
        int32_t u0 = 0;
        int32_t v0 = 0;
        uint32_t sizeU = 0u;
        uint32_t sizeV = 0u;
    };

    std::string m_city;
    std::string m_zoneType;
    int32_t m_u0;
    int32_t m_v0;
    int32_t m_u1;
    int32_t m_v1;
    uint32_t m_zoneId = NO_ID;
    //! \brief The Zones this command re-zoned, as they were before it ran.
    std::vector<SavedZone> m_removed;
    //! \brief Identifiers of the rectangles added back for the parts of those
    //! Zones the new one does not cover.
    std::vector<uint32_t> m_leftovers;
};
} // namespace editor
} // namespace ogb

#endif
