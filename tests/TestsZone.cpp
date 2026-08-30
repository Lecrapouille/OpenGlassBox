#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/Zone.hpp"
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/Layer.hpp"
#  include "OpenGlassBox/RuleCommand.hpp"
#  include "OpenGlassBox/RuleValue.hpp"
#undef protected
#undef private

TEST(TestsZone, SpawnOnNearestSegment)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(90.0f, 30.0f, 0.0f));
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    BuildingType home("Home");
    RuleZoneType growType("Grow");
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestSegment);
    growType.commands.push_back(&spawn);
    RuleZone grow(growType);

    ZoneType residential("Residential");
    residential.rules.push_back(&grow);
    Zone& zone = city.addZone(residential, city.getRegion());

    ASSERT_EQ(city.getBuildings().size(), 0u);
    zone.executeRules();
    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_STREQ(city.getBuildings()[0]->getTypeName().c_str(), "Home");
    ASSERT_NE(city.getBuildings()[0]->getSegment(), nullptr);
    ASSERT_EQ(path.getNodes().size(), 2u);
}

//------------------------------------------------------------------------------
//! \brief A building belongs beside the road that serves it: the cell the Zone
//! grows it on is one the road runs through or fronts, whatever the shape of the
//! footprint.
//------------------------------------------------------------------------------
TEST(TestsZone, SpawnedBuildingsStandAlongTheRoad)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    float const side = city.getCellSize();

    Path& path = city.addPath(keep<PathType>("Road"));
    // A road along the third row of cells, crossing the whole city.
    Node& n1 = path.addNode(Vector3f(0.5f * side, 2.5f * side, 0.0f));
    Node& n2 = path.addNode(Vector3f(7.5f * side, 2.5f * side, 0.0f));
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    BuildingType home("Home");
    RuleZoneType growType("Grow");
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestSegment);
    growType.commands.push_back(&spawn);
    RuleZone grow(growType);

    ZoneType residential("Residential");
    residential.rules.push_back(&grow);
    Zone& zone = city.addZone(residential, city.getRegion());

    // More buildings than the row of cells the road runs through, so that the
    // ones that do not fit have to be refused rather than piled up.
    for (uint32_t i = 0u; i < 12u; ++i)
        zone.executeRules();

    ASSERT_FALSE(city.getBuildings().empty());
    for (auto const& building: city.getBuildings())
    {
        // Two cells at most from the road, and no two buildings on one cell.
        ASSERT_LE(std::abs(building->getCell().v - 2), 1);
        for (auto const& other: city.getBuildings())
        {
            if (other.get() == building.get())
                continue;
            ASSERT_FALSE((other->getCell().u == building->getCell().u) &&
                         (other->getCell().v == building->getCell().v));
        }
    }
}

//------------------------------------------------------------------------------
//! \brief Without a road there is no connection to make, so the Zone grows
//! nothing at all instead of scattering houses in a field.
//------------------------------------------------------------------------------
TEST(TestsZone, NoRoadNoBuilding)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    city.addPath(keep<PathType>("Road"));

    BuildingType home("Home");
    RuleZoneType growType("Grow");
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestSegment);
    growType.commands.push_back(&spawn);
    RuleZone grow(growType);

    ZoneType residential("Residential");
    residential.rules.push_back(&grow);
    Zone& zone = city.addZone(residential, city.getRegion());

    zone.executeRules();
    zone.executeRules();
    ASSERT_TRUE(city.getBuildings().empty());
}

//------------------------------------------------------------------------------
//! \brief A zone rule reads its layers on the plot the next building would stand
//! on, not on the corner of the rectangle the player painted.
//!
//! The corner is an arbitrary cell. A zone of eight by eight used to decide
//! whether it could grow from what its cell (0,0) held, however far that was
//! from any road, so "layer Water greater 10" answered a question about the
//! wrong place.
//------------------------------------------------------------------------------
TEST(TestsZone, RulesReadTheBuildablePlotNotTheCorner)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    float const side = city.getCellSize();

    // A road along the sixth row, far from the corner of the zone.
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.5f * side, 5.5f * side, 0.0f));
    Node& n2 = path.addNode(Vector3f(7.5f * side, 5.5f * side, 0.0f));
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    LayerType waterType("Water", 0x0000FF, 100u);
    Layer& water = city.addLayer(waterType);

    BuildingType home("Home");
    RuleZoneType growType("Grow");
    RuleValueLayer waterValue("Water");
    RuleCommandTest enoughWater(
        waterValue, RuleCommandTest::Comparison::GREATER, 10u);
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestSegment);
    growType.commands.push_back(&enoughWater);
    growType.commands.push_back(&spawn);
    RuleZone grow(growType);

    ZoneType residential("Residential");
    residential.rules.push_back(&grow);
    Zone& zone = city.addZone(residential, city.getRegion());

    Cell const corner{ city.getRegion().u0, city.getRegion().v0 };
    Cell const plot = zone.getRuleCell();

    // The plot sits against the road, so it is not the corner.
    ASSERT_LE(std::abs(plot.v - 5), 1);
    ASSERT_FALSE((plot.u == corner.u) && (plot.v == corner.v));

    // Water on the corner alone leaves the plot dry, and the zone must refuse.
    water.setResource(corner, 100u);
    zone.executeRules();
    ASSERT_TRUE(city.getBuildings().empty());

    // Water on the plot is what the rule asks about.
    water.setResource(corner, 0u);
    water.setResource(plot, 100u);
    zone.executeRules();
    ASSERT_EQ(city.getBuildings().size(), 1u);
}

