#ifndef UNIT_HPP
#  define UNIT_HPP

#  include "Base.hpp"
#  include "Rule.hpp"
//#  include "SimContext.hpp"
//#  include "Point.hpp"

#  include <string>
#  include <vector>
#  include <algorithm>

// -----------------------------------------------------------------------------
//! \brief Represent things: houses, factories, even people.
//! A unit has state: A collection of resource bins
//! Also a well-defined spatial extent: Bounding volume, Simulation footprint.
// -----------------------------------------------------------------------------
class Unit: protected UnitType
{
public:

    Unit(UnitType const& initValues, Point const& position);
    ~Unit();
    void executeRules();
    bool accepts(std::string const& searchTarget, ResourceBin const& resourcesToTryToAdd);

protected:

    std::string              m_id;
    Point                    m_position;
    ResourceBin              m_resources;
    //RuleContext              m_context;
    uint32_t                 m_ticks = 0u;
};

#endif
