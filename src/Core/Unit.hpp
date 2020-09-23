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
//class RuleContext;
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

    UnitType(std::string const& name)
        : m_name(name),
          m_color(0xFFFFFF)
    {}

    UnitType(std::string const& name,
             uint32_t color,
             uint32_t radius,
             Resources resources,
             std::vector<RuleUnit*> rules, // TODO reflexion lazy search: vec<string> puis m_rules(getRuleUnit(string))
             std::vector<std::string> targets)
        : m_name(name),
          m_color(color),
          m_radius(radius),
          m_resources(resources),
          m_rules(rules),
          m_targets(targets)
    {}

    std::string              m_name;
    uint32_t                 m_color;
    uint32_t                 m_radius;
    Resources                m_resources;
    std::vector<RuleUnit*>   m_rules;
    std::vector<std::string> m_targets;
};

//==============================================================================
//! \brief A Unit represents things: houses, factories, even people.
//! A unit has state: a collection of resource.
//! But also a well-defined spatial extent: bounding volume, simulation
//! footprint.
//==============================================================================
class Unit: private UnitType
{
public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Unit(UnitType const& type, Node& node, City& city);

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
    inline std::string const& name() const { return m_name; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Resources& resources() { return m_resources; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Vector3f& position() { return m_node.position(); }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline Node& node() { return m_node; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t color() const { return m_color; }

protected:

    Node          &m_node;
    RuleContext    m_context;
    uint32_t       m_ticks = 0u;
};

using Units = std::vector<std::unique_ptr<Unit>>;

#endif
