#include "OpenGlassBox/Config.hpp"
#include "main.hpp"

#define protected public
#define private public
#include "OpenGlassBox/Agent.hpp"
#include "OpenGlassBox/City.hpp"
#include "TestWorld.hpp"
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
    Resources r;
    r.addResource("oil", 5u);
    Agent a(43u, agent_type, u, r, "target");

    // Check initial values (member variables).
    ASSERT_EQ(a.m_id, 43u);
    ASSERT_STREQ(a.m_type.name.c_str(), "Agent");
    ASSERT_EQ(a.m_type.speed, 5.0f);
    ASSERT_EQ(a.m_type.radius, 3u);
    ASSERT_EQ(a.m_type.color, 42u);
    ASSERT_STREQ(a.m_searchTarget.c_str(), "target");
    ASSERT_EQ(a.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(a.m_resources.getAmount("oil"), 5u);
    ASSERT_EQ(int32_t(a.m_position.x), 1);
    ASSERT_EQ(int32_t(a.m_position.y), 2);
    ASSERT_EQ(int32_t(a.m_position.z), 3);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, nullptr);
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
    SegmentType type2("Dirt", 0xAAAAAA);
    Segment& s1 = p.addSegment(type2, n1, n2);

    Resources r;
    UnitType homeType("Home");
    homeType.color = 0xFF00FF;
    homeType.radius = 1u;
    homeType.resources = r;
    homeType.targets.push_back("Home");
    Unit u(homeType, n1, city);

    UnitType factoryType("Factory");
    factoryType.targets.emplace_back("People");
    factoryType.resources.setCapacity("People", 10u);
    city.addUnit(factoryType, n2);

    AgentType worker("Worker", 5.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent a(43u, worker, u, carried, "People");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;

    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, nullptr);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, nullptr);

    ASSERT_EQ(a.update(city.getRouter(), city.getConfig().routing, dt), false);
    ASSERT_EQ(a.m_currentSegment, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, &n2);

    ASSERT_EQ(a.update(city.getRouter(), city.getConfig().routing, dt), false);
    ASSERT_GT(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_GT(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, &n2);
}

//------------------------------------------------------------------------------
//! \brief Longest distance an Agent can cover during one tick. Anything more is
//! a teleport: an offset that was written instead of being walked.
static float maxStepPerTick(Agent const& agent, float dt)
{
    return agent.getSpeed() * dt * 1.01f + 1e-3f;
}

//------------------------------------------------------------------------------
//! \brief Drive the Agent until it delivers, checking at every tick that it did
//! not jump. Returns the number of ticks, or zero when it never delivered.
static uint32_t
driveUntilDelivered(Agent& agent, City& city, float dt, uint32_t maxTicks)
{
    float const step = maxStepPerTick(agent, dt);
    for (uint32_t tick = 1u; tick <= maxTicks; ++tick)
    {
        Vector3f const before = agent.getPosition();
        bool const delivered = agent.update(city.getRouter(), city.getConfig().routing, dt);
        float const moved = length(agent.getPosition() - before);
        EXPECT_LE(moved, step) << "teleported at tick " << tick;
        if (delivered)
            return tick;
    }
    return 0u;
}

