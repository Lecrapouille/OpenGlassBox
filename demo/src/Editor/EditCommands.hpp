//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file EditCommands.hpp
//! \brief Undoable commands that mutate the map through the editor.


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
//! \brief Lay a road: create the nodes that do not exist yet and the segment
//! joining them.
//!
//! The end points are given as positions, and snapping to an existing node is
//! resolved when the command runs, so that a redo reuses the same node the
//! original edit did.
// ============================================================================
class AddWayCommand: public ICommand
{
public:

    AddWayCommand(std::string city, std::string path, std::string wayType,
                  Vector3f from, Vector3f to, float snapRadius);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override;

private:

    std::string m_city;
    std::string m_path;
    std::string m_wayType;
    Vector3f m_from;
    Vector3f m_to;
    float m_snapRadius;

    //! \brief Identifiers handed out by the first run, replayed by the redos so
    //! that the commands stacked above keep pointing at the right things.
    uint32_t m_wayId = NO_ID;
    uint32_t m_fromId = NO_ID;
    uint32_t m_toId = NO_ID;
    //! \brief Whether the end points were created by this command, and so have
    //! to be taken back by the undo.
    bool m_createdFrom = false;
    bool m_createdTo = false;
};

// ============================================================================
//! \brief Place a building on a road.
//!
//! Dropping one in the middle of a segment cuts it in two and puts the
//! building on the junction, which is what makes it an address agents stop at
//! rather than a shape drawn beside the road. The undo has to put the two
//! halves back together.
// ============================================================================
class AddUnitCommand: public ICommand
{
public:

    AddUnitCommand(std::string city, std::string path, std::string unitType,
                   uint32_t wayId, float offset);

    //! \brief Place the building on an existing node.
    AddUnitCommand(std::string city, std::string path, std::string unitType,
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
    std::string m_unitType;
    uint32_t m_wayId = NO_ID;
    float m_offset = 0.5f;
    uint32_t m_nodeId = NO_ID;
    uint32_t m_unitId = NO_ID;

    //! \brief What the cut created, so that the undo can sew the segment back.
    //! The junction carries the building; the second half runs from it to the
    //! far end the segment used to reach.
    uint32_t m_junctionId = NO_ID;
    uint32_t m_secondHalfId = NO_ID;
    std::string m_wayType;
};

// ============================================================================
//! \brief Demolish a building.
// ============================================================================
class RemoveUnitCommand: public ICommand
{
public:

    RemoveUnitCommand(std::string city, std::string path, uint32_t id,
                      std::string unitType, bool byUnitId = false);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;

private:

    std::string m_city;
    std::string m_path;
    uint32_t m_id;
    std::string m_unitType;
    bool m_byUnitId = false;
    uint32_t m_wayId = NO_ID;
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
class RemoveWayCommand: public ICommand
{
public:

    RemoveWayCommand(std::string city, std::string path, uint32_t wayId);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override { m_captured = false; }

private:

    std::string m_city;
    std::string m_path;
    uint32_t m_wayId;
    std::string m_wayType;
    uint32_t m_fromId = 0u;
    uint32_t m_toId = 0u;
    Vector3f m_fromPosition;
    Vector3f m_toPosition;
    bool m_captured = false;
};

// ============================================================================
//! \brief Demolish a node and, if any, its incident segments.
//!
//! Undoing restores the node and the segments. Units and Agents that sat on
//! them are gone, the same way they are after a road demolition.
// ============================================================================
class RemoveNodeCommand: public ICommand
{
public:

    RemoveNodeCommand(std::string city, std::string path, uint32_t nodeId);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override { m_captured = false; m_ways.clear(); }

private:

    struct WaySnapshot
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
    std::vector<WaySnapshot> m_ways;
    bool m_captured = false;
};

// ============================================================================
//! \brief Paint an amount of resource over a rectangle of Map cells.
//!
//! The previous content of the rectangle is copied before being overwritten,
//! which is what makes the brush undoable.
// ============================================================================
class PaintResourceCommand: public ICommand
{
public:

    PaintResourceCommand(std::string city, std::string map, int32_t u0,
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
    std::string m_map;
    int32_t m_u0;
    int32_t m_v0;
    int32_t m_u1;
    int32_t m_v1;
    uint32_t m_amount;
    //! \brief What the cells held before the brush went over them, row major.
    std::vector<uint32_t> m_previous;
};

// ============================================================================
//! \brief Paint an Area over a rectangle of cells.
//!
//! Areas do not overlap: painting Commercial over a corner of a Residential
//! rectangle re-zones exactly the cells painted. The part of the old Area that
//! survives is put back as up to four rectangles, which is what lets a zone be
//! converted piece by piece instead of being wiped out whole.
// ============================================================================
class AddAreaCommand: public ICommand
{
public:

    AddAreaCommand(std::string city, std::string areaType, int32_t u0,
                   int32_t v0, int32_t u1, int32_t v1);

    bool redo(Simulation& simulation) override;
    void undo(Simulation& simulation) override;
    std::string label() const override;
    void onWorldRebuilt() override
    {
        m_areaId = NO_ID;
        m_removed.clear();
        m_leftovers.clear();
    }

private:

    struct SavedArea
    {
        std::string type;
        int32_t u0 = 0;
        int32_t v0 = 0;
        uint32_t sizeU = 0u;
        uint32_t sizeV = 0u;
    };

    std::string m_city;
    std::string m_areaType;
    int32_t m_u0;
    int32_t m_v0;
    int32_t m_u1;
    int32_t m_v1;
    uint32_t m_areaId = NO_ID;
    //! \brief The Areas this command re-zoned, as they were before it ran.
    std::vector<SavedArea> m_removed;
    //! \brief Identifiers of the rectangles added back for the parts of those
    //! Areas the new one does not cover.
    std::vector<uint32_t> m_leftovers;
};
} // namespace editor
} // namespace ogb

#endif
