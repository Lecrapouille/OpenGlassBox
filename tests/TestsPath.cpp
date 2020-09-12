#include "main.hpp"

#define protected public
#define private public
//#  include "src/Path.hpp"
#  include "src/City.hpp"
#  include "src/Unit.hpp"
#undef protected
#undef private

TEST(TestsNode, Constructor)
{
    //City c("Paris", 32u, 32u);
    //Path p("route", c);
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));

    ASSERT_EQ(n.id(), 42u);
    ASSERT_EQ(int32_t(n.position().x), 1);
    ASSERT_EQ(int32_t(n.position().y), 2);
    ASSERT_EQ(int32_t(n.position().z), 3);
    //ASSERT_STREQ(n.path().id().c_str(), "route");
    ASSERT_EQ(n.m_segments.size(), 0u);
    ASSERT_EQ(n.m_units.size(), 0u);
}

TEST(TestsNode, AddUnit)
{
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Node n2(43u, Vector3f(2.0f, 3.0f, 4.0f));
    ASSERT_EQ(n1.m_units.size(), 0u);
    ASSERT_EQ(n2.m_units.size(), 0u);

    // Add unit u1 to node n1. Check relationship.
    Unit u1("maison", n1);
    ASSERT_EQ(n1.m_units.size(), 1u);
    ASSERT_EQ(n1.m_units[0], &u1);
    ASSERT_EQ(n1.units()[0], &u1);
    ASSERT_EQ(&(n1.m_units[0]->m_node), &n1);
    ASSERT_STREQ(n1.m_units[0]->id().c_str(), "maison");

    // Add again unit u1 to node n2.
    n2.addUnit(u1);
    ASSERT_EQ(n2.m_units.size(), 1u);
    ASSERT_EQ(n2.m_units[0], &u1);
    ASSERT_EQ(n2.units()[0], &u1);
    ASSERT_EQ(&(n2.m_units[0]->m_node), &n1);
    ASSERT_STREQ(n2.m_units[0]->id().c_str(), "maison");

    // Add unit u2 to node n1.
    Unit u2("maison", n2);
    n1.addUnit(u2);
    ASSERT_EQ(n1.m_units.size(), 2u);
    ASSERT_EQ(n1.m_units[0], &u1);
    ASSERT_EQ(n1.m_units[1], &u2);
    ASSERT_EQ(n1.units()[0], &u1);
    ASSERT_EQ(n1.units()[1], &u2);
    ASSERT_STREQ(n1.m_units[0]->id().c_str(), "maison");
    ASSERT_STREQ(n1.m_units[1]->id().c_str(), "maison");
    ASSERT_EQ(&(n1.m_units[0]->m_node), &n1);
    ASSERT_EQ(&(n1.m_units[1]->m_node), &n2);
}

TEST(TestsNode, getMapPosition)
{
    const uint32_t GRILL_SIZE = 32u;

    City c("Paris", GRILL_SIZE, GRILL_SIZE);
    Node n(42u, Vector3f(1.0f, 1.0f, 0.0f));

    uint32_t u, v;
    n.getMapPosition(c, u, v);
    ASSERT_EQ(u, 0u);
    ASSERT_EQ(v, 0u);

    // Position 1
    n.m_position = Vector3f(2.0f, 2.0f, 0.0f);
    n.getMapPosition(c, u, v);
    ASSERT_EQ(u, 1u);
    ASSERT_EQ(v, 1u);

    // Position 2
    n.m_position = Vector3f(4.0f, 2.0f, 0.0f);
    n.getMapPosition(c, u, v);
    ASSERT_EQ(u, 2u);
    ASSERT_EQ(v, 1u);

    // Position 3
    n.m_position = Vector3f(2.0f, 4.0f, 0.0f);
    n.getMapPosition(c, u, v);
    ASSERT_EQ(u, 1u);
    ASSERT_EQ(v, 2u);

    // //Pathological case: out of the map
    n.m_position = Vector3f(-2.0f, -2.0f, 0.0f);
    n.getMapPosition(c, u, v);
    ASSERT_EQ(u, 0u);
    ASSERT_EQ(v, 0u);

    // //Pathological case: out of the map
    n.m_position = Vector3f(2000.0f, 2000.0f, 0.0f);
    n.getMapPosition(c, u, v);
    ASSERT_EQ(u, GRILL_SIZE - 1u);
    ASSERT_EQ(v, GRILL_SIZE - 1u);
}

