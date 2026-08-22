#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/Area.hpp"
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/RuleCommand.hpp"
#undef protected
#undef private

TEST(TestsArea, SpawnOnNearestWay)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(90.0f, 30.0f, 0.0f));
    path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType home("Home");
    RuleAreaType growType("Grow");
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestWay);
    growType.commands.push_back(&spawn);
    RuleArea grow(growType);

    AreaType residential("Residential");
    residential.rules.push_back(&grow);
    Area& area = city.addArea(residential, city.region());

    ASSERT_EQ(city.units().size(), 0u);
    area.executeRules();
    ASSERT_EQ(city.units().size(), 1u);
    ASSERT_STREQ(city.units()[0]->type().c_str(), "Home");
    ASSERT_NE(city.units()[0]->way(), nullptr);
    ASSERT_EQ(path.nodes().size(), 2u);
}

//------------------------------------------------------------------------------
//! \brief A building belongs beside the road that serves it: the cell the Area
//! grows it on is one the road runs through or fronts, whatever the shape of the
//! footprint.
//------------------------------------------------------------------------------
TEST(TestsArea, SpawnedUnitsStandAlongTheRoad)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    float const side = city.gridCellSize();

    Path& path = city.addPath(PathType("Road"));
    // A road along the third row of cells, crossing the whole city.
    Node& n1 = path.addNode(Vector3f(0.5f * side, 2.5f * side, 0.0f));
    Node& n2 = path.addNode(Vector3f(7.5f * side, 2.5f * side, 0.0f));
    path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType home("Home");
    RuleAreaType growType("Grow");
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestWay);
    growType.commands.push_back(&spawn);
    RuleArea grow(growType);

    AreaType residential("Residential");
    residential.rules.push_back(&grow);
    Area& area = city.addArea(residential, city.region());

    // More buildings than the row of cells the road runs through, so that the
    // ones that do not fit have to be refused rather than piled up.
    for (uint32_t i = 0u; i < 12u; ++i)
        area.executeRules();

    ASSERT_FALSE(city.units().empty());
    for (auto const& unit: city.units())
    {
        // Two cells at most from the road, and no two buildings on one cell.
        ASSERT_LE(std::abs(unit->mapV() - 2), 1);
        for (auto const& other: city.units())
        {
            if (other.get() == unit.get())
                continue;
            ASSERT_FALSE((other->mapU() == unit->mapU()) &&
                         (other->mapV() == unit->mapV()));
        }
    }
}

//------------------------------------------------------------------------------
//! \brief Without a road there is no connection to make, so the Area grows
//! nothing at all instead of scattering houses in a field.
//------------------------------------------------------------------------------
TEST(TestsArea, NoRoadNoBuilding)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    city.addPath(PathType("Road"));

    UnitType home("Home");
    RuleAreaType growType("Grow");
    RuleCommandSpawn spawn(home, RuleCommandSpawn::Placement::NearestWay);
    growType.commands.push_back(&spawn);
    RuleArea grow(growType);

    AreaType residential("Residential");
    residential.rules.push_back(&grow);
    Area& area = city.addArea(residential, city.region());

    area.executeRules();
    area.executeRules();
    ASSERT_TRUE(city.units().empty());
}

TEST(TestsArea, CountAndDestroy)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    city.addUnit(UnitType("Home"), n1);

    AreaType residential("Residential");
    Area& area = city.addArea(residential, city.region());
    ASSERT_EQ(area.countUnits("Home"), 1u);

    RuleCommandDestroy destroy("Home");
    RuleContext context;
    context.area = &area;
    context.city = &city;
    ASSERT_TRUE(destroy.validate(context));
    destroy.execute(context);
    ASSERT_EQ(city.units().size(), 0u);
}

TEST(TestsArea, UpgradeKeepsAttachment)
{
    TestWorld world("Paris", 8u, 8u);
    City& city = world.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(30.0f, 30.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(90.0f, 30.0f, 0.0f));
    Way& way = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType home("Home");
    UnitType shop("Shop");
    city.addUnit(home, path, way, 0.5f);

    RuleCommandUpgrade upgrade(home, shop);

    AreaType residential("Residential");
    Area& area = city.addArea(residential, city.region());
    RuleContext context;
    context.area = &area;
    context.city = &city;

    ASSERT_TRUE(upgrade.validate(context));
    upgrade.execute(context);
    ASSERT_EQ(city.units().size(), 1u);
    ASSERT_STREQ(city.units()[0]->type().c_str(), "Shop");
    ASSERT_NE(city.units()[0]->way(), nullptr);
    ASSERT_EQ(path.nodes().size(), 2u);
}
