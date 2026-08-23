#include "main.hpp"
#include "OpenGlassBox/Config.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/Agent.hpp"
#  include "OpenGlassBox/City.hpp"
#undef protected
#undef private

TEST(TestsAgent, Constructor)
{
    TestWorld cityWorld("Paris", 4, 4);
    City& city = cityWorld.city;
    UnitType unit_type("Home");
    unit_type.color = 0xFF00FF;
    unit_type.radius = 2u;
    unit_type.resources.addResource("oil", 5u);
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit u(unit_type, n, city);
    ASSERT_EQ(&n, u.m_node);

    // Create a new Agent
    AgentType agent_type("Agent", 5.0f, 3u, 42u);
    Resources r; r.addResource("oil", 5u);
    Agent a(43u, agent_type, u, r, "target");

    // Check initial values (member variables).
    ASSERT_EQ(a.m_id, 43u);
    ASSERT_STREQ(a.m_type.name.c_str(), "Agent");
    ASSERT_EQ(a.m_type.speed, 5.0f);
    ASSERT_EQ(a.m_type.radius, 3.0f);
    ASSERT_EQ(a.m_type.color, 42u);
    ASSERT_STREQ(a.m_searchTarget.c_str(), "target");
    ASSERT_EQ(a.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(a.m_resources.getAmount("oil"), 5u);
    ASSERT_EQ(int32_t(a.m_position.x), 1);
    ASSERT_EQ(int32_t(a.m_position.y), 2);
    ASSERT_EQ(int32_t(a.m_position.z), 3);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentWay, nullptr);
    ASSERT_EQ(a.m_lastNode, &n);
    ASSERT_EQ(a.m_lastNode, u.m_node);
    ASSERT_EQ(a.m_nextNode, nullptr);
}

TEST(TestsAgent, Move)
{
    const uint32_t GRILL_SIZE = 32u;
    TestWorld cityWorld("Paris", GRILL_SIZE, GRILL_SIZE);
    City& city = cityWorld.city;
    PathType type1("route", 0xAAAAAA);
    Path& p = city.addPath(type1);
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(3.0f, 2.0f, 3.0f));
    WayType type2("Dirt", 0xAAAAAA);
    Way& s1 = p.addWay(type2, n1, n2);

    Resources r;
    UnitType homeType("Home");
    homeType.color = 0xFF00FF;
    homeType.radius = 1u;
    homeType.resources = r;
    homeType.targets.push_back("Home");
    Unit u(homeType, n1, city);

    UnitType factoryType("Factory");
    factoryType.targets.push_back("People");
    factoryType.resources.setCapacity("People", 10u);
    city.addUnit(factoryType, n2);

    AgentType worker("Worker", 5.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent a(43u, worker, u, carried, "People");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;

    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentWay, nullptr);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, nullptr);

    ASSERT_EQ(a.update(city.m_dijkstra, dt), false);
    ASSERT_EQ(a.m_currentWay, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, &n2);

    ASSERT_EQ(a.update(city.m_dijkstra, dt), false);
    ASSERT_GT(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_GT(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentWay, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, &n2);
}

//------------------------------------------------------------------------------
//! \brief Longest distance an Agent can cover during one tick. Anything more is
//! a teleport: an offset that was written instead of being walked.
static float maxStepPerTick(Agent const& agent, float dt)
{
    return agent.speed() * dt * 1.01f + 1e-3f;
}

//------------------------------------------------------------------------------
//! \brief Drive the Agent until it delivers, checking at every tick that it did
//! not jump. Returns the number of ticks, or zero when it never delivered.
static uint32_t driveUntilDelivered(Agent& agent, City& city, float dt,
                                    uint32_t maxTicks)
{
    float const step = maxStepPerTick(agent, dt);
    for (uint32_t tick = 1u; tick <= maxTicks; ++tick)
    {
        Vector3f const before = agent.position();
        bool const delivered = agent.update(city.m_dijkstra, dt);
        float const moved = magnitude(agent.position() - before);
        EXPECT_LE(moved, step) << "teleported at tick " << tick;
        if (delivered)
            return tick;
    }
    return 0u;
}

