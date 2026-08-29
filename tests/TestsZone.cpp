#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/Zone.hpp"
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/RuleCommand.hpp"
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