TEST(TestsSegment, Constuctor)
{
    //const uint32_t GRILL_SIZE = 32u;

    //City c("Paris", GRILL_SIZE, GRILL_SIZE);
    //Path p("route", c);
    Node n1(42u, Vector3f(1.0f, 1.0f, 0.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 0.0f));

    Segment s1(55u, n1, n2);
    ASSERT_EQ(s1.id(), 55u);
    ASSERT_EQ(s1.m_node1, &n1);
    ASSERT_EQ(s1.m_node2, &n2);
    ASSERT_EQ(&s1.node1(), &n1);
    ASSERT_EQ(&s1.node2(), &n2);
    ASSERT_EQ(s1.length(), std::sqrt(2.0f));
}

TEST(TestsSegment, changeNode2)
{
    Node n1(42u, Vector3f(1.0f, 1.0f, 0.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 0.0f));
    Node n3(43u, Vector3f(3.0f, 3.0f, 0.0f));

    // Create the following path:
    //
    //     s1
    // |-------|       |
    // n1      n2      n3
    //
    Segment s1(55u, n1, n2);
    ASSERT_EQ(&s1.node2(), &n2);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 1u);
    ASSERT_EQ(&*(n2.m_segments[0]), &s1);
    ASSERT_EQ(n3.m_segments.size(), 0u);

    // Modify the path:
    //     s1
    // |-------|
    // n1      n3
    //
    s1.changeNode2(n3);
    ASSERT_NE(&n2, &n3);
    ASSERT_EQ(&s1.node1(), &n1);
    ASSERT_EQ(&s1.node2(), &n3);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 0u);
    ASSERT_EQ(n3.m_segments.size(), 1u);
    ASSERT_EQ(&*(n3.m_segments[0]), &s1);

    Segment* s = n1.getSegmentToNode(n2);
    ASSERT_EQ(s, nullptr);
    s = n1.getSegmentToNode(n3);
    ASSERT_EQ(s, &s1);
    s = n1.getSegmentToNode(n1);
    ASSERT_EQ(s, nullptr);
}

TEST(TestsSegment, PathchangeNode2)
{
    Path p("route");

    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(2.0f, 2.0f, 0.0f));
    Node& n3 = p.addNode(Vector3f(3.0f, 3.0f, 0.0f));
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(&*(p.m_nodes[2]), &n3);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_nodes[2]->id(), 2u);
    ASSERT_EQ(p.m_nodes[0]->position().x, 1.0f);
    ASSERT_EQ(p.m_nodes[1]->position().x, 2.0f);
    ASSERT_EQ(p.m_nodes[2]->position().x, 3.0f);


    // Create the following path:
    //
    //     s1
    // |-------|       |
    // n1      n2      n3
    //
    Segment& s1 = p.addSegment(n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_segments.size(), 1u);
    ASSERT_EQ(&s1.node2(), &n2);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 1u);
    ASSERT_EQ(&*(n2.m_segments[0]), &s1);
    ASSERT_EQ(n3.m_segments.size(), 0u);

    // Modify the segment:
    //         s1
    // |---------------|
    // n1      |      n3
    //         n2
    //
    s1.changeNode2(n3);
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_segments.size(), 1u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(&*(p.m_nodes[2]), &n3);
    ASSERT_EQ(&s1.node1(), &n1);
    ASSERT_EQ(&s1.node2(), &n3);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 0u);
    ASSERT_EQ(n3.m_segments.size(), 1u);
    ASSERT_EQ(&*(n3.m_segments[0]), &s1);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_nodes[2]->id(), 2u);
    ASSERT_EQ(p.m_nodes[0]->position().x, 1.0f);
    ASSERT_EQ(p.m_nodes[1]->position().x, 2.0f);
    ASSERT_EQ(p.m_nodes[2]->position().x, 3.0f);

    Segment* s = n1.getSegmentToNode(n2);
    ASSERT_EQ(s, nullptr);
    s = n1.getSegmentToNode(n3);
    ASSERT_EQ(s, &s1);
    s = n1.getSegmentToNode(n1);
    ASSERT_EQ(s, nullptr);
}

