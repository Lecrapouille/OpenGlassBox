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

//==============================================================================
//! \brief A Unit represents things: houses, factories, even people.
//! A unit has state: a collection of resource.
//! But also a well-defined spatial extent: bounding volume, simulation
//! footprint.
//==============================================================================
class Unit
{
public:

    //==========================================================================
    //! \brief Type of Units (Home, Work ...).
    //! Class constructed during the parsing of simulation scripts.
    //! Examples:
    //!  - unit Home color 0xFF00FF mapRadius 1 rules [ SendPeopleToWork ]
    //!          targets [ Home ] caps [ People 4 ] resources [ People 4 ]
    //==========================================================================
    struct Type
    {
        uint32_t                 color;
        Resources                capacities;
        Resources                resources;
        std::vector<RuleUnit*>   rules;
        std::vector<std::string> targets;
    };

public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Unit(std::string const& id, Node& node);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void configure(Unit::Type const& conf, City& city);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    ~Unit();

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
    inline std::string const& id() const { return m_id; }

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

protected:

    std::string              m_id;
    uint32_t                 m_color;
    Node                    &m_node;
    Resources                m_resources;
    RuleContext              m_context;
    std::vector<RuleUnit*>   m_rules;
    std::vector<std::string> m_targets;
    uint32_t                 m_ticks = 0u;
};

using Units = std::vector<std::unique_ptr<Unit>>;

#endif