//------------------------------------------------------------------------------
//! \brief Every building of the demo sits along a Segment rather than on a Node, so
//! an Agent starts and ends its journey in the middle of a segment. It used to
//! be snapped to the end of that segment, which showed up as a teleport to the
//! previous intersection, and as an endless loop when the building refused it.
TEST(TestsAgent, LeavesAndReachesABuildingWithoutJumping)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    Unit& home = city.addUnit(homeType, path, segment, 0.8f);
    ASSERT_FLOAT_EQ(home.getPosition().x, 48.0f);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, segment, 0.2f);
    ASSERT_FLOAT_EQ(work.getPosition().x, 12.0f);

    AgentType worker("Worker", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, worker, home, carried, "Work");
    ASSERT_FLOAT_EQ(agent.getPosition().x, 48.0f);

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);

    // It walked to the door rather than to the intersection.
    ASSERT_NEAR(static_cast<double>(agent.getPosition().x), 12.0, 1.0);
    ASSERT_EQ(work.getResources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief Same journey, but the destination is on another Segment: the Agent has to
//! reach the intersection first, one tick at a time.
TEST(TestsAgent, DrivesToTheIntersectionBeforeTakingAnotherSegment)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Segment& segment1 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    Segment& segment2 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n2, n3);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    Unit& home = city.addUnit(homeType, path, segment1, 0.6f);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, segment2, 0.4f);
    ASSERT_FLOAT_EQ(work.getPosition().x, 84.0f);

    AgentType worker("Worker", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, worker, home, carried, "Work");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);
    ASSERT_NEAR(static_cast<double>(agent.getPosition().x), 84.0, 1.0);
    ASSERT_EQ(work.getResources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief A building standing near one end of a street sends its Agents out by
//! the end the destination is really behind. Leaving by the nearest one meant
//! an Agent bound for a shop to the east was first seen driving west, only to
//! turn back at the corner.
TEST(TestsAgent, LeavesTheSegmentByTheEndTheDestinationIsBehind)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Segment& segment1 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n2, n3);

    // The factory stands at a fifth of the first street, so n1 is its near end
    // and the shop is on the other side of n2.
    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment1, 0.2f);
    ASSERT_FLOAT_EQ(work.getPosition().x, 12.0f);

    UnitType shopType("Shop");
    shopType.targets.emplace_back("Shop");
    shopType.resources.setCapacity("Goods", 4u);
    Unit& shop = city.addUnit(shopType, n3);

    AgentType truck("Truck", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("Goods", 1u);
    Agent agent(1u, truck, work, carried, "Shop");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    float const departure = agent.getPosition().x;

    ASSERT_FALSE(agent.update(city.getRouter(), city.getConfig().routing, dt));
    ASSERT_EQ(agent.m_nextNode, &n2) << "drove away from the shop";

    float const step = maxStepPerTick(agent, dt);
    bool delivered = false;
    for (uint32_t tick = 0u; (tick < 4000u) && !delivered; ++tick)
    {
        Vector3f const before = agent.getPosition();
        delivered = agent.update(city.getRouter(), city.getConfig().routing, dt);
        ASSERT_LE(length(agent.getPosition() - before), step);
        ASSERT_GE(agent.getPosition().x, departure - 0.5f) << "turned back";
    }

    ASSERT_TRUE(delivered);
    ASSERT_EQ(shop.getResources().getAmount("Goods"), 1u);
}

