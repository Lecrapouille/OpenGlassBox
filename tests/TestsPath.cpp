#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/City.hpp"
#  include "src/Core/Unit.hpp"
#undef protected
#undef private

TEST(TestsNode, Constructor)
{
    // Construct a dummy node (not knowing segments or units)
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));

    // Check initial values (member variables).
    ASSERT_EQ(n.m_id, 42u);
    ASSERT_EQ(int32_t(n.m_position.x), 1);
    ASSERT_EQ(int32_t(n.m_position.y), 2);
    ASSERT_EQ(int32_t(n.m_position.z), 3);
    ASSERT_EQ(n.m_ways.size(), 0u);
    ASSERT_EQ(n.m_units.size(), 0u);

    // Check initial values (getter methods).
    ASSERT_EQ(n.id(), 42u);
    ASSERT_EQ(int32_t(n.position().x), 1);
    ASSERT_EQ(int32_t(n.position().y), 2);
    ASSERT_EQ(int32_t(n.position().z), 3);
    ASSERT_EQ(n.ways().size(), 0u);
    ASSERT_EQ(n.units().size(), 0u);
}

// Test adding Units on a Node
TEST(TestsNode, AddUnit)
{
    // Create two Nodes. Check no units are attached.
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Node n2(43u, Vector3f(2.0f, 3.0f, 4.0f));
    ASSERT_EQ(n1.units().size(), 0u);
    ASSERT_EQ(n2.units().size(), 0u);

    // Create an Unit "house" holding resources "people" attached to Node1.
    City city("Paris", 1u, 1u);
    UnitType unit_type("house");
    unit_type.color = 0xFF00FF;
    unit_type.radius = 2u;
    unit_type.resources.setCapacity("people", 10);
    unit_type.resources.addResource("people", 10);
    Unit u1(unit_type, n1, city);

    // Check one Unit has been added knowing Node1.
    ASSERT_EQ(n1.units().size(), 1u);
    ASSERT_EQ(n1.m_units[0], &u1);
    ASSERT_EQ(n1.units()[0], &u1);
    ASSERT_EQ(&(n1.unit(0)), &u1);
    ASSERT_EQ(&(n1.unit(0).m_node), &n1);
    ASSERT_STREQ(n1.unit(0).type().c_str(), "house");

    // Add Unit1 to Node2. Check if the Unit has been attached.
    n2.addUnit(u1);
    ASSERT_EQ(n2.units().size(), 1u);
    ASSERT_EQ(n2.m_units[0], &u1);
    ASSERT_EQ(n2.units()[0], &u1);
    ASSERT_EQ(&(n2.m_units[0]->m_node), &n1);
    ASSERT_STREQ(n2.m_units[0]->name().c_str(), "house");

    // Add Unit2 to Node1. Check if the Unit has been attached.
    Unit u2(unit_type, n2, city);
    n1.addUnit(u2);
    ASSERT_EQ(n1.m_units.size(), 2u);
    ASSERT_EQ(n1.m_units[0], &u1);
    ASSERT_EQ(n1.m_units[1], &u2);
    ASSERT_EQ(n1.units()[0], &u1);
    ASSERT_EQ(n1.units()[1], &u2);
    ASSERT_STREQ(n1.m_units[0]->name().c_str(), "house");
    ASSERT_STREQ(n1.m_units[1]->name().c_str(), "house");
    ASSERT_EQ(&(n1.m_units[0]->m_node), &n1);
    ASSERT_EQ(&(n1.m_units[1]->m_node), &n2);
}

TEST(TestsWay, Constuctor)
{
    // Create two Nodes.
    Node n1(42u, Vector3f(1.0f, 1.0f, 0.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 0.0f));

    // Create a segment linking the two Nodes.
    WayType type("Dirt", 0xAAAAAA);
    Way s1(55u, type, n1, n2);

    // Check if nodes are correctly hold by the Way
    ASSERT_EQ(s1.id(), 55u);
    ASSERT_STREQ(s1.type().c_str(), "Dirt");
    ASSERT_EQ(s1.color(), 0xAAAAAA);
    ASSERT_EQ(s1.m_from, &n1);
    ASSERT_EQ(s1.m_to, &n2);
    ASSERT_EQ(&s1.from(), &n1);
    ASSERT_EQ(&s1.to(), &n2);
    ASSERT_EQ(s1.length(), std::sqrt(2.0f));
}

