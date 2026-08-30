//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Building.hpp
//! \brief Static entities anchored to the road network. They run rules each
//! tick.

#ifndef OPEN_GLASSBOX_BUILDING_HPP
#define OPEN_GLASSBOX_BUILDING_HPP

#include "OpenGlassBox/Entity.hpp"
#include "OpenGlassBox/OpeningHours.hpp"
#include "OpenGlassBox/Rule.hpp"

#include <memory>

namespace ogb
{

class City;
class Node;
class Path;
class RuleBuilding;
class Segment;

//==============================================================================
//! \brief A static entity such as a house, a factory, or a shop.
//!
//! It holds resources. It runs rules on a schedule. Rules send Agents to move
//! what it produced.
//!
//! A Building is not a road Node. It has a world position and a network anchor.
//! The anchor is a Node (a crossroads) or a Segment at an offset (an address
//! on a street). Segment anchors keep the graph small. Without them, forty
//! houses on one street would become forty Nodes and forty-one Segments. The
//! router would update all of them each tick.
//!
//! A Building can also stand at a free world position with no anchor. \c spawn
//! \c at \c freeCell creates this case. A save file restores it too. It can
//! hold resources and run rules on nearby Layers. No Agent can leave it or
//! reach it. Call hasSegments() to detect this. The demo inspector marks it
//! in red.
//!
//! The Building does not own its BuildingType, Node, or Segment. The City owns
//! the Building. Destroying the Building detaches it from its anchor.
//!
//! Example:
//! \code
//! // A house on a crossroads, and a shop halfway down the street.
//! Node& corner = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
//! Building& home = city.addBuilding(simulation.script().buildingType("Home"),
//! corner); Building& shop =
//! city.addBuilding(simulation.script().buildingType("Shop"),
//!                           path, street, 0.5f);
//!
//! if (!shop.hasSegments())
//!     std::cout << "nobody will ever shop there\n";
//! \endcode
//!
//! Script example. \c rules make the building act. \c targets let an Agent
//! end its trip here:
//! \code
//! building Shop color 0xFFAA00 layerRadius 2 rules [ SellGoods ReceiveShopper
//! ]
//!      targets [ Shop ] caps [ People 6 Goods 30 ] resources [ ]
//! \endcode
//==============================================================================
class Building: public Entity<BuildingType>
{
public:

    // -------------------------------------------------------------------------
    //! \brief Stand on a Path Node: a building on a crossroads.
    //! The position follows the Node when the City moves.
    //! \param[in] type building recipe. Kept by reference. Must outlive the
    //! Building.
    //! \param[in] node crossroads to stand on. The Building registers with it
    //! and unregisters on destruction.
    //! \param[in] city City that owns the Building. Provides global resources,
    //! configuration, and clock.
    // -------------------------------------------------------------------------
    Building(BuildingType const& type, Node& node, City& city);

    // -------------------------------------------------------------------------
    //! \brief Stand along a Segment without splitting it: an address on a
    //! street.
    //! \param[in] type building recipe.
    //! \param[in] segment segment to stand along.
    //! \param[in] offset position along the segment, from 0 at
    //! segment.getFrom() to 1 at segment.getTo(). Clamped to that range. The
    //! position is interpolated between the two ends.
    //! \param[in] city City that owns the Building.
    // -------------------------------------------------------------------------
    Building(BuildingType const& type,
             Segment& segment,
             float offset,
             City& city);

    // -------------------------------------------------------------------------
    //! \brief Stand at a free world position with no anchor.
    //!
    //! No Agent can leave or reach such a building. \c spawn \c at \c freeCell
    //! creates this case. A save file restores it when it stored a position
    //! instead of a Node. A Zone with \c nearestSegment will not place
    //! unreachable buildings in open land.
    //!
    //! \param[in] type building recipe.
    //! \param[in] position world position.
    //! \param[in] city City that owns the Building.
    // -------------------------------------------------------------------------
    Building(BuildingType const& type, Vector3f const& position, City& city);

    // -------------------------------------------------------------------------
    //! \brief Detach from the Node or Segment.
    // -------------------------------------------------------------------------
    ~Building();