//------------------------------------------------------------------------------
//! \brief Standing along a street, an Agent has for last Node an end of the
//! segment it has not reached yet. Reading the buildings of that Node let it
//! deliver from the middle of the street, a whole block away from the door.
TEST(TestsAgent, DoesNotDeliverFromTheMiddleOfTheStreet)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment, 0.2f);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 4u);
    Unit& home = city.addUnit(homeType, n1);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_FALSE(agent.update(city.getRouter(), city.getConfig().routing, dt)) << "delivered from afar";
    ASSERT_EQ(home.getResources().getAmount("People"), 0u);

    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);
    ASSERT_NEAR(static_cast<double>(agent.getPosition().x), 0.0, 1.0);
    ASSERT_EQ(home.getResources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief Two groups of Agents heading for the same house: the first one fills
//! it, and the second used to keep knocking at its door for ever. It has to
//! deliver to the building that still has room, even the farthest one.
TEST(TestsAgent, DeliversToTheBuildingThatStillHasRoom)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Segment& segment1 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    Segment& segment2 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n2, n3);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment2, 0.9f);

    // The nearest home is full, the far one is not.
    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 1u);
    homeType.resources.addResource("People", 1u);
    Unit& full = city.addUnit(homeType, path, segment2, 0.4f);
    ASSERT_EQ(full.getResources().getAmount("People"), 1u);

    UnitType freeType("Home");
    freeType.targets.emplace_back("Home");
    freeType.resources.setCapacity("People", 4u);
    Unit& free = city.addUnit(freeType, path, segment1, 0.2f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);

    ASSERT_EQ(free.getResources().getAmount("People"), 1u);
    ASSERT_EQ(full.getResources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief The house fills up while the Agent is on its way. It must not shuttle
//! between the intersection and the door: either it finds another one, or it
//! gives its load back to the building that sent it out and leaves.
TEST(TestsAgent, DoesNotLoopWhenTheDestinationFillsUpOnTheSegment)
{
    Config config;
    config.routing.agentGiveUpTicks = 200u;
    TestWorld cityWorld("Paris", 32u, 32u, Vector3f(0.0f, 0.0f, 0.0f), config);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, segment, 0.8f);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 1u);
    Unit& home = city.addUnit(homeType, path, segment, 0.2f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");
    // Somebody else moves in before the Agent arrives.
    home.getResources().addResource("People", 1u);

    float const dt = 1.0f / city.getConfig().time.ticksPerSecond;
    float const step = maxStepPerTick(agent, dt);
    bool removed = false;
    for (uint32_t tick = 0u; (tick < 2000u) && !removed; ++tick)
    {
        Vector3f const before = agent.getPosition();
        removed = agent.update(city.getRouter(), city.getConfig().routing, dt);
        ASSERT_LE(length(agent.getPosition() - before), step);
    }

    ASSERT_TRUE(removed);
    // The load went back where it came from rather than vanish.
    ASSERT_EQ(work.getResources().getAmount("People"), 1u);
    ASSERT_EQ(home.getResources().getAmount("People"), 1u);
}

//------------------------------------------------------------------------------
//! \brief A rule that abandons a house destroys it while Agents are driving to
//! it. Their itinerary must not keep pointing at the freed building.
TEST(TestsAgent, ForgetsADestroyedDestination)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment, 0.9f);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 4u);
    Unit& home = city.addUnit(homeType, path, segment, 0.1f);

    static AgentType const people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent const& agent = city.addAgent(people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    for (uint32_t tick = 0u; tick < 20u; ++tick)
        city.update(dt);
    ASSERT_EQ(agent.getRoute().getDestination(), &home);

    city.removeUnit(home);
    ASSERT_EQ(agent.getRoute().getDestination(), nullptr);
    ASSERT_EQ(agent.getOwner(), &work);
    ASSERT_NO_THROW(city.update(dt));
}

//------------------------------------------------------------------------------
//! \brief An Agent that has been routed somewhere holds a place there, and that
//! place comes back whichever way the trip ends. A count that never comes back
//! down would make the building invisible to everyone for the rest of the game.
TEST(TestsAgent, ClaimsAndGivesBackAPlaceAtItsDestination)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment, 0.9f);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 4u);
    Unit& home = city.addUnit(homeType, path, segment, 0.1f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    float const dt = 1.0f / defaults::TICKS_PER_SECOND;

    {
        Agent agent(1u, people, work, carried, "Home");
        ASSERT_EQ(home.getReservedCount(), 0u) << "claimed before being routed";

        ASSERT_FALSE(agent.update(city.getRouter(), city.getConfig().routing, dt));
        ASSERT_EQ(agent.getRoute().getDestination(), &home);
        ASSERT_EQ(home.getReservedCount(), 1u) << "routed without claiming";
        ASSERT_EQ(agent.m_reservation, &home);

        // Losing the itinerary hands the place straight back, rather than
        // holding it for the two game hours it takes to give up.
        agent.invalidateRoute();
        ASSERT_EQ(home.getReservedCount(), 0u) << "kept the place while wandering";

        // Routed again, and this time driven to the door.
        ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);
        ASSERT_EQ(home.getResources().getAmount("People"), 1u);
        ASSERT_EQ(home.getReservedCount(), 0u) << "kept the place after delivering";
    }

    // And once more, destroyed halfway through: the destructor is the last
    // line of defence, and the one that covers an Agent the City takes away.
    {
        Agent agent(2u, people, work, carried, "Home");
        ASSERT_FALSE(agent.update(city.getRouter(), city.getConfig().routing, dt));
        ASSERT_EQ(home.getReservedCount(), 1u);
    }
    ASSERT_EQ(home.getReservedCount(), 0u) << "the place died with the Agent";
}

