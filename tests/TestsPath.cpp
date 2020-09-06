#include "main.hpp"

#define protected public
#define private public
#  include "src/Path.hpp"
#  include "src/City.hpp"
#  include "src/Unit.hpp"
#undef protected
#undef private

TEST(TestsNode, Constructor)
{
    City c("Paris", 32u, 32u);
    Path p("route", c);
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f), p);

    ASSERT_EQ(n.id(), 42u);
    ASSERT_EQ(int32_t(n.position().x), 1);
    ASSERT_EQ(int32_t(n.position().y), 2);
    ASSERT_EQ(int32_t(n.position().z), 3);
    ASSERT_STREQ(n.path().id().c_str(), "route");
    ASSERT_EQ(n.m_segments.size(), 0u);
    ASSERT_EQ(n.m_units.size(), 0u);
}

// FIXME node::m_unit KO
TEST(TestsNode, AddUnit)
{
    City c("Paris", 32u, 32u);
    Path p("route", c);
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f), p);
    Node n2(43u, Vector3f(2.0f, 3.0f, 4.0f), p);

    ASSERT_EQ(n1.m_units.size(), 0u);
    ASSERT_EQ(n2.m_units.size(), 0u);

    Unit u1("maison", n1);
    ASSERT_EQ(n1.m_units.size(), 1u);
    ASSERT_NE(n1.m_units[0], nullptr);
    ASSERT_STREQ(n1.m_units[0]->id().c_str(), "maison");
    ASSERT_EQ(&(n1.m_units[0]->m_node), &n1);

    n2.addUnit(u1);
    ASSERT_EQ(n2.m_units.size(), 1u);
    ASSERT_NE(n2.m_units[0], nullptr);
    ASSERT_STREQ(n2.m_units[0]->id().c_str(), "maison");
    // EXPECT_EQ(&(n2.m_units[0]->m_node), &n2); // FIXME KO shall not be &n1

    Unit u2("maison", n2);
    n1.addUnit(u2);
    ASSERT_EQ(n1.m_units.size(), 2u);
    ASSERT_NE(n1.m_units[0], nullptr);
    ASSERT_NE(n1.m_units[1], nullptr);
    ASSERT_STREQ(n1.m_units[0]->id().c_str(), "maison");
    ASSERT_EQ(&(n1.m_units[0]->m_node), &n1);
    ASSERT_STREQ(n1.m_units[1]->id().c_str(), "maison");
    ASSERT_EQ(&(n1.m_units[1]->m_node), &n2);
}

TEST(TestsNode, getMapPosition)
{
    const uint32_t GRILL_SIZE = 32u;

    City c("Paris", GRILL_SIZE, GRILL_SIZE);
    Path p("route", c);
    Node n(42u, Vector3f(1.0f, 1.0f, 0.0f), p);

    uint32_t u, v;
    n.getMapPosition(u, v);
    ASSERT_EQ(u, 0u);
    ASSERT_EQ(v, 0u);

    n.m_position = Vector3f(2.0f, 2.0f, 0.0f);
    n.getMapPosition(u, v);
    ASSERT_EQ(u, 1u);
    ASSERT_EQ(v, 1u);

    n.m_position = Vector3f(4.0f, 2.0f, 0.0f);
    n.getMapPosition(u, v);
    ASSERT_EQ(u, 2u);
    ASSERT_EQ(v, 1u);

    // Pathological case
    n.m_position = Vector3f(-2.0f, -2.0f, 0.0f);
    n.getMapPosition(u, v);
    ASSERT_EQ(u, 0u);
    ASSERT_EQ(v, 0u);

    n.m_position = Vector3f(2000.0f, 2000.0f, 0.0f);
    n.getMapPosition(u, v);
    ASSERT_EQ(u, GRILL_SIZE - 1u);
    ASSERT_EQ(v, GRILL_SIZE - 1u);
}
