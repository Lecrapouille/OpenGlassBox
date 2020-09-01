#ifndef UNIT_HPP
#  define UNIT_HPP

//#  include "Rule.hpp"
//#  include "SimContext.hpp"
#  include "Path.hpp"

#  include <string>
#  include <vector>
#  include <algorithm>

// -----------------------------------------------------------------------------
//! \brief Represent things: houses, factories, even people.
//! A unit has state: A collection of resource bins
//! Also a well-defined spatial extent: Bounding volume, Simulation footprint.
// -----------------------------------------------------------------------------
class Unit // TODO : public Node ?
{
public:

    Unit(std::string const& id, Node const& node);
    ~Unit();
    void executeRules();
    bool accepts(std::string const& searchTarget,
                 ResourceBin const& resourcesToTryToAdd);
    std::string const& id() const { return m_id; }
    ResourceBin& resources() { return m_resources; }

protected:

    std::string              m_id;
    uint32_t                 m_color;
    Node                     m_node;
    ResourceBin              m_resources;
    //RuleContext            m_context;
    //std::vector<RuleUnit>    m_rules;
    std::vector<std::string> m_targets;
    uint32_t                 m_ticks = 0u;
};

#endif
