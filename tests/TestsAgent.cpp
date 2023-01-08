#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "OpenGlassBox/Agent.hpp"
#  include "OpenGlassBox/City.hpp"
#undef protected
#undef private

TEST(TestsAgent, Constructor)
{
    City city("Paris", 4, 4);
    UnitType unit_type("Home");
    unit_type.color = 0xFF00FF;
    unit_type.radius = 2u;
    unit_type.resources.addResource("oil", 5u);
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit u(unit_type, n, city);
    ASSERT_EQ(&n, &(u.m_node));

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
    ASSERT_EQ(a.m_currentWay, nullptr); // FIXME temporary
    ASSERT_EQ(a.m_lastNode, &n);
    ASSERT_EQ(a.m_lastNode, &(u.m_node));
    ASSERT_EQ(a.m_nextNode, nullptr);
}

TEST(TestsAgent, Move)
{
    const uint32_t GRILL_SIZE = 32u;
    City city("Paris", GRILL_SIZE, GRILL_SIZE);

    PathType type1("route", 0xAAAAAA);
    Path& p = city.addPath(type1);
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(3.0f, 2.0f, 3.0f));
    WayType type2("Dirt", 0xAAAAAA);
    /*Way& s1 =*/ p.addWay(type2, n1, n2);

    Resources r;
    UnitType type("Home");
    type.color = 0xFF00FF;
    type.radius = 1u;
    type.resources = r;
    Unit u(type, n1, city);
    AgentType c("Worker", 5.0f, 3u, 42u);
    Agent a(43u, c, u, r, "???");

    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentWay, nullptr); // TODO &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_lastNode, &(u.m_node));
    ASSERT_EQ(&n1, &(u.m_node));
    ASSERT_EQ(a.m_nextNode, nullptr); // &n2);

#if 0 // TODO
    ASSERT_EQ(a.update(city), false); // TODO tester true
    ASSERT_GT(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_GT(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentWay, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, &n2);
#endif
}