TEST(TestsZone, CountAndDestroy)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    city.addBuilding(keep<BuildingType>("Home"), n1);

    ZoneType residential("Residential");
    Zone& zone = city.addZone(residential, city.getRegion());
    ASSERT_EQ(zone.countBuildings("Home"), 1u);

    RuleCommandDestroy destroy("Home");
    RuleContext context;
    context.zone = &zone;
    context.city = &city;
    ASSERT_TRUE(destroy.validate(context));
    destroy.execute(context);
    ASSERT_EQ(city.getBuildings().size(), 0u);
}

//------------------------------------------------------------------------------
//! \brief An upgrade keeps the stock of the building it replaces, and the new
//! capacity limits it.
//!
//! A house that becomes a better house keeps the people who live in it. The new
//! building used to start from the resources of its type, so every inhabitant
//! disappeared and a zone that upgraded its houses emptied the city.
//------------------------------------------------------------------------------
TEST(TestsZone, UpgradeKeepsTheStock)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& node = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));

    BuildingType shack("Shack");
    shack.resources.setCapacity("People", 4u);

    // The better house holds more people, and it starts empty when nobody moves in.
    BuildingType villa("Villa");
    villa.resources.setCapacity("People", 10u);

    // The shop has no room for people, only for goods.
    BuildingType shop("Shop");
    shop.resources.setCapacity("Goods", 5u);

    Building& building = city.addBuilding(shack, node);
    building.getResources().addResource("People", 3u);

    ZoneType residential("Residential");
    Zone& zone = city.addZone(residential, city.getRegion());
    RuleContext context;
    context.zone = &zone;
    context.city = &city;

    RuleCommandUpgrade toVilla(shack, villa);
    ASSERT_TRUE(toVilla.validate(context));
    toVilla.execute(context);

    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_STREQ(city.getBuildings()[0]->getTypeName().c_str(), "Villa");
    ASSERT_EQ(city.getBuildings()[0]->getResources().getAmount("People"), 3u);
    ASSERT_EQ(city.getBuildings()[0]->getResources().getCapacity("People"), 10u);

    // A crowded villa that becomes a shack loses the people who no longer fit,
    // and only those.
    city.getBuildings()[0]->getResources().addResource("People", 7u);
    RuleCommandUpgrade toShack(villa, shack);
    ASSERT_TRUE(toShack.validate(context));
    toShack.execute(context);
    ASSERT_STREQ(city.getBuildings()[0]->getTypeName().c_str(), "Shack");
    ASSERT_EQ(city.getBuildings()[0]->getResources().getAmount("People"), 4u);

    // A building whose type says nothing about people is given none, rather than
    // a stock its own rules never mention.
    RuleCommandUpgrade toShop(shack, shop);
    ASSERT_TRUE(toShop.validate(context));
    toShop.execute(context);
    ASSERT_STREQ(city.getBuildings()[0]->getTypeName().c_str(), "Shop");
    ASSERT_FALSE(city.getBuildings()[0]->getResources().hasResource("People"));
}

//------------------------------------------------------------------------------
//! \brief A zone in decline demolishes its emptiest building first.
//!
//! The command used to take the first building of the zone, which is the oldest
//! one, so a crowded house could fall while an empty one beside it stayed.
//------------------------------------------------------------------------------
TEST(TestsZone, DestroyPrefersTheEmptiestBuilding)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(90.0f, 30.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(150.0f, 30.0f, 0.0f));

    BuildingType shack("Shack");
    shack.resources.setCapacity("People", 8u);

    // The oldest one is the most crowded, so taking the first would take it.
    Building& crowded = city.addBuilding(shack, n1);
    crowded.getResources().addResource("People", 6u);
    Building& empty = city.addBuilding(shack, n2);
    Building& half = city.addBuilding(shack, n3);
    half.getResources().addResource("People", 3u);

    ZoneType residential("Residential");
    Zone& zone = city.addZone(residential, city.getRegion());
    RuleContext context;
    context.zone = &zone;
    context.city = &city;

    Vector3f const emptyPosition = empty.getPosition();

    RuleCommandDestroy destroy("Shack");
    ASSERT_TRUE(destroy.validate(context));
    destroy.execute(context);

    ASSERT_EQ(city.getBuildings().size(), 2u);
    for (auto const& remaining : city.getBuildings())
    {
        ASSERT_NE(remaining->getPosition().x, emptyPosition.x);
    }

    // The next one to fall is the half-full one, not the crowded one.
    destroy.execute(context);
    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_EQ(city.getBuildings()[0]->getResources().getAmount("People"), 6u);
}

TEST(TestsZone, UpgradeKeepsAttachment)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(90.0f, 30.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    BuildingType home("Home");
    BuildingType shop("Shop");
    city.addBuilding(home, path, segment, 0.5f);

    RuleCommandUpgrade upgrade(home, shop);

    ZoneType residential("Residential");
    Zone& zone = city.addZone(residential, city.getRegion());
    RuleContext context;
    context.zone = &zone;
    context.city = &city;

    ASSERT_TRUE(upgrade.validate(context));
    upgrade.execute(context);
    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_STREQ(city.getBuildings()[0]->getTypeName().c_str(), "Shop");
    ASSERT_NE(city.getBuildings()[0]->getSegment(), nullptr);
    ASSERT_EQ(path.getNodes().size(), 2u);
}