TEST(TestsNode, getSegmentToNode)
{
    // Create the following path:
    //
    //     s1      s2
    // |-------|-------|     X
    // n2      n1      n3    n4
    //
    Node n1(42u, Vector3f(1.0f, 1.0f, 0.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 0.0f));
    Node n3(43u, Vector3f(3.0f, 3.0f, 0.0f));
    Node n4(44u, Vector3f(3.0f, 4.0f, 0.0f));
    Segment s1(55u, n1, n2);
    Segment s2(56u, n1, n3);

    // Check that n1 has two neighboring segments.
    Segment* s = n1.getSegmentToNode(n2);
    ASSERT_EQ(s, &s1);
    s = n1.getSegmentToNode(n3);
    ASSERT_EQ(s, &s2);

    // Check that n4 has no neighboring segments.
    s = n4.getSegmentToNode(n3);
    ASSERT_EQ(s, nullptr);

    // Check that n4 has no segment starting and leaving from it
    s = n4.getSegmentToNode(n4);
    ASSERT_EQ(s, nullptr);

    // Add a loop segment
    Segment s3(57u, n4, n4);
    s = n4.getSegmentToNode(n4);
    ASSERT_EQ(s, &s3);

    // Check that n1 has no segment starting and leaving from it
    s = n1.getSegmentToNode(n1);
    ASSERT_EQ(s, nullptr);
}

TEST(TestsPath, Constructor)
{
    Path p("route");

    ASSERT_STREQ(p.id().c_str(), "route");
    ASSERT_EQ(p.m_nodes.size(), 0u);
    ASSERT_EQ(p.m_segments.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 0u);
    ASSERT_EQ(p.m_nextSegmentId, 0u);
}

TEST(TestsPath, Adding)
{
    Path p("route");

    // Add 1st node on the path.
    // Check new node added in the path.
    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 0.0f));
    ASSERT_EQ(p.m_nodes.size(), 1u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_segments.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 1u);
    ASSERT_EQ(p.m_nextSegmentId, 0u);

    // Add 2nd node on the path.
    // Check new node added in the path.
    Node& n2 = p.addNode(Vector3f(2.0f, 2.0f, 0.0f));
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_segments.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextSegmentId, 0u);

    // Add 1st segment on the path.
    // Check new segment added in the path.
    Segment& s1 = p.addSegment(n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_segments.size(), 1u);
    ASSERT_EQ(&*(p.m_segments[0]), &s1);
    ASSERT_EQ(p.m_segments[0]->id(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextSegmentId, 1u);

    // Add 2nd segment on the 1st path.
    // Check new segment added in the path.
    // FIXME Replace the segment or allow multi-graph (== speedway) ?
    Segment& s2 = p.addSegment(n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_segments.size(), 2u);
    ASSERT_EQ(&*(p.m_segments[0]), &s1);
    ASSERT_EQ(&*(p.m_segments[1]), &s2);
    ASSERT_EQ(p.m_segments[0]->id(), 0u);
    ASSERT_EQ(p.m_segments[1]->id(), 1u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextSegmentId, 2u);
}

TEST(TestsPath, SplitSegment)
{
    Path p("route");

    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(1.0f, 3.0f, 0.0f));
    Segment& s1 = p.addSegment(n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_segments.size(), 1u);

    // Split segment on the first node.
    // Check no new segment has been created.
    Node& n3 = p.splitSegment(s1, 0.0f);
    ASSERT_EQ(&n3, &n1);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_segments.size(), 1u);

    // Split segment on the second node.
    // Check no new segment has been created.
    Node& n4 = p.splitSegment(s1, 1.0f);
    ASSERT_EQ(&n4, &n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_segments.size(), 1u);

    // Split segment on its middle.
    // Check a new segment and a new node have been created.
    Node& n5 = p.splitSegment(s1, 0.5f);
    ASSERT_NE(&n5, &n1);
    ASSERT_NE(&n5, &n2);
    ASSERT_EQ(n5.position().x, 1.0f);
    ASSERT_EQ(n5.position().y, 2.0f);
    ASSERT_EQ(n5.position().z, 0.0f);
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_segments.size(), 2u);
}
