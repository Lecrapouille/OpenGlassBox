#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/Building.hpp"
#undef protected
#undef private

TEST(TestsNode, Constructor)
{
    // Construct a dummy node (not knowing segments or buildings)
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));

    // Check initial values (member variables).
    ASSERT_EQ(n.m_id, 42u);
    ASSERT_EQ(int32_t(n.m_position.x), 1);
    ASSERT_EQ(int32_t(n.m_position.y), 2);
    ASSERT_EQ(int32_t(n.m_position.z), 3);
    ASSERT_EQ(n.m_segments.size(), 0u);
    ASSERT_EQ(n.m_buildings.size(), 0u);

    // Check initial values (getter methods).
    ASSERT_EQ(n.getId(), 42u);
    ASSERT_EQ(int32_t(n.getPosition().x), 1);
    ASSERT_EQ(int32_t(n.getPosition().y), 2);
    ASSERT_EQ(int32_t(n.getPosition().z), 3);
    ASSERT_EQ(n.getSegments().size(), 0u);
    ASSERT_EQ(n.getBuildings().size(), 0u);
}

// Test adding buildings on a Node
TEST(TestsNode, AddBuilding)
{
    // Create two Nodes. Check no buildings are attached.
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Node n2(43u, Vector3f(2.0f, 3.0f, 4.0f));
    ASSERT_EQ(n1.getBuildings().size(), 0u);
    ASSERT_EQ(n2.getBuildings().size(), 0u);

    // Create an Building "house" holding resources "people" attached to Node1.
    TestWorld cityWorld("Paris", 1u, 1u);
    City& city = cityWorld.city;
    BuildingType building_type("house");
    building_type.color = 0xFF00FF;
    building_type.radius = 2u;
    building_type.resources.setCapacity("people", 10);
    building_type.resources.addResource("people", 10);
    Building u1(building_type, n1, city);

    // Check one Building has been added knowing Node1.
    ASSERT_EQ(n1.getBuildings().size(), 1u);
    ASSERT_EQ(n1.m_buildings[0], &u1);
    ASSERT_EQ(n1.getBuildings()[0], &u1);
    ASSERT_EQ(n1.getBuildings()[0]->m_node, &n1);
    ASSERT_STREQ(n1.getBuildings()[0]->getTypeName().c_str(), "house");

    // Add building u1 to Node2. Check if the Building has been attached.
    n2.addBuilding(u1);
    ASSERT_EQ(n2.getBuildings().size(), 1u);
    ASSERT_EQ(n2.m_buildings[0], &u1);
    ASSERT_EQ(n2.getBuildings()[0], &u1);
    ASSERT_EQ(n2.m_buildings[0]->m_node, &n1);
    ASSERT_STREQ(n2.m_buildings[0]->getTypeName().c_str(), "house");

    // Add building u2 to Node1. Check if the Building has been attached.
    Building u2(building_type, n2, city);
    n1.addBuilding(u2);
    ASSERT_EQ(n1.m_buildings.size(), 2u);
    ASSERT_EQ(n1.m_buildings[0], &u1);
    ASSERT_EQ(n1.m_buildings[1], &u2);
    ASSERT_EQ(n1.getBuildings()[0], &u1);
    ASSERT_EQ(n1.getBuildings()[1], &u2);
    ASSERT_STREQ(n1.m_buildings[0]->getTypeName().c_str(), "house");
    ASSERT_STREQ(n1.m_buildings[1]->getTypeName().c_str(), "house");
    ASSERT_EQ(n1.m_buildings[0]->m_node, &n1);
    ASSERT_EQ(n1.m_buildings[1]->m_node, &n2);
}

TEST(TestsSegment, Constuctor)
{
    // Create two Nodes.
    Node n1(42u, Vector3f(1.0f, 1.0f, 0.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 0.0f));

    // Create a segment linking the two Nodes.
    SegmentType type("Dirt", 0xAAAAAA);
    Segment s1(55u, type, n1, n2);

    // Check if nodes are correctly hold by the Segment
    ASSERT_EQ(s1.getId(), 55u);
    ASSERT_STREQ(s1.getTypeName().c_str(), "Dirt");
    ASSERT_EQ(s1.getColor(), 0xAAAAAAu);
    ASSERT_EQ(s1.m_from, &n1);
    ASSERT_EQ(s1.m_to, &n2);
    ASSERT_EQ(&s1.getFrom(), &n1);
    ASSERT_EQ(&s1.getTo(), &n2);
    ASSERT_EQ(s1.getLength(), std::sqrt(2.0f));
}