//------------------------------------------------------------------------------
//! \brief An Agent that gives up because nothing will have it must not leave a
//! claim behind either.
TEST(TestsAgent, GivingUpGivesThePlaceBack)
{
    Config config;
    config.routing.agentGiveUpTicks = 100u;
    TestWorld cityWorld("Paris", 32u, 32u, Vector3f(0.0f, 0.0f, 0.0f), config);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    workType.resources.setCapacity("People", 4u);
    Unit& work = city.addUnit(workType, path, segment, 0.8f);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 1u);
    Unit& home = city.addUnit(homeType, path, segment, 0.2f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    // Somebody moves in while the Agent is driving, so it arrives to a full
    // house and eventually hands its load back.
    float const dt = 1.0f / city.getConfig().time.ticksPerSecond;
    ASSERT_FALSE(agent.update(city.getRouter(), city.getConfig().routing, dt));
    ASSERT_EQ(home.getReservedCount(), 1u);
    home.getResources().addResource("People", 1u);

    bool removed = false;
    for (uint32_t tick = 0u; (tick < 2000u) && !removed; ++tick)
        removed = agent.update(city.getRouter(), city.getConfig().routing, dt);

    ASSERT_TRUE(removed);
    ASSERT_EQ(home.getReservedCount(), 0u) << "gave up but kept the place";
}

//------------------------------------------------------------------------------
//! \brief Two Agents, one free place. The second must be sent to the other
//! house rather than follow the first one to a door that is already spoken for.
TEST(TestsAgent, TwoAgentsForOnePlaceGoToDifferentBuildings)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Segment& segment1 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    Segment& segment2 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n2, n3);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment1, 0.1f);

    // The near house has room for one, the far one for plenty.
    UnitType nearType("Home");
    nearType.targets.emplace_back("Home");
    nearType.resources.setCapacity("People", 1u);
    Unit& nearHome = city.addUnit(nearType, path, segment1, 0.9f);

    UnitType farType("Home");
    farType.targets.emplace_back("Home");
    farType.resources.setCapacity("People", 4u);
    Unit& farHome = city.addUnit(farType, path, segment2, 0.9f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent first(1u, people, work, carried, "Home");
    Agent second(2u, people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_FALSE(first.update(city.getRouter(), city.getConfig().routing, dt));
    ASSERT_FALSE(second.update(city.getRouter(), city.getConfig().routing, dt));

    ASSERT_EQ(first.getRoute().getDestination(), &nearHome)
        << "the first one had the nearest house to itself";
    ASSERT_EQ(second.getRoute().getDestination(), &farHome)
        << "both were sent to the same single place";
    ASSERT_EQ(nearHome.getReservedCount(), 1u);
    ASSERT_EQ(farHome.getReservedCount(), 1u);
}

//------------------------------------------------------------------------------
//! \brief An Agent already on its cheapest itinerary must not be told it would
//! gain by rerouting.
//!
//! This is the invariant behind SPTT <= TSTT in the Traffic panel. Measuring
//! the alternative without lifting the Agent's own claim finds its destination
//! full, answers with the far house, and reports a whole city that would be
//! better off going somewhere else than where it is already going.
TEST(TestsAgent, RerouteCostDoesNotSeeItsOwnClaim)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(120.0f, 0.0f, 0.0f));
    Segment& segment1 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    Segment& segment2 = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n2, n3);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment1, 0.1f);

    // Room for exactly one at the near house, which the Agent takes, and
    // plenty at the far one, which is the answer a search that counted the
    // Agent's own claim would fall back on.
    UnitType nearType("Home");
    nearType.targets.emplace_back("Home");
    nearType.resources.setCapacity("People", 1u);
    Unit& nearHome = city.addUnit(nearType, path, segment1, 0.9f);

    UnitType farType("Home");
    farType.targets.emplace_back("Home");
    farType.resources.setCapacity("People", 4u);
    city.addUnit(farType, path, segment2, 0.9f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_FALSE(agent.update(city.getRouter(), city.getConfig().routing, dt));
    ASSERT_EQ(agent.getRoute().getDestination(), &nearHome);
    ASSERT_EQ(nearHome.getReservedCount(), 1u);

    float const remaining = agent.getRemainingCost();
    ASSERT_GT(remaining, 0.0f);

    float const alternative = agent.computeRerouteCost(city.getRouter());
    ASSERT_LE(alternative, remaining + 1e-3f)
        << "the Agent is on its cheapest itinerary, yet rerouting is priced "
           "dearer than finishing";

    // Measuring must leave the claim exactly as it was.
    ASSERT_EQ(nearHome.getReservedCount(), 1u) << "the measurement dropped the claim";
    ASSERT_EQ(agent.m_reservation, &nearHome);
}

