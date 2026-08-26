//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Unit.hpp
//! \brief Buildings anchored to the road network, running their rules each
//! tick.

#ifndef OPEN_GLASSBOX_UNIT_HPP
#define OPEN_GLASSBOX_UNIT_HPP

#include "OpenGlassBox/Entity.hpp"
#include "OpenGlassBox/OpeningHours.hpp"
#include "OpenGlassBox/Rule.hpp"

#include <memory>

namespace ogb
{

class City;
class Node;
class Path;
class RuleUnit;
class Way;

//==============================================================================
//! \brief A building: a house, a factory, a shop. It holds resources, it runs
//! its rules once every so many ticks, and those rules send Agents out to carry
//! what it produced somewhere else.
//!
//! A Unit is not a Node of the road network. It has its own world position and,
//! separately, an anchor on the network: either a Node, which is a crossroads,
//! or a Way at a given offset, which is an address along a street. Anchoring
//! along a Way is what keeps the graph small: a street of forty houses used to
//! become forty Nodes and forty-one segments, and the router paid for all of
//! them at every tick.
//!
//! A third case exists and is worth knowing about: a Unit may stand at a free
//! world position with no anchor at all. It is what \c spawn \c at \c freeCell
//! produces, and what a save restores for such a building. It can hold
//! resources and run rules that only touch the Maps under it, but no Agent can
//! ever leave it or reach it. hasWays() is how to tell, and the inspector of
//! the demo says so in red.
//!
//! A Unit does not own its UnitType, its Node or its Way, and the City owns the
//! Unit. Destroying it detaches it from whatever it was anchored to.
//!
//! Example:
//! \code
//! // A house on a crossroads, and a shop halfway down the street.
//! Node& corner = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
//! Unit& home = city.addUnit(simulation.script().getUnitType("Home"), corner);
//! Unit& shop = city.addUnit(simulation.script().getUnitType("Shop"),
//!                           path, street, 0.5f);
//!
//! if (!shop.hasWays())
//!     std::cout << "nobody will ever shop there\n";
//! \endcode
//!
//! The matching script, where \c rules is what makes the building do anything
//! and \c targets is what lets an Agent end its trip there:
//! \code
//! unit Shop color 0xFFAA00 mapRadius 2 rules [ SellGoods ReceiveShopper ]
//!      targets [ Shop ] caps [ People 6 Goods 30 ] resources [ ]
//! \endcode
//==============================================================================
class Unit: public Entity<UnitType>
{
public:

    // -------------------------------------------------------------------------
    //! \brief Stand on an existing Node of a Path: a building on a crossroads.
    //! Its position follows the Node when the City is moved.
    //! \param[in] type recipe of the building. Kept by reference and has to
    //! outlive the Unit.
    //! \param[in] node the crossroads to stand on. The Unit registers itself
    //! with it and takes itself out on destruction.
    //! \param[in] city the City that will own it, read for its global
    //! resources, its configuration and its clock.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Node& node, City& city);

    // -------------------------------------------------------------------------
    //! \brief Stand along a Way, without splitting it: an address on a street.
    //! \param[in] type recipe of the building.
    //! \param[in] way the segment to stand along.
    //! \param[in] offset where along it, from 0 at way.from() to 1 at
    //! way.to(). Clamped to that range. The position is interpolated between
    //! the two ends.
    //! \param[in] city the City that will own it.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Way& way, float offset, City& city);

    // -------------------------------------------------------------------------
    //! \brief Stand at a free world position, anchored to nothing.
    //!
    //! No Agent can leave such a building and none can reach it, which is
    //! deliberate: this is what \c spawn \c at \c freeCell asks for, and what a
    //! save file restores when it wrote a position rather than a node. An Area
    //! asked for \c nearestWay refuses to grow one at all rather than scatter
    //! unreachable houses in a field.
    //!
    //! \param[in] type recipe of the building.
    //! \param[in] position where to stand, in world coordinates.
    //! \param[in] city the City that will own it.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Vector3f const& position, City& city);

    // -------------------------------------------------------------------------
    //! \brief Detach from the Node or the Way it stood on.
    // -------------------------------------------------------------------------
    ~Unit();

    // -------------------------------------------------------------------------
    //! \brief Count one tick and attempt the rules that fall due on it.
    //!
    //! Each rule has its own period, and the counter is shared: a rule fires
    //! when the count is a multiple of its period. The count starts at a phase
    //! of its own, see desynchronise().
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \brief Whether an Agent may end its trip here.
    //!
    //! The room counted is the stock plus the Agents already heading here, see
    //! reserve(). Without that, twenty Agents routed on the same tick would all
    //! see the same single free slot and all be sent to claim it.
    //!
    //! \param[in] searchTarget what the Agent is looking for, matched against
    //! the \c targets of the script.
    //! \param[in] resourcesToTryToAdd what it carries. The building has to have
    //! room for all of it.
    //! \return true when the name matches and the load fits.
    // -------------------------------------------------------------------------
    bool accepts(Name const& searchTarget,
                 Resources const& resourcesToTryToAdd);