#if 0 // This method no longer exist and has been merged in its parent method
// Test changeNode2 without using Path
TEST(TestsSegment, changeNode2)
{
    Node n1(42u, Vector3f(1.0f, 1.0f, 1.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 2.0f));
    Node n3(43u, Vector3f(3.0f, 3.0f, 3.0f));

    SegmentType type("Dirt", 0xAAAAAA);
    Segment s1(55u, type, n1, n2);

    // Check the graph is the following:
    //     s1
    // |-------|       |
    // n1      n2      n3
    //
    ASSERT_EQ(&s1.getTo(), &n2);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 1u);
    ASSERT_EQ(n3.m_segments.size(), 0u);
    ASSERT_EQ(&*(n1.m_segments[0]), &s1);
    ASSERT_EQ(&*(n2.m_segments[0]), &s1);

    // Change the Node2
    s1.changeNode2(n3);

    // Check the modified path is the follow:
    //         s1
    // |---------------|
    // n1      |      n3
    //         n2
    //
    ASSERT_NE(&n2, &n3);
    ASSERT_EQ(&s1.getFrom(), &n1);
    ASSERT_EQ(&s1.getTo(), &n3);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 0u);
    ASSERT_EQ(n3.m_segments.size(), 1u);
    ASSERT_EQ(&*(n1.m_segments[0]), &s1);
    ASSERT_EQ(&*(n3.m_segments[0]), &s1);

    // Check positions of Nodes did not changed
    ASSERT_EQ(int32_t(n1.m_position.x), 1);
    ASSERT_EQ(int32_t(n1.m_position.y), 1);
    ASSERT_EQ(int32_t(n1.m_position.z), 1);
    ASSERT_EQ(int32_t(n2.m_position.x), 2);
    ASSERT_EQ(int32_t(n2.m_position.y), 2);
    ASSERT_EQ(int32_t(n2.m_position.z), 2);
    ASSERT_EQ(int32_t(n3.m_position.x), 3);
    ASSERT_EQ(int32_t(n3.m_position.y), 3);
    ASSERT_EQ(int32_t(n3.m_position.z), 3);

    // Check Segments of Node
    ASSERT_EQ(n1.findSegmentTo(n1), nullptr);
    ASSERT_EQ(n1.findSegmentTo(n2), nullptr);
    ASSERT_EQ(n1.findSegmentTo(n3), &s1);
}

