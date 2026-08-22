//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/RuleCommand.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Area.hpp"
#include "OpenGlassBox/World.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

//------------------------------------------------------------------------------
namespace ogb {

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
bool RuleCommandAgent::validate(RuleContext& context)
{
    if ((context.unit == nullptr) || (context.city == nullptr))
        return false;

    if (!context.unit->hasWays())
        return false;

    Resources probe;
    for (auto const& resource: m_resources.container())
    {
        if (context.unit->resources().getAmount(resource.type()) <
            resource.getAmount())
        {
            return false;
        }
        probe.addResource(resource.type(), resource.getAmount());
    }

    for (auto& unit: context.city->units())
    {
        if (unit->type() != m_target)
            continue;
        if (unit->accepts(m_target, probe))
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void RuleCommandAgent::execute(RuleContext& context)
{
    if ((context.unit == nullptr) || (context.city == nullptr))
        return;

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

//------------------------------------------------------------------------------
bool RuleCommandHour::validate(RuleContext& context)
{
    if (context.clock == nullptr)
        return false;
    return context.clock->hourBetween(m_from, m_to);
}

//------------------------------------------------------------------------------
void RuleCommandHour::execute(RuleContext& /*context*/)
{}

//------------------------------------------------------------------------------
std::string RuleCommandHour::type()
{
    std::stringstream ss;
    ss << "Hour between " << m_from << " and " << m_to;
    return ss.str();
}

//------------------------------------------------------------------------------
bool RuleCommandCount::validate(RuleContext& context)
{
    if (context.area == nullptr)
        return false;

    uint32_t const n = context.area->countUnits(m_unitType);
    switch (m_comparison)
    {
    case RuleCommandTest::Comparison::EQUALS:
        return n == m_amount;
    case RuleCommandTest::Comparison::GREATER:
        return n > m_amount;
    case RuleCommandTest::Comparison::LESS:
        return n < m_amount;
    default:
        return false;
    }
}

//------------------------------------------------------------------------------
void RuleCommandCount::execute(RuleContext& /*context*/)
{}

//------------------------------------------------------------------------------
std::string RuleCommandCount::type()
{
    std::stringstream ss;
    ss << "Count " << m_unitType;
    return ss.str();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief How far a building may stand from the road that serves it, in world
//! units. A couple of cells: the driveway, not a hike.
//------------------------------------------------------------------------------
static float maxAccessDistance(City const& city)
{
    static constexpr float ACCESS_CELLS = 2.0f;
    return ACCESS_CELLS * city.gridCellSize();
}

//------------------------------------------------------------------------------
bool RuleCommandSpawn::validate(RuleContext& context)
{
    if (context.area == nullptr || context.city == nullptr)
        return false;

    int32_t u = 0, v = 0;

    if (m_placement != Placement::NearestWay)
        return context.area->findFreeCell(u, v);

    // A building has to be reachable. Without a road, no Agent could ever
    // leave it or deliver to it, and the Area would keep growing ghosts, so the
    // free cell is looked for along the network instead of anywhere in the
    // footprint.
    if (!context.area->findBuildableCell(u, v))
        return false;

    float offset = 0.5f;
    Way const* way =
        context.area->nearestWay(context.area->cellWorldPosition(u, v), offset,
                                 maxAccessDistance(*context.city));
    return (way != nullptr) && (way->from().path() != nullptr);
}

//------------------------------------------------------------------------------
void RuleCommandSpawn::execute(RuleContext& context)
{
    if ((context.area == nullptr) || (context.city == nullptr))
        return;

    int32_t u = 0, v = 0;

    if (m_placement != Placement::NearestWay)
    {
        if (!context.area->findFreeCell(u, v))
            return;
        context.city->addUnit(m_unitType, context.area->cellWorldPosition(u, v));
        return;
    }

    if (!context.area->findBuildableCell(u, v))
        return;

    Vector3f const world = context.area->cellWorldPosition(u, v);
    float offset = 0.5f;
    Way* way =
        context.area->nearestWay(world, offset, maxAccessDistance(*context.city));
    Path* path = (way == nullptr) ? nullptr : way->from().path();
    if ((way == nullptr) || (path == nullptr))
        return;

    // The Way is how the building reaches the network, not where it stands: it
    // keeps the cell the Area picked for it. Reading the cell of the road
    // instead would leave that cell free, and the next tick would grow another
    // building on the very same spot.
    Unit& unit = context.city->addUnit(m_unitType, *path, *way, offset);
    unit.placeAt(world);
}

//------------------------------------------------------------------------------
std::string RuleCommandSpawn::type()
{
    return "Spawn " + m_unitType.name;
}

//------------------------------------------------------------------------------
bool RuleCommandUpgrade::validate(RuleContext& context)
{
    return (context.area != nullptr) &&
           (context.area->countUnits(m_fromType.name) > 0u);
}

//------------------------------------------------------------------------------
void RuleCommandUpgrade::execute(RuleContext& context)
{
    auto units = context.area->unitsInside(m_fromType.name);
    if (units.empty())
        return;

    Unit* unit = units.front();
    Vector3f const position = unit->position();
    Node* node = unit->node();
    Way* way = unit->way();
    float const offset = unit->wayOffset();
    Path* path = unit->path();

    context.city->removeUnit(*unit);

    if (node != nullptr)
        context.city->addUnit(m_toType, *node);
    else if ((way != nullptr) && (path != nullptr))
        context.city->addUnit(m_toType, *path, *way, offset);
    else
        context.city->addUnit(m_toType, position);
}

//------------------------------------------------------------------------------
std::string RuleCommandUpgrade::type()
{
    return "Upgrade " + m_fromType.name + " to " + m_toType.name;
}

//------------------------------------------------------------------------------
bool RuleCommandDestroy::validate(RuleContext& context)
{
    return (context.area != nullptr) &&
           (context.area->countUnits(m_unitType) > 0u);
}

//------------------------------------------------------------------------------
void RuleCommandDestroy::execute(RuleContext& context)
{
    auto units = context.area->unitsInside(m_unitType);
    if (!units.empty())
        context.city->removeUnit(*units.front());
}

//------------------------------------------------------------------------------
std::string RuleCommandDestroy::type()
{
    return "Destroy " + m_unitType;
}

} // namespace ogb
