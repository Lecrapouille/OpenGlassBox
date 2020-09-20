#include "Core/Unit.hpp"
#include "Core/City.hpp"

Unit::Unit(UnitType const& type, Node& node, City& city)
    : UnitType(type),
      m_node(node)
{
    m_node.addUnit(*this);
    m_context.unit = this;
    m_context.city = &city;//&(m_node.path().city());
    m_context.globals = &(city.globalResources());
    m_context.locals = &m_resources;
}

void Unit::executeRules()
{
    /*    m_ticks += 1u;

    m_context.city->world2mapPosition(m_node.position(), m_context.u, m_context.v);
    for (size_t i = 0u; i < m_rules.size(); ++i)
    {
        if (m_ticks % m_rules[i]->rate() == 0)
            m_rules[i]->execute(m_context);
            }*/
}

bool Unit::accepts(std::string const& searchTarget, Resources const& resourcesToTryToAdd)
{
    return find(m_targets.begin(), m_targets.end(), searchTarget) != m_targets.end()
            && m_resources.canAddSomeResources(resourcesToTryToAdd);
}