// Test changeNode2 using Path
TEST(TestsSegment, PathchangeNode2)
{
    PathType type1("route");
    Path p(type1);
    ASSERT_STREQ(p.getTypeName().c_str(), "route");

    // Create Nodes
    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 1.0f));
    Node& n2 = p.addNode(Vector3f(2.0f, 2.0f, 2.0f));
    Node& n3 = p.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(&*(p.m_nodes[2]), &n3);
    ASSERT_EQ(p.m_nodes[0]->getId(), 0u);
    ASSERT_EQ(p.m_nodes[1]->getId(), 1u);
    ASSERT_EQ(p.m_nodes[2]->getId(), 2u);
    ASSERT_EQ(p.m_nodes[0]->getPosition().x, 1.0f);
    ASSERT_EQ(p.m_nodes[1]->getPosition().x, 2.0f);
    ASSERT_EQ(p.m_nodes[2]->getPosition().x, 3.0f);

    // Create a graph
    SegmentType type2("road");
    Segment& s1 = p.addSegment(type2, n1, n2);

    // Check the graph is the following:
    //     s1
    // |-------|       |
    // n1      n2      n3
    //
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_segments.size(), 1u);
    ASSERT_EQ(&s1.getTo(), &n2);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 1u);
    ASSERT_EQ(&*(n2.m_segments[0]), &s1);
    ASSERT_EQ(n3.m_segments.size(), 0u);

    // Change the Node2
    s1.changeNode2(n3);

    // Check the modified path is the follow:
    //         s1
    // |---------------|
    // n1      |      n3
    //         n2
    //
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_segments.size(), 1u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(&*(p.m_nodes[2]), &n3);
    ASSERT_EQ(&s1.getFrom(), &n1);
    ASSERT_EQ(&s1.getTo(), &n3);
    ASSERT_EQ(n1.m_segments.size(), 1u);
    ASSERT_EQ(n2.m_segments.size(), 0u);
    ASSERT_EQ(n3.m_segments.size(), 1u);
    ASSERT_EQ(&*(n1.m_segments[0]), &s1);
    ASSERT_EQ(&*(n3.m_segments[0]), &s1);
    ASSERT_EQ(p.m_nodes[0]->getId(), 0u);
    ASSERT_EQ(p.m_nodes[1]->getId(), 1u);
    ASSERT_EQ(p.m_nodes[2]->getId(), 2u);
    ASSERT_EQ(p.m_nodes[0]->getPosition().x, 1.0f);
    ASSERT_EQ(p.m_nodes[1]->getPosition().x, 2.0f);
    ASSERT_EQ(p.m_nodes[2]->getPosition().x, 3.0f);

    // Check positions of Nodes did not changed
    ASSERT_EQ(int32_t(p.m_nodes[0]->m_position.x), 1);
    ASSERT_EQ(int32_t(p.m_nodes[0]->m_position.y), 1);
    ASSERT_EQ(int32_t(p.m_nodes[0]->m_position.z), 1);
    ASSERT_EQ(int32_t(p.m_nodes[1]->m_position.x), 2);
    ASSERT_EQ(int32_t(p.m_nodes[1]->m_position.y), 2);
    ASSERT_EQ(int32_t(p.m_nodes[1]->m_position.z), 2);
    ASSERT_EQ(int32_t(p.m_nodes[2]->m_position.x), 3);
    ASSERT_EQ(int32_t(p.m_nodes[2]->m_position.y), 3);
    ASSERT_EQ(int32_t(p.m_nodes[2]->m_position.z), 3);

    // Check Segments of Node
    ASSERT_EQ(n1.findSegmentTo(n1), nullptr);
    ASSERT_EQ(n1.findSegmentTo(n2), nullptr);
    ASSERT_EQ(n1.findSegmentTo(n3), &s1);
}
#endif

TEST(TestsNode, findSegmentTo)
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
    SegmentType type("road");
    Segment s1(55u, type, n1, n2);
    Segment s2(56u, type, n1, n3);

    // Check that n1 has two neighboring segments.
    ASSERT_EQ(n1.findSegmentTo(n2), &s1);
    ASSERT_EQ(n1.findSegmentTo(n3), &s2);

    // Check that n4 has no neighboring segments.
    ASSERT_EQ(n4.findSegmentTo(n3), nullptr);

    // Check that n4 has no segment starting and leaving from it
    ASSERT_EQ(n4.findSegmentTo(n4), nullptr);

    // Add a loop segment
    Segment s3(57u, type, n4, n4);
    ASSERT_EQ(n4.findSegmentTo(n4), &s3);

    // Check that n1 has no segment starting and leaving from it
    ASSERT_EQ(n1.findSegmentTo(n1), nullptr);
}

TEST(TestsPath, Constructor)
{
    PathType type("route");
    Path p(type);

    ASSERT_STREQ(p.getTypeName().c_str(), "route");
    ASSERT_EQ(p.m_nodes.size(), 0u);
    ASSERT_EQ(p.m_segments.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 0u);
    ASSERT_EQ(p.m_nextSegmentId, 0u);
}

