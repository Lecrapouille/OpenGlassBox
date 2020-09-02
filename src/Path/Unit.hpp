#ifndef UNIT_HPP
#  define UNIT_HPP

#  include "ResourceBin.hpp"

#  include "Rule.hpp"
//#  include "SimContext.hpp"
#  include "Path.hpp"
#  include <string>
#  include <vector>
#  include <algorithm>

// =============================================================================
//! \brief A Unit represents things: houses, factories, even people.
//! A unit has state: a collection of resource.
//! But also a well-defined spatial extent: bounding volume, simulation footprint.
// =============================================================================
class Unit
{
public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Unit(std::string const& id, Node const& node);

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
                 ResourceBin const& resourcesToTryToAdd);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline std::string const& id() const { return m_id; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline ResourceBin& resources() { return m_resources; }

protected:

    std::string              m_id;
    uint32_t                 m_color;
    Node                     m_node;
    ResourceBin              m_resources;
    //RuleContext            m_context;
    std::vector<RuleUnit>    m_rules;
    std::vector<std::string> m_targets;
    uint32_t                 m_ticks = 0u;
};

#endif
