#include "main.hpp"
#include "OpenGlassBox/World.hpp"
#include "OpenGlassBox/Types.hpp"

using namespace ogb;

TEST(TestsWorld, AddRoadSplitsAtBorder)
{
    World world;
    City& west = world.addCity("West", Vector3f(0.0f, 0.0f, 0.0f), 4u, 4u);
    City& east = world.addCity("East", Vector3f(4.0f * world.cellSize(), 0.0f, 0.0f),
                               4u, 4u);

    PathType road("Road");
    WayType dirt("Dirt");
    west.addPath(road);

    Vector3f const from(world.cellSize() * 1.0f, world.cellSize() * 2.0f, 0.0f);
    Vector3f const to(world.cellSize() * 6.0f, world.cellSize() * 2.0f, 0.0f);
    ASSERT_TRUE(world.addRoad(west, "Road", dirt, from, to));

    ASSERT_FALSE(west.paths().empty());
    ASSERT_FALSE(east.paths().empty());
    ASSERT_GE(west.getPath("Road").ways().size(), 1u);
    ASSERT_GE(east.getPath("Road").ways().size(), 1u);
}

TEST(TestsWorld, ListenerCanRefuseCrossing)
{
    class Refuse: public World::Listener
    {
    public:
        bool allowWayAcross(City&, City&, WayProposal const&) override
        {
            return false;
        }
    };

    World world;
    Refuse refuse;
    world.setListener(refuse);

    City& west = world.addCity("West", Vector3f(0.0f, 0.0f, 0.0f), 4u, 4u);
    world.addCity("East", Vector3f(4.0f * world.cellSize(), 0.0f, 0.0f), 4u, 4u);

    PathType road("Road");
    WayType dirt("Dirt");
    west.addPath(road);

    Vector3f const from(world.cellSize() * 1.0f, world.cellSize() * 2.0f, 0.0f);
    Vector3f const to(world.cellSize() * 6.0f, world.cellSize() * 2.0f, 0.0f);
    ASSERT_FALSE(world.addRoad(west, "Road", dirt, from, to));
}

TEST(TestsWorld, IRouterIsOwnedByCity)
{
    World world;
    City& city = world.addCity("Solo", 4u, 4u);
    IRouter& router = city.router();
    (void)router;
}
