#include "main.hpp"

#define protected public
#define private public
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Building.hpp"
#include "OpenGlassBox/DijkstraRouter.hpp"
#include "TestWorld.hpp"
#undef protected
#undef private

namespace
{
BuildingType makeFactoryType()
{
    BuildingType type("Factory");
    type.targets.emplace_back("People");
    type.resources.setCapacity("People", 10u);
    return type;
}
} // namespace

TEST(TestsDijkstra, DirectPath)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& home = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& mid = path.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    Node& factory = path.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    SegmentType segmentType("Dirt", 0xAAAAAA);
    path.addSegment(segmentType, home, mid);
    path.addSegment(segmentType, mid, factory);

    BuildingType factoryType = makeFactoryType();
    city.addBuilding(factoryType, factory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    Name target = "People";
    Node* next = router.findNextNode(home, target, carried);

    ASSERT_EQ(next, &mid);
}

TEST(TestsDijkstra, ShortestBranch)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& start = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& longRoute = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& shortRoute = path.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    Node& factory = path.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    SegmentType segmentType("Dirt", 0xAAAAAA);
    path.addSegment(segmentType, start, longRoute);
    path.addSegment(segmentType, longRoute, factory);
    path.addSegment(segmentType, start, shortRoute);
    path.addSegment(segmentType, shortRoute, factory);

    BuildingType factoryType = makeFactoryType();
    city.addBuilding(factoryType, factory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    Name target = "People";
    Node* next = router.findNextNode(start, target, carried);

    ASSERT_EQ(next, &shortRoute);
}

TEST(TestsDijkstra, AlreadyAtDestination)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& factory = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    BuildingType factoryType = makeFactoryType();
    city.addBuilding(factoryType, factory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    Name target = "People";
    Node* next = router.findNextNode(factory, target, carried);

    ASSERT_EQ(next, &factory);
}

TEST(TestsDijkstra, RandomFallbackWhenNoDestination)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& start = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& other = path.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), start, other);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    router.setRandomSeed(42u);
    Name target = "People";
    Node* next = router.findNextNode(start, target, carried);

    ASSERT_EQ(next, &other);
}

TEST(TestsDijkstra, DisconnectedGraph)
{
    Node orphan(0u, Vector3f(0.0f, 0.0f, 0.0f));

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    Name target = "People";
    Node* next = router.findNextNode(orphan, target, carried);

    ASSERT_EQ(next, nullptr);
}

TEST(TestsDijkstra, PathScopedRouting)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& road = city.addPath(keep<PathType>("Road"));
    Path& rail = city.addPath(keep<PathType>("Rail"));

    Node& start = road.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& roadMid = road.addNode(Vector3f(1.0f, 0.0f, 0.0f));
    Node& roadFactory = road.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    SegmentType segmentType("Dirt", 0xAAAAAA);
    road.addSegment(segmentType, start, roadMid);
    road.addSegment(segmentType, roadMid, roadFactory);

    Node& railFactory = rail.addNode(Vector3f(0.0f, 1.0f, 0.0f));
    BuildingType railFactoryType = makeFactoryType();
    BuildingType roadFactoryType = makeFactoryType();
    city.addBuilding(railFactoryType, railFactory);
    city.addBuilding(roadFactoryType, roadFactory);

    Resources carried;
    carried.addResource("People", 1u);

    Dijkstra router;
    Name target = "People";
    Node* next = router.findNextNode(start, target, carried);

    ASSERT_EQ(next, &roadMid);
    ASSERT_NE(next, &railFactory);
}
