//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/RuleCommand.hpp"
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Zone.hpp"
#include "OpenGlassBox/World.hpp"
#include <cassert>
#include <iostream>
#include <optional>
#include <sstream>

//------------------------------------------------------------------------------
namespace ogb {

bool RuleCommandAdd::validate(RuleContext& context)
{
    return m_target.get(context) < m_target.getCapacity(context);
}

//------------------------------------------------------------------------------
void RuleCommandAdd::execute(RuleContext& context)
{
    m_target.add(context, m_amount);
}

//------------------------------------------------------------------------------
std::string RuleCommandAdd::getDescription() const
{
    std::stringstream ss;
    ss << "Add " << m_amount << " Resources " << m_target.getTypeName();
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
std::string RuleCommandRemove::getDescription() const
{
    std::stringstream ss;
    ss << "Remove " << m_amount << " Resources " << m_target.getTypeName();
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
std::string RuleCommandTest::getDescription() const
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
    ss << m_amount << " Resources " << m_target.getTypeName();
    return ss.str().c_str();
}

//------------------------------------------------------------------------------
bool RuleCommandAgent::validate(RuleContext& context)
{
    if ((context.building == nullptr) || (context.city == nullptr))
        return false;

    if (!context.building->hasSegments())
        return false;

    Resources probe;
    for (auto const& resource: m_resources.getAll())
    {
        if (context.building->getResources().getAmount(resource.getTypeName()) <
            resource.getAmount())
        {
            return false;
        }
        probe.addResource(resource.getTypeName(), resource.getAmount());
    }

    // What an Agent looks for is a name in the targets of a Building, not the
    // name of its type. Building::accepts is what the router and the Agent both
    // use, so asking anything more here refuses Rules the run time would have
    // honoured: a ruleset with tiers names its houses Shack, House and Villa and
    // has none called Home, and every "agent Worker to Home" stopped validating.
    for (auto& building: context.city->getBuildings())
    {
        if (building->accepts(m_target, probe))
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
void RuleCommandAgent::execute(RuleContext& context)
{
    if ((context.building == nullptr) || (context.city == nullptr))
        return;

    if (context.building->hasSegments())
    {
        context.city->addAgent(
            m_type, *(context.building), m_resources, m_target);
    }
#if !defined(NDEBUG)
    else
    {
       std::cerr << "Ill-formed: Building " << context.building->getId() << " is attached "
                 << "to a orphan Path Node and its Agent will not be able to "
                 << "move towards the City." << std::endl;
    }
#endif
}

//------------------------------------------------------------------------------
std::string RuleCommandAgent::getDescription() const
{
    return {"Add Agent"};
}

//------------------------------------------------------------------------------
bool RuleCommandHour::validate(RuleContext& context)
{
    if (context.clock == nullptr)
        return false;
    return context.clock->isHourBetween(m_from, m_to);
}

//------------------------------------------------------------------------------
void RuleCommandHour::execute(RuleContext& /*context*/)
{}

//------------------------------------------------------------------------------
std::string RuleCommandHour::getDescription() const
{
    std::stringstream ss;
    ss << "Hour between " << m_from << " and " << m_to;
    return ss.str();
}

//------------------------------------------------------------------------------
bool RuleCommandCount::validate(RuleContext& context)
{
    if (context.zone == nullptr)
        return false;

    uint32_t const n = context.zone->countBuildings(m_buildingType);
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
std::string RuleCommandCount::getDescription() const
{
    std::stringstream ss;
    ss << "Count " << m_buildingType;
    return ss.str();
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief How far a building may stand from the road that serves it, in world
//! buildings. A couple of cells: the driveway, not a hike.
//------------------------------------------------------------------------------
static float maxAccessDistance(City const& city)
{
    static constexpr float ACCESS_CELLS = 2.0f;
    return ACCESS_CELLS * city.getCellSize();
}

//------------------------------------------------------------------------------
bool RuleCommandSpawn::validate(RuleContext& context)
{
    if (context.zone == nullptr || context.city == nullptr)
        return false;

    if (m_placement != Placement::NearestSegment)
        return context.zone->findFreeCell().has_value();

    // A building has to be reachable. Without a road, no Agent could ever
    // leave it or deliver to it, and the Zone would keep growing ghosts, so the
    // free cell is looked for along the network instead of anywhere in the
    // footprint.
    std::optional<Cell> const cell = context.zone->findFreeCellNearRoad();
    if (!cell.has_value())
        return false;

    float offset = 0.5f;
    Segment const* segment = context.zone->findNearestSegment(
        context.zone->getCellCentre(*cell), offset,
        maxAccessDistance(*context.city));
    return (segment != nullptr) && (segment->getFrom().getPath() != nullptr);
}

//------------------------------------------------------------------------------
void RuleCommandSpawn::execute(RuleContext& context)
{
    if ((context.zone == nullptr) || (context.city == nullptr))
        return;

    if (m_placement != Placement::NearestSegment)
    {
        std::optional<Cell> const free = context.zone->findFreeCell();
        if (!free.has_value())
            return;
        context.city->addBuilding(m_buildingType,
                              context.zone->getCellCentre(*free));
        return;
    }

    std::optional<Cell> const cell = context.zone->findFreeCellNearRoad();
    if (!cell.has_value())
        return;

    Vector3f const world = context.zone->getCellCentre(*cell);
    float offset = 0.5f;
    Segment* segment = context.zone->findNearestSegment(world, offset,
                                            maxAccessDistance(*context.city));
    Path* path = (segment == nullptr) ? nullptr : segment->getFrom().getPath();
    if ((segment == nullptr) || (path == nullptr))
        return;

    // The Segment is how the building reaches the network, not where it stands: it
    // keeps the cell the Zone picked for it. Reading the cell of the road
    // instead would leave that cell free, and the next tick would grow another
    // building on the very same spot.
    Building& building = context.city->addBuilding(m_buildingType, *path, *segment, offset);
    building.setPosition(world);
}

//------------------------------------------------------------------------------
std::string RuleCommandSpawn::getDescription() const
{
    return "Spawn " + m_buildingType.name.str();
}

//------------------------------------------------------------------------------
bool RuleCommandUpgrade::validate(RuleContext& context)
{
    return (context.zone != nullptr) &&
           (context.zone->countBuildings(m_fromType.name) > 0u);
}

//------------------------------------------------------------------------------
void RuleCommandUpgrade::execute(RuleContext& context)
{
    auto buildings = context.zone->getBuildingsInside(m_fromType.name);
    if (buildings.empty())
        return;

    Building* building = buildings.front();
    Vector3f const position = building->getPosition();
    Node* node = building->getNode();
    Segment* segment = building->getSegment();
    float const offset = building->getSegmentOffset();
    Path* path = building->getPath();

    // An upgrade replaces one building by another, and a copy of the stock is
    // the only thing that survives the removal.
    Resources const stock = building->getResources();

    context.city->removeBuilding(*building);

    Building* upgraded = nullptr;
    if (node != nullptr)
        upgraded = &(context.city->addBuilding(m_toType, *node));
    else if ((segment != nullptr) && (path != nullptr))
        upgraded = &(context.city->addBuilding(m_toType, *path, *segment, offset));
    else
        upgraded = &(context.city->addBuilding(m_toType, position));

    transferStock(stock, *upgraded);
}

//------------------------------------------------------------------------------
void RuleCommandUpgrade::transferStock(Resources const& stock, Building& upgraded)
{
    // A house that becomes a better house keeps the people who live in it. The
    // new building used to start from the resources of its type, so every
    // inhabitant disappeared, and a zone that upgraded its houses emptied the
    // city it was meant to grow.
    Resources& kept = upgraded.getResources();

    for (Resource const& resource : stock.getAll())
    {
        // Only what the new type declares. A shop with no room for people is
        // given none, rather than a stock its own rules never mention.
        if (!kept.hasResource(resource.getTypeName()))
            continue;

        // Replace what the new type starts with rather than add to it, and let
        // addResource() drop what is above the new capacity: a tower holds more
        // people than a shack, a shack holds fewer than a tower.
        kept.removeResource(resource.getTypeName(), Resource::MAX_CAPACITY);
        kept.addResource(resource.getTypeName(), resource.getAmount());
    }
}

//------------------------------------------------------------------------------
std::string RuleCommandUpgrade::getDescription() const
{
    return "Upgrade " + m_fromType.name.str() + " to " + m_toType.name.str();
}

//------------------------------------------------------------------------------
bool RuleCommandDestroy::validate(RuleContext& context)
{
    return (context.zone != nullptr) &&
           (context.zone->countBuildings(m_buildingType) > 0u);
}

//------------------------------------------------------------------------------
void RuleCommandDestroy::execute(RuleContext& context)
{
    auto buildings = context.zone->getBuildingsInside(m_buildingType);
    if (buildings.empty())
        return;

    // A neighbourhood in decline loses its empty houses first. The command used
    // to take the first building of the zone, which is the oldest one, so a
    // crowded house could fall while an empty one beside it stayed: the zone lost
    // its inhabitants faster than it lost its buildings.
    Building* emptiest = buildings.front();
    uint32_t fewest = countStock(*emptiest);

    for (Building* candidate : buildings)
    {
        uint32_t const stock = countStock(*candidate);
        if (stock < fewest)
        {
            fewest = stock;
            emptiest = candidate;
        }
    }

    context.city->removeBuilding(*emptiest);
}

//------------------------------------------------------------------------------
uint32_t RuleCommandDestroy::countStock(Building const& building)
{
    // Everything the building holds, whatever its name: the command does not
    // know which resource the script calls its inhabitants.
    uint32_t total = 0u;

    for (Resource const& resource : building.getResources().getAll())
    {
        if (resource.getAmount() >= Resource::MAX_CAPACITY - total)
            return Resource::MAX_CAPACITY;
        total += resource.getAmount();
    }

    return total;
}

//------------------------------------------------------------------------------
std::string RuleCommandDestroy::getDescription() const
{
    return "Destroy " + m_buildingType.str();
}

} // namespace ogb