TEST(TestsPath, Adding)
{
    PathType type1("route");
    Path p(type1);

    // Add 1st node on the path.
    // Check new node added in the path.
    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 0.0f));
    ASSERT_EQ(p.m_nodes.size(), 1u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(p.m_nodes[0]->getId(), 0u);
    ASSERT_EQ(p.m_segments.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 1u);
    ASSERT_EQ(p.m_nextSegmentId, 0u);

    // Add 2nd node on the path.
    // Check new node added in the path.
    Node& n2 = p.addNode(Vector3f(2.0f, 2.0f, 0.0f));
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(p.m_nodes[0]->getId(), 0u);
    ASSERT_EQ(p.m_nodes[1]->getId(), 1u);
    ASSERT_EQ(p.m_segments.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextSegmentId, 0u);

    // Add 1st segment on the path.
    // Check new segment added in the path.
    SegmentType type2("Dirt", 0xAAAAAA);
    Segment& s1 = p.addSegment(type2, n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_nodes[0]->getId(), 0u);
    ASSERT_EQ(p.m_nodes[1]->getId(), 1u);
    ASSERT_EQ(p.m_segments.size(), 1u);
    ASSERT_EQ(&*(p.m_segments[0]), &s1);
    ASSERT_EQ(p.m_segments[0]->getId(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextSegmentId, 1u);

    // Add 2nd segment on the 1st path.
    // Check new segment added in the path.
    // FIXME Replace the segment or allow multi-graph (== speedway) ?
    Segment& s2 = p.addSegment(type2, n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_nodes[0]->getId(), 0u);
    ASSERT_EQ(p.m_nodes[1]->getId(), 1u);
    ASSERT_EQ(p.m_segments.size(), 2u);
    ASSERT_EQ(&*(p.m_segments[0]), &s1);
    ASSERT_EQ(&*(p.m_segments[1]), &s2);
    ASSERT_EQ(p.m_segments[0]->getId(), 0u);
    ASSERT_EQ(p.m_segments[1]->getId(), 1u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextSegmentId, 2u);
}

TEST(TestsPath, SplitSegment)
{
    PathType type1("route");
    Path p(type1);

    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(1.0f, 3.0f, 0.0f));
    SegmentType type2("Dirt", 0xAAAAAA);
    Segment& s1 = p.addSegment(type2, n1, n2);
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
    ASSERT_EQ(n5.getPosition().x, 1.0f);
    ASSERT_EQ(n5.getPosition().y, 2.0f);
    ASSERT_EQ(n5.getPosition().z, 0.0f);
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_segments.size(), 2u);
}

TEST(TestsPath, MoveNode)
{
    PathType type1("route");
    Path p(type1);

    // Three nodes on the same position
    Node& n1 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n3 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));

    // Create two Segments in which Node1 is the extremity
    SegmentType type2("Dirt", 0xAAAAAA);
    Segment& s1 = p.addSegment(type2, n1, n2);
    Segment& s2 = p.addSegment(type2, n1, n3);

    // Check segments have dummy size.
    ASSERT_EQ(s1.getLength(), 0.0f);
    ASSERT_EQ(s2.getLength(), 0.0f);

    // Move nodes
    n2.translate(Vector3f(1.0f, 1.0f, 0.0f));
    n3.translate(Vector3f(-1.0f, -1.0f, 0.0f));

    // Check the segments updated their length.
    ASSERT_EQ(s1.getLength(), std::sqrt(2.0f));
    ASSERT_EQ(s2.getLength(), std::sqrt(2.0f));
}

TEST(TestsPath, RemoveSegmentDetachesItsExtremities)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    Node& n1 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& n3 = p.addNode(Vector3f(10.0f, 10.0f, 0.0f));

    Segment& s1 = p.addSegment(type2, n1, n2);
    p.addSegment(type2, n2, n3);

    uint32_t const id = s1.getId();
    ASSERT_EQ(n1.getSegments().size(), 1u);
    ASSERT_EQ(n2.getSegments().size(), 2u);

    p.removeSegment(s1);

    // The segment is gone from the graph and from both of its extremities,
    // which are kept even when they become orphan.
    ASSERT_EQ(p.getSegments().size(), 1u);
    ASSERT_EQ(p.getNodes().size(), 3u);
    ASSERT_EQ(p.findSegment(id), nullptr);
    ASSERT_EQ(n1.getSegments().size(), 0u);
    ASSERT_EQ(n2.getSegments().size(), 1u);
    ASSERT_EQ(n1.hasSegments(), false);
}

