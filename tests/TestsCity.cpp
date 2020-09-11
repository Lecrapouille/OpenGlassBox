#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/City.hpp"
#  include "src/Path.hpp"
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
    Map& m1 = c.addMap("map1");
    Map& m2 = c.getMap("map1");
    ASSERT_EQ(&m1, &m2);

    Map& m3 = c.addMap("map2");
    Map& m4 = c.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m3, &m4);

    Map& m5 = c.addMap("map2");
    Map& m6 = c.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m5, &m6);

    Path& p1 = c.addPath("path1");
    Path& p2 = c.getPath("path1");
    ASSERT_EQ(&p1, &p2);

    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f)/*, p1*/);
    Unit& u1 = c.addUnit("unit1", n1);
    //Unit& u2 = c.getUnit("unit1");
    //ASSERT_EQ(&u1, &u2);

    // FIXME segfault
    //Resources r;
    //Agent& a1 = c.addAgent(n1, u1, r, "???");
    //Agent& a2 = c.getAgent("unit1");
    //ASSERT_EQ(&a1, &a2);
}

TEST(TestsCity, update)
{
    // FIXME TODO
}
