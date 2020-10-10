//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_UNIT_HPP
#  define OPEN_GLASSBOX_UNIT_HPP

#  include "Core/Path.hpp"
#  include "Core/Rule.hpp"
#  include <algorithm>

class RuleUnit;
class City;

//==============================================================================
//! \brief A Unit represents things: houses, factories, even people. A unit has
//! state: a collection of resource but also a well-defined spatial extent:
//! bounding volume, simulation footprint. Currently implemented, an Unit shall
//! refer to an existing Path Node to allow Agent to carry Resources from an
//! Unit to another Unit.
//==============================================================================
class Unit
{
public:

    // -------------------------------------------------------------------------
    //! \brief Create a new Unit instance placed on an existing Path's Node.
    //!
    //! \param[in] type: const reference of a given type of Unit also refered
    //! internally. The refered instance shall not be deleted before this Unit
    //! instance is destroyed.
    //!
    //! \param[in] node: The reference to the Path Node owning this instance
    //! also refered internally. The refered instance shall not be deleted
    //! before this Unit instance is destroyed.
    //!
    //! \param[in] city: The reference to the City owning this instance also
    //! refered internally. The refered instance shall not be deleted before
    //! this Unit instance is destroyed.
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Node& node, City& city);

    // -------------------------------------------------------------------------
    //! \brief Virtual destructor only needed because of the presence of virtual
    //! methods only needed for unit tests.
    // -------------------------------------------------------------------------
    VIRTUAL ~Unit() = default;

    // -------------------------------------------------------------------------
    //! \brief Execute simulation rules given by UnitType (defined by the
    //! simulation script).
    //! \note VIRTUAL is only used for unit tests.
    // -------------------------------------------------------------------------
    VIRTUAL void executeRules();

    // -------------------------------------------------------------------------
    //! \brief Check if the current resources \c m_resources of this instance of
    //! Unit can add external resources \c resourcesToTryToAdd from a matching
    //! target.
    // -------------------------------------------------------------------------
    bool accepts(std::string const& searchTarget, Resources const& resourcesToTryToAdd);

    // -------------------------------------------------------------------------
    //! \brief Getter: return the type of Unit.
    // -------------------------------------------------------------------------
    inline std::string const& type() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief Return current resources.
    // -------------------------------------------------------------------------
    inline Resources& resources() { return m_resources; }

    // -------------------------------------------------------------------------
    //! \brief Return the position inside the World coordinate of the Path Node that is refering.
    // -------------------------------------------------------------------------
    inline Vector3f const& position() const { return m_node.position(); }

    // -------------------------------------------------------------------------
    //! \brief Return the color for the Renderer.
    // -------------------------------------------------------------------------
    inline uint32_t color() const { return m_type.color; }

    // -------------------------------------------------------------------------
    //! \brief Return the associated Path Node.
    // -------------------------------------------------------------------------
    inline Node& node() { return m_node; }

    // -------------------------------------------------------------------------
    //! \brief Return the unique identifier.
    // -------------------------------------------------------------------------
    inline uint32_t id() const { return m_node.id(); }

    // -------------------------------------------------------------------------
    //! \brief Check if can access to at least one Way. And Unit shall refer to
    //! a Node with neighbors else Agents cannot move towards Path.
    // -------------------------------------------------------------------------
    inline bool hasWays() const { return m_node.hasWays(); }

private:

    //! \brief Reference to the type of Unit defined in the simulation script.
    //! The reference shall not be destroyed before this instance.
    UnitType const& m_type;
    //! \brief Reference to Node of a Path.
    //! The reference shall not be destroyed before this instance.
    Node          & m_node;
    //! \brief Unit produce resources has defined by simulation scripts.
    Resources       m_resources;
    //! \brief Structure holding usefull information for the good execution of
    //! UnitRules.
    RuleContext     m_context;
    //! \brief Discrete time for running UnitRules at the rate time defined by
    //! simulation scripts (UnitType).
    uint32_t        m_ticks = 0u;
};

using Units = std::vector<std::unique_ptr<Unit>>;

#endif
