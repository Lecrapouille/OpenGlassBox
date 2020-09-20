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

    ASSERT_STREQ(c.name().c_str(), "Paris");
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
    MapType type1("map1");
    Map& m1 = c.addMap(type1);
    Map& m2 = c.getMap("map1");
    ASSERT_EQ(&m1, &m2);
    ASSERT_STREQ(m1.type().c_str(), "map1");
    ASSERT_EQ(m1.capacity(), MapType::MAX_CAPACITY);
    ASSERT_EQ(m1.color(), 0xFFFFFF);

    // Add Map2.
    MapType type2("map2", 0x00, 10u);
    Map& m3 = c.addMap(type2);
    Map& m4 = c.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m3, &m4);
    ASSERT_STREQ(m4.type().c_str(), "map2");
    ASSERT_EQ(m4.capacity(), 10u);
    ASSERT_EQ(m4.color(), 0x00);

   // Add again Map2. Check previous map disapeared
    Map& m5 = c.addMap(type2);
    Map& m6 = c.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m5, &m6);
    ASSERT_NE(&m6, &m4);

    // Add Path
    PathType type3("path1");
    Path& p1 = c.addPath(type3);
    Path& p2 = c.getPath("path1");
    ASSERT_EQ(&p1, &p2);

    // Add units
    Resources r;
    UnitType type4 = { "unit1", 0xFF00FF, r, {}, {} };
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f)/*, p1*/);
    Unit& u1 = c.addUnit(type4, n1);
    ASSERT_EQ(c.getUnits().size(), 1u);
    Unit& u2 = *(c.getUnits()[0]);
    ASSERT_EQ(&u1, &u2);
    ASSERT_EQ(u2.color(), 0xFF00FF);

    // Add agent
    AgentType t(1.0f, 2u, 0xFFFFFF); t.m_speed = 42.0f;
    Agent& a1 = c.addAgent(t, u1, r, "???");
    Agent& a2 = *(c.getAgents()[0]);
    ASSERT_EQ(&a1, &a2);
    ASSERT_EQ(a1.m_speed, t.m_speed);
    ASSERT_EQ(a1.m_color, t.m_color);
    ASSERT_EQ(a1.m_speed, t.m_speed);
    ASSERT_EQ(&(a1.m_owner), &u1);
}

TEST(TestsCity, CreateCity)
{
    const uint32_t GRILL_SIZE = 32u;
    City c("Paris", GRILL_SIZE, GRILL_SIZE);

    PathType type1("Road");
    Path& p = c.addPath(type1);
    Node& n1 = p.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    SegmentType type2("Dirt", 0xAAAAAA);
    Segment& s1 = p.addSegment(type2, n1, n2);
    ASSERT_STREQ(s1.name().c_str(), "Dirt");
    ASSERT_EQ(s1.color(), 0xAAAAAA);

// TODO a finir
}

TEST(TestsCity, update)
{
    // FIXME TODO
}