TEST(TestsPath, RemoveNodeTakesItsIncidentSegments)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    Node& n1 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& n3 = p.addNode(Vector3f(10.0f, 10.0f, 0.0f));

    p.addSegment(type2, n1, n2);
    p.addSegment(type2, n2, n3);
    p.addSegment(type2, n3, n1);

    uint32_t const id = n2.getId();
    p.removeNode(n2);

    ASSERT_EQ(p.getNodes().size(), 2u);
    ASSERT_EQ(p.findNode(id), nullptr);
    // Only the segment that avoided n2 is left.
    ASSERT_EQ(p.getSegments().size(), 1u);
    ASSERT_EQ(p.getNodes()[0]->getSegments().size(), 1u);
    ASSERT_EQ(p.getNodes()[1]->getSegments().size(), 1u);
}

TEST(TestsPath, RecreatedNodeKeepsItsIdentifier)
{
    PathType type1("route");
    Path p(type1);

    Node& n1 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    uint32_t const id = n2.getId();
    ASSERT_NE(n1.getId(), id);

    p.removeNode(n2);
    ASSERT_EQ(p.findNode(id), nullptr);

    // Undoing an edit gives the identifier back, which is what the commands
    // stacked above the removal refer to.
    Node& restored = p.addNode(id, Vector3f(10.0f, 0.0f, 0.0f));
    ASSERT_EQ(restored.getId(), id);
    ASSERT_EQ(p.findNode(id), &restored);

    // And the identifiers handed out afterwards do not collide with it.
    Node& n3 = p.addNode(Vector3f(20.0f, 0.0f, 0.0f));
    ASSERT_NE(n3.getId(), id);
    ASSERT_NE(n3.getId(), n1.getId());
}

TEST(TestsPath, FindCrossingsReportsWhereTheLineCutsTheGraph)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    // An horizontal street from (0,10) to (20,10).
    Node& west = p.addNode(Vector3f(0.0f, 10.0f, 0.0f));
    Node& east = p.addNode(Vector3f(20.0f, 10.0f, 0.0f));
    Segment& street = p.addSegment(type2, west, east);

    // A vertical line drawn a quarter of the way along it.
    auto crossings = p.findCrossings(Vector3f(5.0f, 0.0f, 0.0f),
                                     Vector3f(5.0f, 20.0f, 0.0f));

    ASSERT_EQ(crossings.size(), 1u);
    ASSERT_EQ(crossings[0].segment, &street);
    ASSERT_FLOAT_EQ(crossings[0].segmentOffset, 0.25f);
    ASSERT_FLOAT_EQ(crossings[0].lineOffset, 0.5f);
}

TEST(TestsPath, FindCrossingsSortsThemAlongTheLine)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    // Two horizontal streets, the farther one declared first, so that the
    // order of the answer cannot come from the order of the graph.
    Node& a = p.addNode(Vector3f(0.0f, 30.0f, 0.0f));
    Node& b = p.addNode(Vector3f(20.0f, 30.0f, 0.0f));
    Segment& far = p.addSegment(type2, a, b);

    Node& c = p.addNode(Vector3f(0.0f, 10.0f, 0.0f));
    Node& d = p.addNode(Vector3f(20.0f, 10.0f, 0.0f));
    Segment& near = p.addSegment(type2, c, d);

    auto crossings = p.findCrossings(Vector3f(5.0f, 0.0f, 0.0f),
                                     Vector3f(5.0f, 40.0f, 0.0f));

    ASSERT_EQ(crossings.size(), 2u);
    ASSERT_EQ(crossings[0].segment, &near);
    ASSERT_EQ(crossings[1].segment, &far);
    ASSERT_LT(crossings[0].lineOffset, crossings[1].lineOffset);
}

