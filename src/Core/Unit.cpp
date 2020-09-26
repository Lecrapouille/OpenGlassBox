#include "Core/Unit.hpp"
#include "Core/City.hpp"

// -----------------------------------------------------------------------------
Unit::Unit(UnitType& type, Node& node, City& city)
    : m_type(type), m_node(node)
{
    m_node.addUnit(*this);

    // References states needed for running Rules
    m_context.unit = this;
    m_context.city = &city;
    m_context.globals = &(city.globalResources());
    m_context.locals = &(type.resources);
    m_context.radius = type.radius;
    m_node.getMapPosition(city, m_context.u, m_context.v);
}

// -----------------------------------------------------------------------------
void Unit::executeRules()
{
    m_ticks += 1u;

    size_t i = m_type.rules.size();
    while (i--)
    {
        if (m_ticks % m_type.rules[i]->rate() == 0u)
        {
            m_type.rules[i]->execute(m_context);
        }
    }
}

// -----------------------------------------------------------------------------
bool Unit::accepts(std::string const& searchTarget, Resources const& resourcesToTryToAdd)
{
    return (m_type.resources.canAddSomeResources(resourcesToTryToAdd)) &&
            ((find(m_type.targets.begin(), m_type.targets.end(), searchTarget)
              != m_type.targets.end()));
}
