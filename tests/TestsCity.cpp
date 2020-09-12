#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/Core/City.hpp"
#  include "src/Core/Path.hpp"
#undef protected
#undef private

TEST(TestsCity, Constructor)
{
    const uint32_t GRILL = 4u;

    City c("Paris", GRILL, GRILL + 1u);

    ASSERT_STREQ(c.id().c_str(), "Paris");
    ASSERT_EQ(c.m_gridSizeX, GRILL);
    ASSERT_EQ(c.m_gridSizeY, GRILL + 1u);
    ASSERT_EQ(c.m_nextUnitId, 0u);
    ASSERT_EQ(c.m_nextAgentId, 0u);
    ASSERT_EQ(c.m_globals.m_bin.size(), 0u);
    ASSERT_EQ(c.m_globals.isEmpty(), true);
    ASSERT_EQ(c.m_maps.size(), 0u);
    ASSERT_EQ(c.m_paths.size(), 0u);
    ASSERT_EQ(c.m_units.size(), 0u);
    ASSERT_EQ(c.m_agents.size(), 0u);
}

TEST(TestsCity, addMap)
{
    const uint32_t GRILL = 4u;
    City c("Paris", GRILL, GRILL);

    // Add Map1.
    Map& m1 = c.addMap("map1");
    Map& m2 = c.getMap("map1");
    ASSERT_EQ(&m1, &m2);

    // Add Map2.
    Map& m3 = c.addMap("map2");
    Map& m4 = c.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m3, &m4);

   // Add again Map2. Check previous map disapeared
    Map& m5 = c.addMap("map2");
    Map& m6 = c.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m5, &m6);
    ASSERT_NE(&m6, &m4);

    // Add Path
    Path& p1 = c.addPath("path1");
    Path& p2 = c.getPath("path1");
    ASSERT_EQ(&p1, &p2);

    // Add units
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f)/*, p1*/);
    Unit& u1 = c.addUnit("unit1", n1);
    ASSERT_EQ(c.getUnits().size(), 1u);
    Unit& u2 = *(c.getUnits()[0]);
    ASSERT_EQ(&u1, &u2);

    // Add agent
    Resources r;
    Agent& a1 = c.addAgent(n1, u1, r, "???");
    Agent& a2 = *(c.getAgents()[0]);
    ASSERT_EQ(&a1, &a2);
}

TEST(TestsCity, CreateCity)
{
    const uint32_t GRILL_SIZE = 32u;
    City c("Paris", GRILL_SIZE, GRILL_SIZE);

    Path& p = c.addPath("route");
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    Segment& s1 = p.addSegment(n1, n2);

// TODO a finir
}

TEST(TestsCity, update)
{
    // FIXME TODO
}