    // -------------------------------------------------------------------------
    //! \brief Claim a place for an Agent that has just been routed here.
    //!
    //! Every reserve() has to be matched by exactly one release(), whether the
    //! Agent delivers, gives up, changes its mind or is destroyed. A count that
    //! never comes back down makes the building invisible to every Agent for
    //! the rest of the game, which is worse than the crowding it prevents.
    //! Agent::setRoute is the single place both are called from.
    // -------------------------------------------------------------------------
    inline void reserve()
    {
        ++m_inbound;
    }

    // -------------------------------------------------------------------------
    //! \brief Give back a place claimed by reserve().
    // -------------------------------------------------------------------------
    inline void release()
    {
        if (m_inbound != 0u)
            --m_inbound;
    }

    // -------------------------------------------------------------------------
    //! \brief How many Agents are currently on their way here.
    //!
    //! Not saved: itineraries are recomputed on loading, so the counts rebuild
    //! themselves. Exposed for the leak invariant the tests check.
    // -------------------------------------------------------------------------
    inline uint32_t inbound() const
    {
        return m_inbound;
    }

    // -------------------------------------------------------------------------
    //! \brief What the building currently holds, and how much of each resource
    //! it can hold. Writable: this is what a delivery lands in and what a rule
    //! consumes.
    // -------------------------------------------------------------------------
    inline Resources& resources()
    {
        return m_resources;
    }

    // -------------------------------------------------------------------------
    //! \brief \copydoc resources()
    // -------------------------------------------------------------------------
    inline Resources const& resources() const
    {
        return m_resources;
    }

    // -------------------------------------------------------------------------
    //! \brief The Node it stands on, or nullptr when it stands along a Way or
    //! at a free position.
    // -------------------------------------------------------------------------
    inline Node* node() const
    {
        return m_node;
    }

    // -------------------------------------------------------------------------
    //! \brief The Way it stands along, or nullptr when it stands on a Node or
    //! at a free position.
    // -------------------------------------------------------------------------
    inline Way* way() const
    {
        return m_way;
    }

    // -------------------------------------------------------------------------
    //! \brief Where along way() it stands, from 0 at the origin Node to 1 at
    //! the other end. Meaningless when way() is null.
    // -------------------------------------------------------------------------
    inline float wayOffset() const
    {
        return m_offset;
    }

    // -------------------------------------------------------------------------
    //! \brief Whether Agents can leave this building and reach it: it has to
    //! stand along a Way, or on a Node that at least one Way is incident to.
    //!
    //! A house put up before the road that was meant to serve it answers false,
    //! and so does one left behind by a demolished street.
    // -------------------------------------------------------------------------
    bool hasWays() const;

    // -------------------------------------------------------------------------
    //! \brief The Node an Agent starts from or aims at when dealing with this
    //! building: the Node it stands on, or the nearer end of the Way it stands
    //! along.
    //! \return nullptr when the building is anchored to nothing.
    //!
    //! An Agent leaving a building along a street does not always leave by that
    //! end: see Agent::computeRouteAlongWay, which weighs both.
    // -------------------------------------------------------------------------
    Node* accessNode() const;

    // -------------------------------------------------------------------------
    //! \brief The road network it is anchored to, or nullptr when it is
    //! anchored to nothing. Two Paths never meet, so this is also which network
    //! an Agent dealing with it will drive on.
    // -------------------------------------------------------------------------
    Path* path() const;

    // -------------------------------------------------------------------------
    //! \brief How far, in grid cells, its rules read and write the Maps around
    //! it. From the script: \c mapRadius.
    // -------------------------------------------------------------------------
    inline uint32_t mapRadius() const
    {
        return m_type.radius;
    }

    // -------------------------------------------------------------------------
    //! \brief Column of the grid cell it stands on. Signed: the grid grows in
    //! the four directions.
    // -------------------------------------------------------------------------
    inline int32_t mapU() const
    {
        return m_context.u;
    }

    // -------------------------------------------------------------------------
    //! \brief Row of the grid cell it stands on. See mapU().
    // -------------------------------------------------------------------------
    inline int32_t mapV() const
    {
        return m_context.v;
    }

    // -------------------------------------------------------------------------
    //! \brief The rules it attempts, in the order the script listed them. Owned
    //! by ScriptDefinitions and possibly holding nullptr, when the script named
    //! a rule that does not exist.
    // -------------------------------------------------------------------------
    inline std::vector<RuleUnit*> const& rules() const
    {
        return m_type.rules;
    }

