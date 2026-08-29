#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Types.hpp"
#include "OpenGlassBox/World.hpp"
#include "main.hpp"

using namespace ogb;

TEST(TestsWorld, AddRoadSplitsAtBorder)
{
    SimulationClock clock;
    World world({}, clock);
    City& west = world.addCity("West", Vector3f(0.0f, 0.0f, 0.0f), 4u, 4u);
    City& east = world.addCity(
        "East", Vector3f(4.0f * world.getCellSize(), 0.0f, 0.0f), 4u, 4u);

    PathType road("Road");
    SegmentType dirt("Dirt");
    west.addPath(road);

    Vector3f const from(world.getCellSize() * 1.0f, world.getCellSize() * 2.0f, 0.0f);
    Vector3f const to(world.getCellSize() * 6.0f, world.getCellSize() * 2.0f, 0.0f);
    ASSERT_TRUE(world.addRoad(west, "Road", dirt, from, to));

    ASSERT_FALSE(west.getPaths().empty());
    ASSERT_FALSE(east.getPaths().empty());
    ASSERT_GE(west.getPath("Road").getSegments().size(), 1u);
    ASSERT_GE(east.getPath("Road").getSegments().size(), 1u);
}

TEST(TestsWorld, ListenerCanRefuseCrossing)
{
    class Refuse: public World::Listener
    {
    public:

        bool allowSegmentAcross(City&, City&, World::Listener::SegmentProposal const&) override
        {
            return false;
        }
    };

    SimulationClock clock;
    World world({}, clock);
    Refuse refuse;
    world.setListener(refuse);

    City& west = world.addCity("West", Vector3f(0.0f, 0.0f, 0.0f), 4u, 4u);
    world.addCity(
        "East", Vector3f(4.0f * world.getCellSize(), 0.0f, 0.0f), 4u, 4u);

    PathType road("Road");
    SegmentType dirt("Dirt");
    west.addPath(road);

    Vector3f const from(world.getCellSize() * 1.0f, world.getCellSize() * 2.0f, 0.0f);
    Vector3f const to(world.getCellSize() * 6.0f, world.getCellSize() * 2.0f, 0.0f);
    ASSERT_FALSE(world.addRoad(west, "Road", dirt, from, to));
}

TEST(TestsWorld, IRouterIsOwnedByCity)
{
    SimulationClock clock;
    World world({}, clock);
    City& city = world.addCity("Solo", Vector3f(0.0f, 0.0f, 0.0f), 4u, 4u);
    installDijkstraRouter(city, world.getConfig());
    IRouter& router = city.getRouter();
    (void)router;
}
