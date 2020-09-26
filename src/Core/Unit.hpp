#ifndef UNIT_HPP
#  define UNIT_HPP

#  include "Core/Rule.hpp"
#  include "Core/Resources.hpp"
#  include "Core/Path.hpp"
#  include "Core/Unique.hpp"
#  include <string>
#  include <vector>
#  include <algorithm>

class Rule;
class RuleUnit;
class Node;
class Resources;
class City;

//==========================================================================
//! \brief Type of Units (Home, Work ...).
//! Class constructed during the parsing of simulation scripts.
//! Examples:
//!  - unit Home color 0xFF00FF radius 1 rules [ SendPeopleToWork ]
//!          targets [ Home ] capacities [ People 4 ] resources [ People 4 ]
//==========================================================================
class UnitType
{
public:

    UnitType(UnitType const&) = default;

    UnitType(std::string const& name_)
        : name(name_), color(0xFFFFFF), radius(1u)
    {}

    UnitType(std::string const& name_, uint32_t const color_, uint32_t const radius_,
             Resources const& resources_,
             std::vector<RuleUnit*> const& rules_, // TODO reflexion lazy search: vec<string> puis rules(getRuleUnit(string))
             std::vector<std::string> const& targets_)
        : name(name_), color(color_), radius(radius_),
          resources(resources_), rules(rules_), targets(targets_)
    {}

    std::string              name;
    uint32_t                 color;
    uint32_t                 radius;
    Resources                resources;
    std::vector<RuleUnit*>   rules;
    std::vector<std::string> targets;
};

//==============================================================================
//! \brief A Unit represents things: houses, factories, even people.
//! A unit has state: a collection of resource.
//! But also a well-defined spatial extent: bounding volume, simulation
//! footprint.
//==============================================================================
class Unit
{
public:

    // -------------------------------------------------------------------------
    //! \brief
    //! TODO const& type ?
    // -------------------------------------------------------------------------
    Unit(UnitType& type, Node& node, City& city);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void executeRules();

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    bool accepts(std::string const& searchTarget,
                 Resources const& resourcesToTryToAdd);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline std::string const& name() const { return m_type.name; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Resources& resources() { return m_type.resources; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Vector3f const& position() const { return m_node.position(); }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Node& node() { return m_node; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_type.color; }

private:

    UnitType       &m_type;
    Node           &m_node;
    RuleContext     m_context;
    uint32_t        m_ticks = 0u;
};

using Units = std::vector<std::unique_ptr<Unit>>;

#endif