    // -------------------------------------------------------------------------
    //! \brief Ticks counted so far, which is what decides which rules fall due.
    //! Starts at a phase of its own, see desynchronise().
    // -------------------------------------------------------------------------
    inline uint32_t ticks() const
    {
        return m_ticks;
    }

    // -------------------------------------------------------------------------
    //! \brief The names an Agent may look for to end its trip here. Empty means
    //! nothing can ever be delivered to it.
    // -------------------------------------------------------------------------
    inline std::vector<Name> const& targets() const
    {
        return m_type.targets;
    }

    // -------------------------------------------------------------------------
    //! \brief When this building has something to do, read from the \c hour
    //! \c between conditions of its rules.
    //!
    //! \return the timetable of the building. A building whose rules keep no
    //! office hours is open around the clock, and OpeningHours::bounded() says
    //! so, which is what keeps the inspector from claiming that a road is open.
    // -------------------------------------------------------------------------
    OpeningHours openingHours() const;

    // -------------------------------------------------------------------------
    //! \brief Follow the City as it is moved in the world.
    //! \param[in] direction how far the City moved.
    //!
    //! A building anchored to a Node or a Way is moved by it instead, so as not
    //! to be moved twice.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief Let go of the Node or the Way it stands on, becoming a building
    //! anchored to nothing. Called by the destructor and by City::removeUnit.
    // -------------------------------------------------------------------------
    void detach();

    // -------------------------------------------------------------------------
    //! \brief Number the building. Called by City::addUnit, and by a load with
    //! the number the save file wrote, so that what refers to a building by
    //! number still finds it.
    //! \param[in] id identifier, unique among the buildings of the City.
    // -------------------------------------------------------------------------
    void setId(uint32_t id)
    {
        m_id = id;
    }

    // -------------------------------------------------------------------------
    //! \brief Offset the tick counter so that this building does not run its
    //! rules on the same tick as every other one.
    //!
    //! Every Unit used to start counting at zero, so at eight in the morning
    //! the whole city left home on the same tick. The phase is derived from the
    //! identifier and from SimulationConfig::randomSeed, which keeps a run
    //! reproducible, and it never exceeds one game hour so that a rule counted
    //! in days cannot fire the moment the building goes up.
    //!
    //! Call it once, after setId().
    // -------------------------------------------------------------------------
    void desynchronise();

    // -------------------------------------------------------------------------
    //! \brief Recompute which grid cell it stands on. Called after the City has
    //! been moved, or after the building itself has.
    // -------------------------------------------------------------------------
    void refreshMapPosition();

    // -------------------------------------------------------------------------
    //! \brief Stand at that world position while staying anchored to the Node
    //! or the Way given at construction.
    //!
    //! This is how a building occupies the cell its Area chose for it while
    //! being served by a road running a few cells away. Such a position is kept
    //! as is by reanchor(), which would otherwise pull the building back onto
    //! the street.
    //!
    //! \param[in] position where to stand, in world coordinates.
    // -------------------------------------------------------------------------
    void placeAt(Vector3f const& position);

    // -------------------------------------------------------------------------
    //! \brief Move the anchor onto another Way, at the given offset.
    //!
    //! Cutting a segment in two leaves the buildings along it anchored to a
    //! segment that no longer runs under them, and this is how City::splitWay
    //! puts them back where they stand. A building placed by an Area keeps its
    //! footprint, see placeAt().
    //!
    //! \param[in] way the segment to stand along from now on.
    //! \param[in] offset where along it, clamped to [0..1].
    // -------------------------------------------------------------------------
    void reanchor(Way& way, float offset);

private:

    // -------------------------------------------------------------------------
    //! \brief Fill in the context its rules run with: the City, its global
    //! resources, the clock, and the grid cell the building stands on.
    // -------------------------------------------------------------------------
    void bind(City& city);

private:

    //! \brief The crossroads it stands on, or nullptr. Not owned.
    Node* m_node = nullptr;
    //! \brief The street it stands along, or nullptr. Not owned.
    Way* m_way = nullptr;
    //! \brief Where along m_way it stands, in [0..1].
    float m_offset = 0.0f;
    //! \brief Whether m_position is the footprint of the building rather than
    //! the position of its anchor. Set by placeAt().
    bool m_placed = false;
    //! \brief What it holds now. Starts as a copy of UnitType::resources.
    Resources m_resources;
    //! \brief What its rules read and write: itself, its City, the globals, the
    //! clock and the grid cell it acts on.
    RuleContext m_context;
    //! \brief Ticks counted, which is what makes the rules fall due.
    uint32_t m_ticks = 0u;
    //! \brief How many Agents have been routed here and have not arrived yet.
    //! See reserve().
    uint32_t m_inbound = 0u;
};

//! \brief The buildings of a City, which owns them.
using Units = std::vector<std::unique_ptr<Unit>>;

} // namespace ogb

#endif
