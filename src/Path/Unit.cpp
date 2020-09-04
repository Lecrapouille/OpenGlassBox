#include "Unit.hpp"
#include "Path.hpp"

// TODO: a completer

Unit::Unit(std::string const& id, Node const& node)
    : m_id(id)
{
    //m_color = initValues.color;

    m_node = node;
    m_node.addUnit(*this);

    //m_resources.setCapacities(initValues.capacities);
    //m_resources.addResources(initValues.resources);

    //m_context.m_unit = this;
    //m_context.m_box = m_node.path.box;
    //m_context.m_globalResources = m_context.m_box.globals;
    //m_context.m_localResources = &m_resources;

    //m_rules = initValues.rules;
    //m_targets = initValues.targets;
}

Unit::~Unit()
{
    //m_node.units.remove(this);
}

void Unit::executeRules()
{
    m_ticks += 1u;

    /*m_node.getMapPosition(m_context.mapPositionX, m_context.mapPositionY);
    for (size_t i = 0u; i < m_rules.size(); ++i)
    {
        if (m_ticks % m_rules[i].rate == 0)
            m_rules[i].execute(m_context);
    }*/
}

bool Unit::accepts(std::string const& searchTarget, Resources const& resourcesToTryToAdd)
{
    return find(m_targets.begin(), m_targets.end(), searchTarget) != m_targets.end()
            && m_resources.canAddSomeResources(resourcesToTryToAdd);
}