TEST(TestsPath, FindCrossingsIgnoresWhatTheLineDoesNotReach)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    Node& west = p.addNode(Vector3f(0.0f, 10.0f, 0.0f));
    Node& east = p.addNode(Vector3f(20.0f, 10.0f, 0.0f));
    p.addSegment(type2, west, east);

    // Stopping short of the street.
    ASSERT_EQ(p.findCrossings(Vector3f(5.0f, 0.0f, 0.0f),
                              Vector3f(5.0f, 9.0f, 0.0f)).size(), 0u);

    // Beside it.
    ASSERT_EQ(p.findCrossings(Vector3f(25.0f, 0.0f, 0.0f),
                              Vector3f(25.0f, 20.0f, 0.0f)).size(), 0u);

    // Along it: two streets laid over one another meet everywhere, so there is
    // no one point to make a junction of.
    ASSERT_EQ(p.findCrossings(Vector3f(0.0f, 10.0f, 0.0f),
                              Vector3f(20.0f, 10.0f, 0.0f)).size(), 0u);
}

TEST(TestsPath, FindCrossingsPutsAMeetingOnANodeOnThatNode)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    Node& west = p.addNode(Vector3f(0.0f, 10.0f, 0.0f));
    Node& east = p.addNode(Vector3f(20.0f, 10.0f, 0.0f));
    p.addSegment(type2, west, east);

    // A line running through the eastern end. Cutting a hair short of it would
    // leave a segment of no length beside a crossroads that is already there.
    auto crossings = p.findCrossings(Vector3f(20.0f, 0.0f, 0.0f),
                                     Vector3f(20.0f, 20.0f, 0.0f));

    ASSERT_EQ(crossings.size(), 1u);
    ASSERT_EQ(crossings[0].segmentOffset, 1.0f);
}

TEST(TestsPath, FindCrossingsLeavesTheEndsOfTheLineAlone)
{
    PathType type1("route");
    Path p(type1);
    SegmentType type2("Dirt", 0xAAAAAA);

    Node& west = p.addNode(Vector3f(0.0f, 10.0f, 0.0f));
    Node& east = p.addNode(Vector3f(20.0f, 10.0f, 0.0f));
    p.addSegment(type2, west, east);

    // A line that starts on the street and leaves it: whoever draws it is
    // putting a node on that spot anyway.
    ASSERT_EQ(p.findCrossings(Vector3f(5.0f, 10.0f, 0.0f),
                              Vector3f(5.0f, 30.0f, 0.0f)).size(), 0u);

    // And one that ends on it.
    ASSERT_EQ(p.findCrossings(Vector3f(5.0f, 30.0f, 0.0f),
                              Vector3f(5.0f, 10.0f, 0.0f)).size(), 0u);
}

TEST(TestsPath, FindCrossingsObeysTheNetworkType)
{
    // A network whose lines pass over one another without meeting: a water
    // main under a power line is not a junction anybody can turn at.
    PathType type1("pipes");
    type1.crossings = false;
    Path p(type1);
    SegmentType type2("Duct", 0xAAAAAA);

    Node& west = p.addNode(Vector3f(0.0f, 10.0f, 0.0f));
    Node& east = p.addNode(Vector3f(20.0f, 10.0f, 0.0f));
    p.addSegment(type2, west, east);

    ASSERT_EQ(p.findCrossings(Vector3f(5.0f, 0.0f, 0.0f),
                              Vector3f(5.0f, 20.0f, 0.0f)).size(), 0u);
}

TEST(TestsPath, RemoveSegmentLowersTheMaxFreeFlowSpeed)
{
    PathType type1("route");
    Path p(type1);

    SegmentType slow("Dirt", 0xAAAAAA);
    slow.speed = 10.0f;
    SegmentType fast("Highway", 0xBBBBBB);
    fast.speed = 100.0f;

    Node& n1 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& n3 = p.addNode(Vector3f(20.0f, 0.0f, 0.0f));

    p.addSegment(slow, n1, n2);
    Segment& highway = p.addSegment(fast, n2, n3);
    ASSERT_EQ(p.getMaxFreeFlowSpeed(), 100.0f);

    // Demolishing the fastest segment has to bring the cache back down: a
    // router turning a distance into a lower bound of a travel time divides by
    // this speed, and a stale value would make that bound unsound.
    p.removeSegment(highway);
    ASSERT_EQ(p.getMaxFreeFlowSpeed(), 10.0f);
}
