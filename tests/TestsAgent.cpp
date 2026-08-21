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
