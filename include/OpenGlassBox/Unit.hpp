//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Unit.hpp
//! \brief Buildings attached to nodes or roads, running unit rules each tick.


#ifndef OPEN_GLASSBOX_UNIT_HPP
#  define OPEN_GLASSBOX_UNIT_HPP

#  include "OpenGlassBox/Path.hpp"
#  include "OpenGlassBox/Rule.hpp"

namespace ogb {

class RuleUnit;
class City;

//==============================================================================
//! \brief A Unit represents things: houses, factories, even people. A unit has
//! state: a collection of resource but also a well-defined spatial extent.
//!
//! A Unit is no longer forced to sit on a Path Node. It has its own world
//! position, and optionally an anchor on the road network: a Node (an
//! intersection) or a Way at a given offset (a building along a street). The
//! latter is what keeps the graph small: a street of forty houses used to
//! become forty nodes and forty-one segments.
//==============================================================================
class Unit
{
public:

    // -------------------------------------------------------------------------
    //! \brief Sit on an existing Path Node. The position of the Unit follows
    //! the Node.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Node& node, City& city);

    // -------------------------------------------------------------------------
    //! \brief Sit on a Way at the given offset, without splitting it. The
    //! position is interpolated between the two extremities.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Way& way, float offset, City& city);

    // -------------------------------------------------------------------------
    //! \brief Sit at a free world position, with no attachment to the road
    //! network. Agents cannot reach it and it cannot produce any.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Vector3f const& position, City& city);

    VIRTUAL ~Unit();

    VIRTUAL void executeRules();

    bool accepts(std::string const& searchTarget, Resources const& resourcesToTryToAdd);

    inline std::string const& type() const { return m_type.name; }
    inline Resources& resources() { return m_resources; }
    inline Resources const& resources() const { return m_resources; }
    inline Vector3f const& position() const { return m_position; }
    inline uint32_t color() const { return m_type.color; }

    // -------------------------------------------------------------------------
    //! \brief The Node this Unit sits on, or nullptr when it sits on a Way or
    //! at a free position.
    // -------------------------------------------------------------------------
    inline Node* node() const { return m_node; }

    // -------------------------------------------------------------------------
    //! \brief The Way this Unit sits on, or nullptr when it sits on a Node or
    //! at a free position.
    // -------------------------------------------------------------------------
    inline Way* way() const { return m_way; }

    // -------------------------------------------------------------------------
    //! \brief Offset along way(), in [0..1] from the origin node.
    // -------------------------------------------------------------------------
    inline float wayOffset() const { return m_offset; }

    // -------------------------------------------------------------------------
    //! \brief Unique identifier, independent of any Node.
    // -------------------------------------------------------------------------
    inline uint32_t id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief Whether Agents can leave this Unit and reach it: it has to be
    //! attached to a Node with at least one Way, or to a Way itself.
    // -------------------------------------------------------------------------
    bool hasWays() const;

    // -------------------------------------------------------------------------
    //! \brief A Node Agents can start from or aim at: the Node the Unit sits
    //! on, or the closer extremity of the Way it sits on. Nullptr when the
    //! Unit is not attached to the network.
    // -------------------------------------------------------------------------
    Node* accessNode() const;

    // -------------------------------------------------------------------------
    //! \brief The Path this Unit is attached to, or nullptr.
    // -------------------------------------------------------------------------
    Path* path() const;

    inline uint32_t mapRadius() const { return m_type.radius; }
    inline int32_t mapU() const { return m_context.u; }
    inline int32_t mapV() const { return m_context.v; }
    inline std::vector<RuleUnit*> const& rules() const { return m_type.rules; }
    inline uint32_t ticks() const { return m_ticks; }
    inline std::vector<std::string> const& targets() const { return m_type.targets; }

    // -------------------------------------------------------------------------
    //! \brief Translate the Unit when the City itself is translated. A Unit
    //! sitting on a Node or a Way follows it instead.
    // -------------------------------------------------------------------------
    void translate(Vector3f const& direction);

    // -------------------------------------------------------------------------
    //! \brief Detach from the Node or Way this Unit sits on. Called by the
    //! destructor and by City::removeUnit.
    // -------------------------------------------------------------------------
    void detach();

    // -------------------------------------------------------------------------
    //! \brief Identifier assigned by the City, unique inside it.
    // -------------------------------------------------------------------------
    void setId(uint32_t id) { m_id = id; }

    // -------------------------------------------------------------------------
    //! \brief Refresh the cell this Unit acts on after the City has moved.
    // -------------------------------------------------------------------------
    void refreshMapPosition();

    // -------------------------------------------------------------------------
    //! \brief Stand at that world position while staying anchored to the Node
    //! or the Way given at construction. This is how a building occupies the
    //! cell its Area chose for it while being served by a road that runs a few
    //! cells away.
    // -------------------------------------------------------------------------
    void placeAt(Vector3f const& position);

private:

    void bind(City& city);

private:

    uint32_t        m_id = 0u;
    UnitType const& m_type;
    Vector3f        m_position;
    Node*           m_node = nullptr;
    Way*            m_way = nullptr;
    float           m_offset = 0.0f;
    //! \brief Whether m_position is the footprint of the Unit rather than the
    //! position of its anchor. Set by placeAt().
    bool            m_placed = false;
    Resources       m_resources;
    RuleContext     m_context;
    uint32_t        m_ticks = 0u;
};

using Units = std::vector<std::unique_ptr<Unit>>;

} // namespace ogb

#endif
