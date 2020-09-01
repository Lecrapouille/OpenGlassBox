#include "Unit.hpp"

// TODO: a completer

Unit::Unit(UnitType const& initValues, Point const& position)
{
    m_id = initValues.id;
    m_color = initValues.color;

    m_position = position;
    //m_position.units.add(this);

    m_resources.setCapacities(initValues.capacities);
    m_resources.addResources(initValues.resources);

    //m_context.m_unit = this;
    //m_context.m_box = m_position.path.box;
    //m_context.m_globalResources = m_context.m_box.globals;
    //m_context.m_localResources = &m_resources;

    m_rules = initValues.rules;
    m_targets = initValues.targets;
}

Unit::~Unit()
{
    //m_position.units.remove(this);
}

void Unit::executeRules()
{
    m_ticks++;

    //m_position.getMapPosition(m_context.mapPositionX, m_context.mapPositionY);
    for (size_t i = 0u; i < m_rules.size(); ++i)
    {
        if (m_ticks % m_rules[i].rate == 0)
            m_rules[i].execute(/*m_context*/);
    }
}

bool Unit::accepts(std::string const& searchTarget, ResourceBin const& resourcesToTryToAdd)
{
    return find(m_targets.begin(), m_targets.end(), searchTarget) != m_targets.end()
            && m_resources.canAddSomeResources(resourcesToTryToAdd);
}
