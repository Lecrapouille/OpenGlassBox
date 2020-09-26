#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/Core/Agent.hpp"
#  include "src/Core/City.hpp"
#undef protected
#undef private

TEST(TestsAgent, Constructor)
{
    City city("Paris", 4, 4);
    Resources r; r.addResource("oil", 5u);
    UnitType type = { "Home", 0xFF00FF, 2u, r, {}, {} };
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit u(type, n, city);
    AgentType c(5.0f, 3u, 42u);
    Agent a(43u, c, u, r, "target");

    ASSERT_EQ(a.m_id, 43u);
    ASSERT_EQ(&(a.m_owner), &u);
    ASSERT_EQ(a.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(a.m_resources.getAmount("oil"), 5u);
    //FIXME ASSERT_EQ(a.m_searchTarget, "target");
    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, nullptr); // FIXME temporary
    ASSERT_EQ(a.m_lastNode, &n);
    ASSERT_EQ(a.m_lastNode, &(u.m_node));
    ASSERT_EQ(&n, &(u.m_node));
    ASSERT_EQ(a.m_nextNode, nullptr);
    ASSERT_EQ(a.m_speed, 5.0f);
    ASSERT_EQ(a.m_radius, 3.0f);
    ASSERT_EQ(a.m_color, 42u);
}

TEST(TestsAgent, Move)
{
    const uint32_t GRILL_SIZE = 32u;
    City city("Paris", GRILL_SIZE, GRILL_SIZE);

    PathType type1("route"/*, 0xAAAAAA*/);
    Path& p = city.addPath(type1);
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(3.0f, 2.0f, 3.0f));
    SegmentType type2("Dirt", 0xAAAAAA);
    Segment& s1 = p.addSegment(type2, n1, n2);

    Resources r;
    UnitType type = { "Home", 0xFF00FF, 1u, r, {}, {} };
    Unit u(type, n1, city);
    AgentType c(5.0f, 3u, 42u);
    Agent a(43u, c, u, r, "???");

    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_lastNode, &(u.m_node));
    ASSERT_EQ(&n1, &(u.m_node));
    ASSERT_EQ(a.m_nextNode, &n2);

    a.move(city);
    ASSERT_GT(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_GT(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
    ASSERT_EQ(a.m_nextNode, &n2);
}