//------------------------------------------------------------------------------
//! \brief Every building of the demo sits along a Way rather than on a Node, so
//! an Agent starts and ends its journey in the middle of a segment. It used to
//! be snapped to the end of that segment, which showed up as a teleport to the
//! previous intersection, and as an endless loop when the building refused it.
TEST(TestsAgent, LeavesAndReachesABuildingWithoutJumping)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Way& way = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    Unit& home = city.addUnit(homeType, path, way, 0.8f);
    ASSERT_FLOAT_EQ(home.position().x, 48.0f);

    UnitType workType("Work");
    workType.targets.push_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, way, 0.2f);
    ASSERT_FLOAT_EQ(work.position().x, 12.0f);

    AgentType worker("Worker", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, worker, home, carried, "Work");
    ASSERT_FLOAT_EQ(agent.position().x, 48.0f);

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);

    // It walked to the door rather than to the intersection.
    ASSERT_NEAR(agent.position().x, 12.0f, 1.0f);
    ASSERT_EQ(work.resources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief Same journey, but the destination is on another Way: the Agent has to
//! reach the intersection first, one tick at a time.
TEST(TestsAgent, DrivesToTheIntersectionBeforeTakingAnotherWay)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Way& way1 = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);
    Way& way2 = path.addWay(WayType("Dirt", 0xAAAAAA), n2, n3);

    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    Unit& home = city.addUnit(homeType, path, way1, 0.6f);

    UnitType workType("Work");
    workType.targets.push_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, way2, 0.4f);
    ASSERT_FLOAT_EQ(work.position().x, 84.0f);

    AgentType worker("Worker", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, worker, home, carried, "Work");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);
    ASSERT_NEAR(agent.position().x, 84.0f, 1.0f);
    ASSERT_EQ(work.resources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief A building standing near one end of a street sends its Agents out by
//! the end the destination is really behind. Leaving by the nearest one meant
//! an Agent bound for a shop to the east was first seen driving west, only to
//! turn back at the corner.
TEST(TestsAgent, LeavesTheWayByTheEndTheDestinationIsBehind)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Way& way1 = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);
    path.addWay(WayType("Dirt", 0xAAAAAA), n2, n3);

    // The factory stands at a fifth of the first street, so n1 is its near end
    // and the shop is on the other side of n2.
    UnitType workType("Work");
    workType.targets.push_back("Work");
    Unit& work = city.addUnit(workType, path, way1, 0.2f);
    ASSERT_FLOAT_EQ(work.position().x, 12.0f);

    UnitType shopType("Shop");
    shopType.targets.push_back("Shop");
    shopType.resources.setCapacity("Goods", 4u);
    Unit& shop = city.addUnit(shopType, n3);

    AgentType truck("Truck", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("Goods", 1u);
    Agent agent(1u, truck, work, carried, "Shop");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    float const departure = agent.position().x;

    ASSERT_FALSE(agent.update(city.m_dijkstra, dt));
    ASSERT_EQ(agent.m_nextNode, &n2) << "drove away from the shop";

    float const step = maxStepPerTick(agent, dt);
    bool delivered = false;
    for (uint32_t tick = 0u; (tick < 4000u) && !delivered; ++tick)
    {
        Vector3f const before = agent.position();
        delivered = agent.update(city.m_dijkstra, dt);
        ASSERT_LE(magnitude(agent.position() - before), step);
        ASSERT_GE(agent.position().x, departure - 0.5f) << "turned back";
    }

    ASSERT_TRUE(delivered);
    ASSERT_EQ(shop.resources().getAmount("Goods"), 1u);
}

