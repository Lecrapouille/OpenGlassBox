//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/RuleValue.hpp"
#include "OpenGlassBox/City.hpp"

namespace ogb {

uint32_t RuleValueGlobal::get(RuleContext& context)
{
    return context.globals->getAmount(m_resource.type());
}

uint32_t RuleValueGlobal::capacity(RuleContext& context)
{
    // The script has no syntax to cap a global stock, so a city treasury the
    // ruleset never mentioned is unbounded. Reading a missing bin as a capacity
    // of zero used to refuse every "global Money add 1", which is why nothing
    // was ever sold and no zone could grow on the proceeds.
    if (!context.globals->hasResource(m_resource.type()))
        return Resource::MAX_CAPACITY;

    return context.globals->getCapacity(m_resource.type());
}

void RuleValueGlobal::add(RuleContext& context, uint32_t toAdd)
{
    context.globals->addResource(m_resource.type(), toAdd);
}

void RuleValueGlobal::remove(RuleContext& context, uint32_t toRemove)
{
    context.globals->removeResource(m_resource.type(), toRemove);
}

std::string const& RuleValueGlobal::type() const
{
    return m_resource.type();
}

// ----

uint32_t RuleValueLocal::get(RuleContext& context)
{
    return context.locals->getAmount(m_resource.type());
}

uint32_t RuleValueLocal::capacity(RuleContext& context)
{
    return context.locals->getCapacity(m_resource.type());
}

void RuleValueLocal::add(RuleContext& context, uint32_t toAdd)
{
    context.locals->addResource(m_resource.type(), toAdd);
}

void RuleValueLocal::remove(RuleContext& context, uint32_t toRemove)
{
    context.locals->removeResource(m_resource.type(), toRemove);
}

std::string const& RuleValueLocal::type() const
{
    return m_resource.type();
}

// ----

Map& RuleValueMap::map(RuleContext& context)
{
    World const* const world = &(context.city->world());
    if ((m_map == nullptr) || (m_world != world))
    {
        m_map = &(context.city->getMap(m_mapId));
        m_world = world;
    }
    return *m_map;
}

uint32_t RuleValueMap::get(RuleContext& context)
{
    return map(context).getResource(context.u, context.v, context.radius,
                                    context.city->region());
}

uint32_t RuleValueMap::capacity(RuleContext& context)
{
    Map& field = map(context);
    uint32_t const perCell = field.getCapacity();

    // get() sums every cell the radius covers, so the capacity has to be the
    // capacity of the same set of cells. Comparing a sum over a building's
    // whole footprint against the capacity of a single cell is what stopped
    // "map Pollution add 1" from ever validating once the neighbourhood held
    // more than one cell worth of pollution, and with it the whole rule.
    uint32_t const cells = field.cellsInRadius(
        context.u, context.v, context.radius, context.city->region());

    if ((perCell == 0u) || (cells == 0u))
        return 0u;
    if (perCell > (Resource::MAX_CAPACITY / cells))
        return Resource::MAX_CAPACITY;

    return perCell * cells;
}

void RuleValueMap::add(RuleContext& context, uint32_t toAdd)
{
    map(context).addResource(context.u, context.v, context.radius,
                             context.city->region(), toAdd);
}

void RuleValueMap::remove(RuleContext& context, uint32_t toRemove)
{
    map(context).removeResource(context.u, context.v, context.radius,
                                context.city->region(), toRemove);
}

std::string const& RuleValueMap::type() const
{
    return m_mapId;
}

} // namespace ogb