    // -------------------------------------------------------------------------
    //! \brief Count one tick and run rules that are due.
    //!
    //! Each rule has its own period. The counter is shared. A rule runs when
    //! the count is a multiple of its period. The count starts at a phase set
    //! by spreadRuleStart().
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \brief Return whether an Agent may end its trip here.
    //!
    //! Room includes stock plus Agents already heading here. See reserve().
    //! Without that, many Agents routed on the same tick would all see one free
    //! slot and all be sent to claim it.
    //!
    //! \param[in] searchTarget what the Agent looks for. Matched against script
    //! \c targets.
    //! \param[in] resourcesToTryToAdd what the Agent carries. The building must
    //! have room for all of it.
    //! \return true when the name matches and the load fits.
    // -------------------------------------------------------------------------
    bool accepts(Name const& searchTarget,
                 Resources const& resourcesToTryToAdd);

    // -------------------------------------------------------------------------
    //! \brief Reserve a slot for an Agent routed here.
    //!
    //! Each reserve() needs exactly one release(). Call release() when the
    //! Agent delivers, gives up, changes route, or is destroyed. A count that
    //! never drops makes the building invisible to every Agent for the rest of
    //! the game. Agent::route calls both reserve() and release().
    // -------------------------------------------------------------------------
    inline void reserve()
    {
        ++m_inbound;
    }

    // -------------------------------------------------------------------------
    //! \brief Release a slot reserved by reserve().
    // -------------------------------------------------------------------------
    inline void release()
    {
        if (m_inbound != 0u)
            --m_inbound;
    }

    // -------------------------------------------------------------------------
    //! \brief Return how many Agents are on their way here.
    //!
    //! Not saved. Itineraries are recomputed on load, so counts rebuild
    //! themselves. Exposed for leak checks in tests.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getReservedCount() const
    {
        return m_inbound;
    }

    // -------------------------------------------------------------------------
    //! \brief Return what the building holds and its capacity for each
    //! resource.
    //!
    //! \note Writable. Deliveries land here. Rules consume from here.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline Resources& getResources()
    {
        return m_resources;
    }

    //! \copydoc getResources()
    [[nodiscard]] inline Resources const& getResources() const
    {
        return m_resources;
    }

    // -------------------------------------------------------------------------
    //! \brief Return the Node it stands on, or nullptr when it stands along a
    //! Segment or at a free position.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline Node* getNode() const
    {
        return m_node;
    }

    // -------------------------------------------------------------------------
    //! \brief Return the Segment it stands along, or nullptr when it stands on
    //! a Node or at a free position.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline Segment* getSegment() const
    {
        return m_segment;
    }

    // -------------------------------------------------------------------------
    //! \brief Return where along getSegment() it stands, from 0 at the origin
    //! Node to 1 at the other end. Meaningless when getSegment() is null.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline float getSegmentOffset() const
    {
        return m_offset;
    }

    // -------------------------------------------------------------------------
    //! \brief Return whether Agents can leave and reach this building.
    //!
    //! True when it stands along a Segment, or on a Node with at least one
    //! incident Segment. False when built before its road exists, or when its
    //! street was removed.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool hasSegments() const;

    // -------------------------------------------------------------------------
    //! \brief Return the Node an Agent uses to start from or reach this
    //! building.
    //!
    //! Returns the Node it stands on, or the nearer end of its Segment. Returns
    //! nullptr when the building has no anchor.
    //!
    //! An Agent on a Segment may leave from either end. See
    //! Agent::computeRouteAlongSegment.
    // -------------------------------------------------------------------------
    [[nodiscard]] Node* getAccessNode() const;

    // -------------------------------------------------------------------------
    //! \brief Return the road network it is anchored to, or nullptr when it has
    //! no anchor.
    //!
    //! Two Paths never meet. This also identifies which network an Agent uses
    //! for this building.
    // -------------------------------------------------------------------------
    [[nodiscard]] Path* getPath() const;

    // -------------------------------------------------------------------------
    //! \brief Return how far, in grid cells, its rules read and write nearby
    //! Layers. From script \c layerRadius.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getLayerRadius() const
    {
        return m_type.radius;
    }

    // -------------------------------------------------------------------------
    //! \brief Return the grid cell it stands on.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline Cell getCell() const
    {
        return m_context.cell;
    }

    // -------------------------------------------------------------------------
    //! \brief Return the rules it runs, in script order.
    //!
    //! Owned by ScriptDefinitions. Entries may be nullptr when the script named
    //! a rule that does not exist.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline std::vector<RuleBuilding*> const& getRules() const
    {
        return m_type.rules;
    }

    // -------------------------------------------------------------------------
    //! \brief Return ticks counted so far. This decides which rules are due.
    //! Starts at a phase set by spreadRuleStart().
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getTicks() const
    {
        return m_ticks;
    }

