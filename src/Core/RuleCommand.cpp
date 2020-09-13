#include "Core/RuleCommand.hpp"
#include "Core/City.hpp"

#if 0
//------------------------------------------------------------------------------
bool RuleCommandAdd::validate(RuleContext const& context) const
{
    return target.get(context) < target.capacity(context);
}

//------------------------------------------------------------------------------
void RuleCommandAdd::execute(RuleContext& context)
{
    target.add(context, amount);
}

//------------------------------------------------------------------------------
bool RuleCommandRemove::validate(RuleContext const& context) const
{
    return target.get(context) >= amount;
}

//------------------------------------------------------------------------------
void RuleCommandRemove::execute(RuleContext& context)
{
    target.remove(context, amount);
}
#endif

//------------------------------------------------------------------------------
bool RuleCommandAgent::validate(RuleContext const& /*context*/) const
{
    return true;
}

//------------------------------------------------------------------------------
void RuleCommandAgent::execute(RuleContext& context)
{
    context.city->addAgent(/*agentType, FIXME*/
        context.unit->node(),
        *(context.unit),
        resources,
        target);
}
