#include "Core/Unit.hpp"
#include "Core/City.hpp"

Unit::Unit(std::string const& id, Node& node)
    : m_id(id),
      m_node(node)
{
    m_node.addUnit(*this);
}

Unit::~Unit()
{
    //m_node.units.remove(this);
}

void Unit::configure(Unit::Config const& conf, City& city)
{
    m_color = conf.color;
    m_rules = conf.rules;
    m_targets = conf.targets;

    m_resources.setCapacities(conf.capacities);
    m_resources.addResources(conf.resources);

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

//Units& Unit::getUnits() { return m_units; }
//Agents& Unit::getAgents() { return m_agents; }
