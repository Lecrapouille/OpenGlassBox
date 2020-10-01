//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/RuleValue.hpp"
#include "Core/City.hpp"

uint32_t RuleValueGlobal::get(RuleContext& context)
{
    return context.globals->getAmount(resource.type());
}

uint32_t RuleValueGlobal::capacity(RuleContext& context)
{
    return context.globals->getCapacity(resource.type());
}

void RuleValueGlobal::add(RuleContext& context, uint32_t toAdd)
{
    context.globals->addResource(resource.type(), toAdd);
}

void RuleValueGlobal::remove(RuleContext& context, uint32_t toRemove)
{
    context.globals->removeResource(resource.type(), toRemove);
}

// ----

uint32_t RuleValueLocal::get(RuleContext& context)
{
    return context.locals->getAmount(resource.type());
}

uint32_t RuleValueLocal::capacity(RuleContext& context)
{
    return context.locals->getCapacity(resource.type());
}

void RuleValueLocal::add(RuleContext& context, uint32_t toAdd)
{
    context.locals->addResource(resource.type(), toAdd);
}

void RuleValueLocal::remove(RuleContext& context, uint32_t toRemove)
{
    context.locals->removeResource(resource.type(), toRemove);
}

// ----

uint32_t RuleValueMap::get(RuleContext& context)
{
    return context.city->getMap(mapId).getResource(context.u, context.v, context.radius);
}

uint32_t RuleValueMap::capacity(RuleContext& context)
{
    return context.city->getMap(mapId).getCapacity();
}

void RuleValueMap::add(RuleContext& context, uint32_t toAdd)
{
    context.city->getMap(mapId).addResource(context.u, context.v, context.radius, toAdd);
}

void RuleValueMap::remove(RuleContext& context, uint32_t toRemove)
{
    context.city->getMap(mapId).removeResource(context.u, context.v, context.radius, toRemove);
}
