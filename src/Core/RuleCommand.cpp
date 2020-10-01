//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/RuleCommand.hpp"
#include "Core/City.hpp"

//------------------------------------------------------------------------------
bool RuleCommandAdd::validate(RuleContext& context)
{
    return m_target.get(context) < m_target.capacity(context);
}

//------------------------------------------------------------------------------
void RuleCommandAdd::execute(RuleContext& context)
{
    m_target.add(context, m_amount);
}

//------------------------------------------------------------------------------
bool RuleCommandRemove::validate(RuleContext& context)
{
    return m_target.get(context) >= m_amount;
}

//------------------------------------------------------------------------------
void RuleCommandRemove::execute(RuleContext& context)
{
    m_target.remove(context, m_amount);
}

//------------------------------------------------------------------------------
bool RuleCommandTest::validate(RuleContext& context)
{
    switch (m_comparison)
    {
    case Comparison::EQUALS:
        return m_target.get(context) == m_amount;
    case Comparison::GREATER:
        return m_target.get(context) > m_amount;
    case Comparison::LESS:
    default:
        return m_target.get(context) < m_amount;
    }
}

//------------------------------------------------------------------------------
void RuleCommandTest::execute(RuleContext& context)
{
    // Do nothing
}

//------------------------------------------------------------------------------
bool RuleCommandAgent::validate(RuleContext& /*context*/)
{
    return true;
}

//------------------------------------------------------------------------------
void RuleCommandAgent::execute(RuleContext& context)
{
    context.city->addAgent(*this, *(context.unit), m_resources, m_target);
}
