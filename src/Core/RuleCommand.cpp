//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/RuleCommand.hpp"
#include "Core/City.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

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
std::string RuleCommandAdd::type()
{
    std::stringstream ss;
    ss << "Add " << m_amount << " Resources " << m_target.type();
    return ss.str().c_str();
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
std::string RuleCommandRemove::type()
{
    std::stringstream ss;
    ss << "Remove " << m_amount << " Resources " << m_target.type();
    return ss.str().c_str();
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
        return m_target.get(context) < m_amount;
    default:
        assert(0 && "Unhandled special enum constant in RuleCommandTest::validate");
        return false;
    }
}

//------------------------------------------------------------------------------
void RuleCommandTest::execute(RuleContext& /*context*/)
{
    // Do nothing
}

//------------------------------------------------------------------------------
std::string RuleCommandTest::type()
{
    std::stringstream ss;
    switch (m_comparison)
    {
    case Comparison::EQUALS:
        ss << "Test Equal ";
        break;
    case Comparison::GREATER:
        ss << "Test Greater ";
        break;
    case Comparison::LESS:
        ss << "Test Less ";
        break;
    default:
        break;
    }
    ss << m_amount << " Resources " << m_target.type();
    return ss.str().c_str();
}

//------------------------------------------------------------------------------
bool RuleCommandAgent::validate(RuleContext& /*context*/)
{
    return true;
}

//------------------------------------------------------------------------------
void RuleCommandAgent::execute(RuleContext& context)
{
    if (context.unit->hasWays())
    {
        context.city->addAgent(*this, *(context.unit), m_resources, m_target);
    }
#if !defined(NDEBUG)
    else
    {
       std::cerr << "Ill-formed: Unit " << context.unit->id() << " is attached "
                 << "to a orphan Path Node and its Agent will not be able to "
                 << "move towards the City." << std::endl;
    }
#endif
}

//------------------------------------------------------------------------------
std::string RuleCommandAgent::type()
{
    return {"Add Agent"};
}
