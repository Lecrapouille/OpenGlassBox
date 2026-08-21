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
