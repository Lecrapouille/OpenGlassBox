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
    return context.globals->getAmount(m_resource.getTypeName());
}

uint32_t RuleValueGlobal::getCapacity(RuleContext& context)
{
    // The script has no syntax to cap a global stock, so a city treasury the
    // ruleset never mentioned is unbounded. Reading a missing bin as a capacity
    // of zero used to refuse every "global Money add 1", which is why nothing
    // was ever sold and no zone could grow on the proceeds.
    if (!context.globals->hasResource(m_resource.getTypeName()))
        return Resource::MAX_CAPACITY;

    return context.globals->getCapacity(m_resource.getTypeName());
}

void RuleValueGlobal::add(RuleContext& context, uint32_t toAdd)
{
    context.globals->addResource(m_resource.getTypeName(), toAdd);
}

void RuleValueGlobal::remove(RuleContext& context, uint32_t toRemove)
{
    context.globals->removeResource(m_resource.getTypeName(), toRemove);
}

Name const& RuleValueGlobal::getTypeName() const
{
    return m_resource.getTypeName();
}

// ----

uint32_t RuleValueLocal::get(RuleContext& context)
{
    return context.locals->getAmount(m_resource.getTypeName());
}

uint32_t RuleValueLocal::getCapacity(RuleContext& context)
{
    return context.locals->getCapacity(m_resource.getTypeName());
}

void RuleValueLocal::add(RuleContext& context, uint32_t toAdd)
{
    context.locals->addResource(m_resource.getTypeName(), toAdd);
}

void RuleValueLocal::remove(RuleContext& context, uint32_t toRemove)
{
    context.locals->removeResource(m_resource.getTypeName(), toRemove);
}

Name const& RuleValueLocal::getTypeName() const
{
    return m_resource.getTypeName();
}

// ----

Layer& RuleValueLayer::layer(RuleContext& context)
{
    City const* const city = context.city;
    if ((m_layer == nullptr) || (m_city != city))
    {
        m_layer = &(context.city->getLayer(m_layerId.str()));
        m_city = city;
    }
    return *m_layer;
}

uint32_t RuleValueLayer::get(RuleContext& context)
{
    return layer(context).getResource(context.cell, context.radius,
                                    context.city->getRegion());
}

uint32_t RuleValueLayer::getCapacity(RuleContext& context)
{
    Layer& field = layer(context);
    uint32_t const perCell = field.getCellCapacity();

    // get() sums every cell the radius covers, so the capacity has to be the
    // capacity of the same set of cells. Comparing a sum over a building's
    // whole footprint against the capacity of a single cell is what stopped
    // "layer Pollution add 1" from ever validating once the neighbourhood held
    // more than one cell worth of pollution, and with it the whole rule.
    uint32_t const cells = field.countCellsInRadius(
        context.cell, context.radius, context.city->getRegion());

    if ((perCell == 0u) || (cells == 0u))
        return 0u;
    if (perCell > (Resource::MAX_CAPACITY / cells))
        return Resource::MAX_CAPACITY;

    return perCell * cells;
}

void RuleValueLayer::add(RuleContext& context, uint32_t toAdd)
{
    layer(context).addResource(context.cell, context.radius,
                             context.city->getRegion(), toAdd);
}

void RuleValueLayer::remove(RuleContext& context, uint32_t toRemove)
{
    layer(context).removeResource(context.cell, context.radius,
                                context.city->getRegion(), toRemove);
}

Name const& RuleValueLayer::getTypeName() const
{
    return m_layerId;
}

} // namespace ogb
