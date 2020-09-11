#ifndef UNIT_HPP
#  define UNIT_HPP

#  include "Rule.hpp"
#  include "Resources.hpp"
#  include "Path.hpp"
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

    struct Config
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
    void configure(Unit::Config const& conf, City& city);

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

#endif