// Test changeNode2 without using Path
TEST(TestsWay, changeNode2)
{
    Node n1(42u, Vector3f(1.0f, 1.0f, 1.0f));
    Node n2(43u, Vector3f(2.0f, 2.0f, 2.0f));
    Node n3(43u, Vector3f(3.0f, 3.0f, 3.0f));

    WayType type("Dirt", 0xAAAAAA);
    Way s1(55u, type, n1, n2);

    // Check the graph is the following:
    //     s1
    // |-------|       |
    // n1      n2      n3
    //
    ASSERT_EQ(&s1.to(), &n2);
    ASSERT_EQ(n1.m_ways.size(), 1u);
    ASSERT_EQ(n2.m_ways.size(), 1u);
    ASSERT_EQ(n3.m_ways.size(), 0u);
    ASSERT_EQ(&*(n1.m_ways[0]), &s1);
    ASSERT_EQ(&*(n2.m_ways[0]), &s1);

    // Change the Node2
    s1.changeNode2(n3);

    // Check the modified path is the follow:
    //         s1
    // |---------------|
    // n1      |      n3
    //         n2
    //
    ASSERT_NE(&n2, &n3);
    ASSERT_EQ(&s1.from(), &n1);
    ASSERT_EQ(&s1.to(), &n3);
    ASSERT_EQ(n1.m_ways.size(), 1u);
    ASSERT_EQ(n2.m_ways.size(), 0u);
    ASSERT_EQ(n3.m_ways.size(), 1u);
    ASSERT_EQ(&*(n1.m_ways[0]), &s1);
    ASSERT_EQ(&*(n3.m_ways[0]), &s1);

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

    // Check Ways of Node
    ASSERT_EQ(n1.getWayToNode(n1), nullptr);
    ASSERT_EQ(n1.getWayToNode(n2), nullptr);
    ASSERT_EQ(n1.getWayToNode(n3), &s1);
}

// Test changeNode2 using Path
TEST(TestsWay, PathchangeNode2)
{
    PathType type1("route");
    Path p(type1);
    ASSERT_STREQ(p.type().c_str(), "route");

    // Create Nodes
    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 1.0f));
    Node& n2 = p.addNode(Vector3f(2.0f, 2.0f, 2.0f));
    Node& n3 = p.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(&*(p.m_nodes[2]), &n3);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_nodes[2]->id(), 2u);
    ASSERT_EQ(p.m_nodes[0]->position().x, 1.0f);
    ASSERT_EQ(p.m_nodes[1]->position().x, 2.0f);
    ASSERT_EQ(p.m_nodes[2]->position().x, 3.0f);

    // Create a graph
    WayType type2("road");
    Way& s1 = p.addWay(type2, n1, n2);

    // Check the graph is the following:
    //     s1
    // |-------|       |
    // n1      n2      n3
    //
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_ways.size(), 1u);
    ASSERT_EQ(&s1.to(), &n2);
    ASSERT_EQ(n1.m_ways.size(), 1u);
    ASSERT_EQ(n2.m_ways.size(), 1u);
    ASSERT_EQ(&*(n2.m_ways[0]), &s1);
    ASSERT_EQ(n3.m_ways.size(), 0u);

    // Change the Node2
    s1.changeNode2(n3);

    // Check the modified path is the follow:
    //         s1
    // |---------------|
    // n1      |      n3
    //         n2
    //
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_ways.size(), 1u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(&*(p.m_nodes[2]), &n3);
    ASSERT_EQ(&s1.from(), &n1);
    ASSERT_EQ(&s1.to(), &n3);
    ASSERT_EQ(n1.m_ways.size(), 1u);
    ASSERT_EQ(n2.m_ways.size(), 0u);
    ASSERT_EQ(n3.m_ways.size(), 1u);
    ASSERT_EQ(&*(n1.m_ways[0]), &s1);
    ASSERT_EQ(&*(n3.m_ways[0]), &s1);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_nodes[2]->id(), 2u);
    ASSERT_EQ(p.m_nodes[0]->position().x, 1.0f);
    ASSERT_EQ(p.m_nodes[1]->position().x, 2.0f);
    ASSERT_EQ(p.m_nodes[2]->position().x, 3.0f);

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

    // Check Ways of Node
    ASSERT_EQ(n1.getWayToNode(n1), nullptr);
    ASSERT_EQ(n1.getWayToNode(n2), nullptr);
    ASSERT_EQ(n1.getWayToNode(n3), &s1);
}

TEST(TestsNode, getWayToNode)
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
    WayType type("road");
    Way s1(55u, type, n1, n2);
    Way s2(56u, type, n1, n3);

    // Check that n1 has two neighboring segments.
    ASSERT_EQ(n1.getWayToNode(n2), &s1);
    ASSERT_EQ(n1.getWayToNode(n3), &s2);

    // Check that n4 has no neighboring segments.
    ASSERT_EQ(n4.getWayToNode(n3), nullptr);

    // Check that n4 has no segment starting and leaving from it
    ASSERT_EQ(n4.getWayToNode(n4), nullptr);

    // Add a loop segment
    Way s3(57u, type, n4, n4);
    ASSERT_EQ(n4.getWayToNode(n4), &s3);

    // Check that n1 has no segment starting and leaving from it
    ASSERT_EQ(n1.getWayToNode(n1), nullptr);
}

