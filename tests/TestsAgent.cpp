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
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit u("U", n);
    Resources r; r.addResource("oil", 5u);
    Agent a(43u, n, u, r, "???");
    AgentConfig c(5.0f, 3.0f, 42u);
    a.configure(c);

    ASSERT_EQ(a.m_id, 43u);
    ASSERT_EQ(&(a.m_owner), &u);
    ASSERT_EQ(a.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(a.m_resources.getAmount("oil"), 5u);
    ASSERT_EQ(a.m_searchTarget, "???");
    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, nullptr); // FIXME temporary
    ASSERT_EQ(a.m_lastNode, &n);
    ASSERT_EQ(a.m_nextNode, nullptr);
    ASSERT_EQ(a.m_speed, 5.0f);
    ASSERT_EQ(a.m_radius, 3.0f);
    ASSERT_EQ(a.m_color, 42u);
}

TEST(TestsAgent, Move)
{
    const uint32_t GRILL_SIZE = 32u;
    City city("Paris", GRILL_SIZE, GRILL_SIZE);

    Path& p = city.addPath("route");
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(3.0f, 2.0f, 3.0f));
    Segment& s1 = p.addSegment(n1, n2);

    Unit u("U", n1); Resources r;
    Agent a(43u, n1, u, r, "???");

    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, &s1);
    ASSERT_EQ(a.m_lastNode, &n1);
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
