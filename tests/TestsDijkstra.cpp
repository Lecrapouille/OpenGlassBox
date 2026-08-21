#include "main.hpp"

#define protected public
#define private public
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/Dijkstra.hpp"
#  include "OpenGlassBox/Unit.hpp"
#undef protected
#undef private

namespace
{
    UnitType makeFactoryType()
    {
        UnitType type("Factory");
        type.targets.push_back("People");
        type.resources.setCapacity("People", 10u);
        return type;
    }
}

TEST(TestsDijkstra, DirectPath)
{
    City city("Paris", 32u, 32u);
    Path& path = city.addPath(PathType("Road"));
    Node& home = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& mid = path.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    Node& factory = path.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    WayType wayType("Dirt", 0xAAAAAA);
    path.addWay(wayType, home, mid);
    path.addWay(wayType, mid, factory);

    UnitType factoryType = makeFactoryType();
    city.addUnit(factoryType, factory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    std::string target = "People";
    Node* next = router.findNextPoint(home, target, carried);

    ASSERT_EQ(next, &mid);
}

TEST(TestsDijkstra, ShortestBranch)
{
    City city("Paris", 32u, 32u);
    Path& path = city.addPath(PathType("Road"));
    Node& start = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& longRoute = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& shortRoute = path.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    Node& factory = path.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    WayType wayType("Dirt", 0xAAAAAA);
    path.addWay(wayType, start, longRoute);
    path.addWay(wayType, longRoute, factory);
    path.addWay(wayType, start, shortRoute);
    path.addWay(wayType, shortRoute, factory);

    UnitType factoryType = makeFactoryType();
    city.addUnit(factoryType, factory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    std::string target = "People";
    Node* next = router.findNextPoint(start, target, carried);

    ASSERT_EQ(next, &shortRoute);
}

TEST(TestsDijkstra, AlreadyAtDestination)
{
    City city("Paris", 32u, 32u);
    Path& path = city.addPath(PathType("Road"));
    Node& factory = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    UnitType factoryType = makeFactoryType();
    city.addUnit(factoryType, factory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    std::string target = "People";
    Node* next = router.findNextPoint(factory, target, carried);

    ASSERT_EQ(next, &factory);
}

TEST(TestsDijkstra, RandomFallbackWhenNoDestination)
{
    City city("Paris", 32u, 32u);
    Path& path = city.addPath(PathType("Road"));
    Node& start = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& other = path.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    path.addWay(WayType("Dirt", 0xAAAAAA), start, other);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    router.setRandomSeed(42u);
    std::string target = "People";
    Node* next = router.findNextPoint(start, target, carried);

    ASSERT_EQ(next, &other);
}

TEST(TestsDijkstra, DisconnectedGraph)
{
    Node orphan(0u, Vector3f(0.0f, 0.0f, 0.0f));

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    std::string target = "People";
    Node* next = router.findNextPoint(orphan, target, carried);

    ASSERT_EQ(next, nullptr);
}

TEST(TestsDijkstra, PathScopedRouting)
{
    City city("Paris", 32u, 32u);
    Path& road = city.addPath(PathType("Road"));
    Path& rail = city.addPath(PathType("Rail"));

    Node& start = road.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& roadMid = road.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    Node& roadFactory = road.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    WayType wayType("Dirt", 0xAAAAAA);
    road.addWay(wayType, start, roadMid);
    road.addWay(wayType, roadMid, roadFactory);

    Node& railFactory = rail.addNode(Vector3f(0.0f, 1.0f, 0.0f));
    UnitType railFactoryType = makeFactoryType();
    UnitType roadFactoryType = makeFactoryType();
    city.addUnit(railFactoryType, railFactory);
    city.addUnit(roadFactoryType, roadFactory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    std::string target = "People";
    Node* next = router.findNextPoint(start, target, carried);

    ASSERT_EQ(next, &roadMid);
    ASSERT_NE(next, &railFactory);
}