TEST(TestsPath, Constructor)
{
    PathType type("route");
    Path p(type);

    ASSERT_STREQ(p.type().c_str(), "route");
    ASSERT_EQ(p.m_nodes.size(), 0u);
    ASSERT_EQ(p.m_ways.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 0u);
    ASSERT_EQ(p.m_nextWayId, 0u);
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
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_ways.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 1u);
    ASSERT_EQ(p.m_nextWayId, 0u);

    // Add 2nd node on the path.
    // Check new node added in the path.
    Node& n2 = p.addNode(Vector3f(2.0f, 2.0f, 0.0f));
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(&*(p.m_nodes[0]), &n1);
    ASSERT_EQ(&*(p.m_nodes[1]), &n2);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_ways.size(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextWayId, 0u);

    // Add 1st segment on the path.
    // Check new segment added in the path.
    WayType type2("Dirt", 0xAAAAAA);
    Way& s1 = p.addWay(type2, n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_ways.size(), 1u);
    ASSERT_EQ(&*(p.m_ways[0]), &s1);
    ASSERT_EQ(p.m_ways[0]->id(), 0u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextWayId, 1u);

    // Add 2nd segment on the 1st path.
    // Check new segment added in the path.
    // FIXME Replace the segment or allow multi-graph (== speedway) ?
    Way& s2 = p.addWay(type2, n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_nodes[0]->id(), 0u);
    ASSERT_EQ(p.m_nodes[1]->id(), 1u);
    ASSERT_EQ(p.m_ways.size(), 2u);
    ASSERT_EQ(&*(p.m_ways[0]), &s1);
    ASSERT_EQ(&*(p.m_ways[1]), &s2);
    ASSERT_EQ(p.m_ways[0]->id(), 0u);
    ASSERT_EQ(p.m_ways[1]->id(), 1u);
    ASSERT_EQ(p.m_nextNodeId, 2u);
    ASSERT_EQ(p.m_nextWayId, 2u);
}

TEST(TestsPath, SplitWay)
{
    PathType type1("route");
    Path p(type1);

    Node& n1 = p.addNode(Vector3f(1.0f, 1.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(1.0f, 3.0f, 0.0f));
    WayType type2("Dirt", 0xAAAAAA);
    Way& s1 = p.addWay(type2, n1, n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_ways.size(), 1u);

    // Split segment on the first node.
    // Check no new segment has been created.
    Node& n3 = p.splitWay(s1, 0.0f);
    ASSERT_EQ(&n3, &n1);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_ways.size(), 1u);

    // Split segment on the second node.
    // Check no new segment has been created.
    Node& n4 = p.splitWay(s1, 1.0f);
    ASSERT_EQ(&n4, &n2);
    ASSERT_EQ(p.m_nodes.size(), 2u);
    ASSERT_EQ(p.m_ways.size(), 1u);

    // Split segment on its middle.
    // Check a new segment and a new node have been created.
    Node& n5 = p.splitWay(s1, 0.5f);
    ASSERT_NE(&n5, &n1);
    ASSERT_NE(&n5, &n2);
    ASSERT_EQ(n5.position().x, 1.0f);
    ASSERT_EQ(n5.position().y, 2.0f);
    ASSERT_EQ(n5.position().z, 0.0f);
    ASSERT_EQ(p.m_nodes.size(), 3u);
    ASSERT_EQ(p.m_ways.size(), 2u);
}

TEST(TestsPath, MoveNode)
{
    PathType type1("route");
    Path p(type1);

    // Three nodes on the same position
    Node& n1 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n3 = p.addNode(Vector3f(0.0f, 0.0f, 0.0f));

    // Create two Ways in which Node1 is the extremity
    WayType type2("Dirt", 0xAAAAAA);
    Way& s1 = p.addWay(type2, n1, n2);
    Way& s2 = p.addWay(type2, n1, n3);

    // Check segments have dummy size.
    ASSERT_EQ(s1.length(), 0.0f);
    ASSERT_EQ(s2.length(), 0.0f);

    // Move nodes
    n2.translate(Vector3f(1.0f, 1.0f, 0.0f));
    n3.translate(Vector3f(-1.0f, -1.0f, 0.0f));

    // Check segments they length updated.
    ASSERT_EQ(s1.length(), std::sqrt(2.0f));
    ASSERT_EQ(s2.length(), std::sqrt(2.0f));
}