//------------------------------------------------------------------------------
//! \brief Standing along a street, an Agent has for last Node an end of the
//! segment it has not reached yet. Reading the buildings of that Node let it
//! deliver from the middle of the street, a whole block away from the door.
TEST(TestsAgent, DoesNotDeliverFromTheMiddleOfTheStreet)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Way& way = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.push_back("Work");
    Unit& work = city.addUnit(workType, path, way, 0.2f);

    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    homeType.resources.setCapacity("People", 4u);
    Unit& home = city.addUnit(homeType, n1);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    ASSERT_FALSE(agent.update(city.m_dijkstra, dt)) << "delivered from afar";
    ASSERT_EQ(home.resources().getAmount("People"), 0u);

    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);
    ASSERT_NEAR(agent.position().x, 0.0f, 1.0f);
    ASSERT_EQ(home.resources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief Two groups of Agents heading for the same house: the first one fills
//! it, and the second used to keep knocking at its door for ever. It has to
//! deliver to the building that still has room, even the farthest one.
TEST(TestsAgent, DeliversToTheBuildingThatStillHasRoom)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Way& way1 = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);
    Way& way2 = path.addWay(WayType("Dirt", 0xAAAAAA), n2, n3);

    UnitType workType("Work");
    workType.targets.push_back("Work");
    Unit& work = city.addUnit(workType, path, way2, 0.9f);

    // The nearest home is full, the far one is not.
    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    homeType.resources.setCapacity("People", 1u);
    homeType.resources.addResource("People", 1u);
    Unit& full = city.addUnit(homeType, path, way2, 0.4f);
    ASSERT_EQ(full.resources().getAmount("People"), 1u);

    UnitType freeType("Home");
    freeType.targets.push_back("Home");
    freeType.resources.setCapacity("People", 4u);
    Unit& free = city.addUnit(freeType, path, way1, 0.2f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);

    ASSERT_EQ(free.resources().getAmount("People"), 1u);
    ASSERT_EQ(full.resources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief The house fills up while the Agent is on its way. It must not shuttle
//! between the intersection and the door: either it finds another one, or it
//! gives its load back to the building that sent it out and leaves.
TEST(TestsAgent, DoesNotLoopWhenTheDestinationFillsUpOnTheWay)
{
    SimulationConfig config;
    config.agentGiveUpTicks = 200u;
    TestWorld cityWorld("Paris", 32u, 32u, Vector3f(0.0f, 0.0f, 0.0f), config);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Way& way = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.push_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, way, 0.8f);

    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    homeType.resources.setCapacity("People", 1u);
    Unit& home = city.addUnit(homeType, path, way, 0.2f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");
    agent.setConfig(city.config());

    // Somebody else moves in before the Agent arrives.
    home.resources().addResource("People", 1u);

    float const dt = 1.0f / city.config().ticksPerSecond;
    float const step = maxStepPerTick(agent, dt);
    bool removed = false;
    for (uint32_t tick = 0u; (tick < 2000u) && !removed; ++tick)
    {
        Vector3f const before = agent.position();
        removed = agent.update(city.m_dijkstra, dt);
        ASSERT_LE(magnitude(agent.position() - before), step);
    }

    ASSERT_TRUE(removed);
    // The load went back where it came from rather than vanish.
    ASSERT_EQ(work.resources().getAmount("People"), 1u);
    ASSERT_EQ(home.resources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief A rule that abandons a house destroys it while Agents are driving to
//! it. Their itinerary must not keep pointing at the freed building.
TEST(TestsAgent, ForgetsADestroyedDestination)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Way& way = path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.push_back("Work");
    Unit& work = city.addUnit(workType, path, way, 0.9f);

    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    homeType.resources.setCapacity("People", 4u);
    Unit& home = city.addUnit(homeType, path, way, 0.1f);

    static AgentType const people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent& agent = city.addAgent(people, work, carried, "Home");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    for (uint32_t tick = 0u; tick < 20u; ++tick)
        city.update(dt);
    ASSERT_EQ(agent.route().destination, &home);

    city.removeUnit(home);
    ASSERT_EQ(agent.route().destination, nullptr);
    ASSERT_EQ(agent.owner(), &work);
    ASSERT_NO_THROW(city.update(dt));
}

TEST(TestsAgent, ZeroLengthWayDoesNotCrash)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(PathType("Road"));
    Node& n1 = path.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = path.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    path.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    UnitType homeType("Home");
    homeType.targets.push_back("Home");
    Unit u(homeType, n1, city);

    UnitType factoryType("Factory");
    factoryType.targets.push_back("People");
    factoryType.resources.setCapacity("People", 10u);
    city.addUnit(factoryType, n2);

    Resources carried;
    carried.addResource("People", 1u);
    Agent a(1u, AgentType("Worker", 5.0f, 3u, 42u), u, carried, "People");

    float const dt = 1.0f / config::DEFAULT_TICKS_PER_SECOND;
    ASSERT_NO_THROW(a.update(city.m_dijkstra, dt));
    ASSERT_NO_THROW(a.update(city.m_dijkstra, dt));
}