    // -------------------------------------------------------------------------
    //! \brief Return names an Agent may search for to end its trip here.
    //! Empty means nothing can be delivered.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline std::vector<Name> const& getTargets() const
    {
        return m_type.targets;
    }

    // -------------------------------------------------------------------------
    //! \brief Return when this building is active, from \c hour \c between rule
    //! conditions.
    //!
    //! Returns the building timetable. Rules with no hours mean open all day.
    //! OpeningHours::isRestricted() reports that. This stops the inspector from
    //! marking a road as open by mistake.
    // -------------------------------------------------------------------------
    [[nodiscard]] OpeningHours getOpeningHours() const;

    // -------------------------------------------------------------------------
    //! \brief Move with the City in the world.
    //! \param[in] direction how far the City moved.
    //!
    //! A building on a Node or Segment moves with its anchor instead, to avoid
    //! moving twice.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief Read the position back from the Node or Segment the building
    //! stands on. Called when the anchor moved under it. See City::moveNode().
    //!
    //! A building given a footprint of its own by a Zone stays where it was
    //! put: it was placed on a cell, not on the road. See setPosition().
    // -------------------------------------------------------------------------
    void followAnchor();

    // -------------------------------------------------------------------------
    //! \brief Release the Node or Segment anchor. The building then has no
    //! anchor. Called by the destructor and City::removeBuilding.
    // -------------------------------------------------------------------------
    void detach();

    // -------------------------------------------------------------------------
    //! \brief Set the building id.
    //!
    //! Called by City::addBuilding() and on load with the saved id.
    //! \param[in] value id, unique among buildings in the City.
    // -------------------------------------------------------------------------
    void setId(size_t value)
    {
        m_id = value;
    }

    // -------------------------------------------------------------------------
    //! \brief Offset the tick counter so this building does not run all rules
    //! on the same tick as every other building.
    //!
    //! All Units used to start at zero. At eight in the morning, the whole city
    //! left home on one tick. The phase comes from the id and
    //! Config::randomSeed. That keeps runs reproducible. The phase never
    //! exceeds one game hour, so a daily rule cannot fire the moment the
    //! building appears.
    //!
    //! Call once, after setId().
    // -------------------------------------------------------------------------
    void spreadRuleStart();

    // -------------------------------------------------------------------------
    //! \brief Recompute the grid cell it stands on.
    //!
    //! Called after the City moves or after the building moves.
    // -------------------------------------------------------------------------
    void updateCell();

    // -------------------------------------------------------------------------
    //! \brief Set the world position while keeping the Node or Segment anchor
    //! from construction.
    //!
    //! Lets a building occupy the cell its Zone chose while being served by a
    //! road nearby. moveOntoSegment() keeps this position instead of pulling
    //! the building back onto the street.
    //!
    //! \param[in] position world position.
    // -------------------------------------------------------------------------
    void setPosition(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Move the anchor to another Segment at the given offset.
    //!
    //! City::splitSegment() calls this when a split leaves buildings on a
    //! segment that no longer runs under them. A Zone-placed building keeps its
    //! footprint. See setPosition().
    //!
    //! \param[in] segment segment to stand along from now on.
    //! \param[in] offset position along it, clamped to [0..1].
    // -------------------------------------------------------------------------
    void moveOntoSegment(Segment& segment, float offset);

private:

    // -------------------------------------------------------------------------
    //! \brief Fill the context for rules: City, global resources, clock, and
    //! grid cell.
    // -------------------------------------------------------------------------
    void bind(City& city);

private:

    //! \brief Current stock. Starts as a copy of BuildingType::resources.
    Resources m_resources;
    //! \brief What rules read and write: this Building, its City, globals,
    //! clock, and grid cell.
    RuleContext m_context;
    //! \brief Crossroads it stands on, or nullptr. Not owned.
    Node* m_node = nullptr;
    //! \brief Street it stands along, or nullptr. Not owned.
    Segment* m_segment = nullptr;
    //! \brief Position along m_segment, in [0..1].
    float m_offset = 0.0f;
    //! \brief Tick count. Decides when rules run.
    uint32_t m_ticks = 0u;
    //! \brief Agents routed here that have not arrived yet. See reserve().
    uint32_t m_inbound = 0u;
    //! \brief True when m_position is the building footprint, not the anchor
    //! position. Set by setPosition().
    bool m_placed = false;
};

//! \brief Buildings in a City. The City owns them.
using Buildings = std::vector<std::unique_ptr<Building>>;

} // namespace ogb

#endif