//------------------------------------------------------------------------------
//! \brief The claim is against the other Agents, not against oneself: an Agent
//! holding one through its own tick would find its destination full and never
//! be let in.
TEST(TestsAgent, ItsOwnClaimDoesNotShutTheDoorOnIt)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment, 0.9f);

    // Room for exactly one, which is the Agent's own claim.
    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 1u);
    Unit& home = city.addUnit(homeType, path, segment, 0.1f);

    AgentType people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent agent(1u, people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_GT(driveUntilDelivered(agent, city, dt, 4000u), 0u);
    ASSERT_EQ(home.getResources().getAmount("People"), 1u);
    ASSERT_EQ(home.getReservedCount(), 0u);
}

//------------------------------------------------------------------------------
//! \brief A building demolished under an Agent that had claimed a place there.
//! The claim has to be given back while it is still standing.
TEST(TestsAgent, GivesThePlaceBackBeforeTheBuildingGoes)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(60.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType workType("Work");
    workType.targets.emplace_back("Work");
    Unit& work = city.addUnit(workType, path, segment, 0.9f);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    homeType.resources.setCapacity("People", 4u);
    Unit& home = city.addUnit(homeType, path, segment, 0.1f);

    static AgentType const people("People", 10.0f, 3u, 42u);
    Resources carried;
    carried.addResource("People", 1u);
    Agent const& agent = city.addAgent(people, work, carried, "Home");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    for (uint32_t tick = 0u; tick < 20u; ++tick)
        city.update(dt);
    ASSERT_EQ(agent.getRoute().getDestination(), &home);
    ASSERT_EQ(home.getReservedCount(), 1u);

    city.removeUnit(home);
    ASSERT_EQ(agent.m_reservation, nullptr);
    ASSERT_NO_THROW(city.update(dt));
}

TEST(TestsAgent, ZeroLengthSegmentDoesNotCrash)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = path.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType homeType("Home");
    homeType.targets.emplace_back("Home");
    Unit u(homeType, n1, city);

    UnitType factoryType("Factory");
    factoryType.targets.emplace_back("People");
    factoryType.resources.setCapacity("People", 10u);
    city.addUnit(factoryType, n2);

    Resources carried;
    carried.addResource("People", 1u);
    Agent a(1u, keep<AgentType>("Worker", 5.0f, 3u, 42u), u, carried, "People");

    float const dt = 1.0f / defaults::TICKS_PER_SECOND;
    ASSERT_NO_THROW(a.update(city.getRouter(), city.getConfig().routing, dt));
    ASSERT_NO_THROW(a.update(city.getRouter(), city.getConfig().routing, dt));
}
