#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/Core/City.hpp"
#  include "src/Core/Path.hpp"
#undef protected
#undef private

#  include "src/Config.hpp"

// -----------------------------------------------------------------------------
TEST(TestsCity, Constructors)
{
    // Constructor 1
    const uint32_t GRILL = 4u;
    City city("Paris", GRILL, GRILL + 1u);

    // Check initial values (member variables).
    ASSERT_STREQ(city.m_name.c_str(), "Paris");
    ASSERT_EQ(city.m_position.x, 0.0f);
    ASSERT_EQ(city.m_position.y, 0.0f);
    ASSERT_EQ(city.m_position.z, 0.0f);
    ASSERT_EQ(city.m_gridSizeU, GRILL);
    ASSERT_EQ(city.m_gridSizeV, GRILL + 1u);
    //FIXME: not used ASSERT_EQ(city.m_nextUnitId, 0u);
    ASSERT_EQ(city.m_nextAgentId, 0u);
    ASSERT_EQ(city.m_globals.m_bin.size(), 0u);
    ASSERT_EQ(city.m_maps.size(), 0u);
    ASSERT_EQ(city.m_paths.size(), 0u);
    ASSERT_EQ(city.m_units.size(), 0u);
    ASSERT_EQ(city.m_agents.size(), 0u);

    // Check initial values (getter methods).
    ASSERT_STREQ(city.name().c_str(), "Paris");
    ASSERT_EQ(city.position().x, 0.0f);
    ASSERT_EQ(city.position().y, 0.0f);
    ASSERT_EQ(city.position().z, 0.0f);
    ASSERT_EQ(city.gridSizeU(), GRILL);
    ASSERT_EQ(city.gridSizeV(), GRILL + 1u);
    ASSERT_EQ(city.globals().m_bin.size(), 0u);
    ASSERT_EQ(city.globals().isEmpty(), true);
    ASSERT_EQ(city.maps().size(), 0u);
    ASSERT_EQ(city.paths().size(), 0u);
    ASSERT_EQ(city.units().size(), 0u);
    ASSERT_EQ(city.agents().size(), 0u);

    // Constructor 3
    City city2("Marseille", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL);
    ASSERT_EQ(int32_t(city2.position().x), 1);
    ASSERT_EQ(int32_t(city2.position().y), 2);
    ASSERT_EQ(int32_t(city2.position().z), 3);
    ASSERT_EQ(city2.gridSizeU(), GRILL);
    ASSERT_EQ(city2.gridSizeV(), GRILL);

    // Constructor 3
    City city3("Lyon");
    ASSERT_EQ(city3.position().x, 0.0f);
    ASSERT_EQ(city3.position().y, 0.0f);
    ASSERT_EQ(city3.position().z, 0.0f);
    ASSERT_EQ(city3.gridSizeU(), 32u);
    ASSERT_EQ(city3.gridSizeV(), 32u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, GridPosition)
{
    uint32_t u, v;
    const uint32_t GRILL = 4u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL);

    // Lower bound of the City
    city.world2mapPosition(Vector3f(0.0f, 0.0f, 0.0f), u, v);
    ASSERT_EQ(u, 0u); ASSERT_EQ(v, 0u);

    // Upper bound of the City
    city.world2mapPosition(Vector3f(100.0f, 100.0f, 100.0f), u, v);
    ASSERT_EQ(u, GRILL - 1u); ASSERT_EQ(v, GRILL - 1u);

    // At the origin of the City
    city.world2mapPosition(Vector3f(1.0f, 2.0f, 3.0f), u, v);
    ASSERT_EQ(u, 0u); ASSERT_EQ(v, 0u);

    // 1 cell from the origin for each axis
    city.world2mapPosition(Vector3f(1.0f + config::GRID_SIZE,
                                    2.0f + config::GRID_SIZE,
                                    3.0f), u, v);
    ASSERT_EQ(u, 1u); ASSERT_EQ(v, 1u);

    // A little shift from previous test: still in the same cell
    city.world2mapPosition(Vector3f(1.0f + config::GRID_SIZE + 0.5f,
                                    2.0f + config::GRID_SIZE + 0.5f,
                                    3.0f), u, v);
    ASSERT_EQ(u, 1u); ASSERT_EQ(v, 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, BuildingCity)
{
    const uint32_t GRILL = 4u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL);

    // Add Map1.
    Map& m1 = city.addMap(MapType("map1"));
    Map& m2 = city.getMap("map1");

    // Check initial values of the newly created Map
    ASSERT_EQ(&m1, &m2);
    ASSERT_STREQ(m1.type().c_str(), "map1");
    ASSERT_EQ(m1.position().x, city.position().x);
    ASSERT_EQ(m1.position().y, city.position().y);
    ASSERT_EQ(m1.position().z, city.position().z);
    ASSERT_EQ(m1.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(m1.color(), 0xFFFFFF);

    // Add Map2.
    Map& m3 = city.addMap(MapType("map2", 0x00, 10u));
    Map& m4 = city.getMap("map2");

    // Check initial values of the newly created Map
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m3, &m4);
    ASSERT_STREQ(m4.type().c_str(), "map2");
    ASSERT_EQ(m4.position().x, city.position().x);
    ASSERT_EQ(m4.position().y, city.position().y);
    ASSERT_EQ(m4.position().z, city.position().z);
    ASSERT_EQ(m4.getCapacity(), 10u);
    ASSERT_EQ(m4.color(), 0x00);

    // Add again Map2. Check previous map has been replaced
    Map& m5 = city.addMap(MapType("map2"));
    Map& m6 = city.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m5, &m6);
    ASSERT_NE(&m6, &m4);
    // No longer cap 10 and no longer black color
    ASSERT_EQ(m6.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(m6.color(), 0xFFFFFF);

    // Add a Path
    Path& p1 = city.addPath(PathType("path1"));
    Path& p2 = city.getPath("path1");
    ASSERT_EQ(&p1, &p2);

    // Check initial values of the newly created Path
    ASSERT_STREQ(p2.type().c_str(), "path1");
    ASSERT_EQ(p2.m_type.color, 0xFFFFFF);
    ASSERT_EQ(p2.nodes().size(), 0u);
    ASSERT_EQ(p2.ways().size(), 0u);
    ASSERT_EQ(p2.m_nextNodeId, 0u);
    ASSERT_EQ(p2.m_nextWayId, 0u);

    // Replace the Path
    Path& p3 = city.addPath(PathType("path1", 0xAA));
    Path& p4 = city.getPath("path1");

    // Check previous map has been replaced
    ASSERT_EQ(&p3, &p4);
    ASSERT_NE(&p3, &p1);
    ASSERT_NE(&p4, &p2);
    ASSERT_STREQ(p4.type().c_str(), "path1");
    ASSERT_EQ(p4.m_type.color, 0xAA);

    // Add units (way 1)
    UnitType type5("unit1");
    type5.color = 0xFF00FF;
    type5.radius = 2u;
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit& u1 = city.addUnit(type5, n1);
    ASSERT_EQ(city.units().size(), 1u);
    Unit& u2 = *(city.units()[0]);
    ASSERT_EQ(&u1, &u2);
    ASSERT_EQ(u2.color(), 0xFF00FF);

    // Add agent
    AgentType t("Worker", 1.0f, 2u, 0xFFFFFF);
    Agent& a1 = city.addAgent(t, u2, Resources(), "???");
    Agent& a2 = *(city.agents()[0]);
    ASSERT_EQ(&a1, &a2);
    ASSERT_STREQ(a1.m_type.name.c_str(), "Worker");
    ASSERT_EQ(a1.m_type.speed, t.speed);
    ASSERT_EQ(a1.m_type.color, t.color);
    ASSERT_EQ(a1.m_type.radius, t.radius);
    //ASSERT_EQ(&(a1.m_owner), &u1);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, AddUnitSplitRoad)
{
    City city("Paris");
    Path& p1 = city.addPath(PathType("Road"));
    Node& n1 = p1.addNode(Vector3f(0.0f, 0.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(2.0f, 0.0f, 3.0f));
    Way& w1 = p1.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);

    // Check number of nodes
    ASSERT_EQ(p1.nodes().size(), 2u);
    ASSERT_EQ(p1.ways().size(), 1u);

    // Add Unit splitting the way into two ways and add a new nodes
    Unit& u1 = city.addUnit(UnitType("unit"), p1, w1, 0.5f);
    ASSERT_EQ(p1.nodes().size(), 3u);
    ASSERT_EQ(p1.ways().size(), 2u);

    // Newly added Node
    ASSERT_EQ(p1.nodes()[2]->m_id, 2u);
    ASSERT_EQ(int32_t(p1.nodes()[2]->position().x), 1);
    ASSERT_EQ(int32_t(p1.nodes()[2]->position().y), 0);
    ASSERT_EQ(int32_t(p1.nodes()[2]->position().z), 3);

    // Newly added Way
    ASSERT_EQ(p1.ways()[1]->m_id, 1u);
    ASSERT_EQ(int32_t(p1.ways()[0]->position1().x), 0); // Node0
    ASSERT_EQ(int32_t(p1.ways()[0]->position1().y), 0);
    ASSERT_EQ(int32_t(p1.ways()[0]->position1().z), 3);
    ASSERT_EQ(int32_t(p1.ways()[0]->position2().x), 1); // New Node
    ASSERT_EQ(int32_t(p1.ways()[0]->position2().y), 0);
    ASSERT_EQ(int32_t(p1.ways()[0]->position2().z), 3);
    ASSERT_EQ(int32_t(p1.ways()[1]->position1().x), 1); // New Node
    ASSERT_EQ(int32_t(p1.ways()[1]->position1().y), 0);
    ASSERT_EQ(int32_t(p1.ways()[1]->position1().z), 3);
    ASSERT_EQ(int32_t(p1.ways()[1]->position2().x), 2); // Node1
    ASSERT_EQ(int32_t(p1.ways()[1]->position2().y), 0);
    ASSERT_EQ(int32_t(p1.ways()[1]->position2().z), 3);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, translate)
{
    City city("Paris");

    Map& m1 = city.addMap(MapType("water"));
    Path& p1 = city.addPath(PathType("Road"));
    Node& n1 = p1.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    Way& w1 = p1.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);
    Unit& u1 = city.addUnit(UnitType("unit1"), n1);
    Agent& a1 = city.addAgent(AgentType("Worker", 1.0f, 2u,
         0xFFFFFF), u1, Resources(), "target");

    // Displace the City
    city.translate(Vector3f(1.0f, 1.0f, 1.0f));
    city.translate(Vector3f(0.0f, 1.0f, -1.0f));

    // Check if all elements have been translated
    ASSERT_EQ(int32_t(city.m_position.x), 1);
    ASSERT_EQ(int32_t(city.m_position.y), 2);
    ASSERT_EQ(int32_t(city.m_position.z), 0);

    ASSERT_EQ(int32_t(m1.m_position.x), 1);
    ASSERT_EQ(int32_t(m1.m_position.y), 2);
    ASSERT_EQ(int32_t(m1.m_position.z), 0);

    ASSERT_EQ(int32_t(n1.m_position.x), 1+1);
    ASSERT_EQ(int32_t(n1.m_position.y), 2+2);
    ASSERT_EQ(int32_t(n1.m_position.z), 3+0);

    ASSERT_EQ(int32_t(n2.m_position.x), 3+1);
    ASSERT_EQ(int32_t(n2.m_position.y), 3+2);
    ASSERT_EQ(int32_t(n2.m_position.z), 3+0);

    // Position of the Unit == Node1
    ASSERT_EQ(int32_t(n1.m_position.x), 1+1);
    ASSERT_EQ(int32_t(n1.m_position.y), 2+2);
    ASSERT_EQ(int32_t(n1.m_position.z), 3+0);

    // Position of the Way1 == Node1 and Node2
    ASSERT_EQ(int32_t(w1.position1().x), 1+1);
    ASSERT_EQ(int32_t(w1.position1().y), 2+2);
    ASSERT_EQ(int32_t(w1.position1().z), 3+0);
    ASSERT_EQ(int32_t(w1.position2().x), 3+1);
    ASSERT_EQ(int32_t(w1.position2().y), 3+2);
    ASSERT_EQ(int32_t(w1.position2().z), 3+0);

    // Position of the Agent == Node1
    ASSERT_EQ(int32_t(a1.m_position.x), 1+1);
    ASSERT_EQ(int32_t(a1.m_position.y), 2+2);
    ASSERT_EQ(int32_t(a1.m_position.z), 3+0);
}

// -----------------------------------------------------------------------------
// For testing City::update()
class MockMap: public Map
{
public:

    MockMap(std::string const& name, City& city)
        : Map(MapType(name), city)
    {}
    MOCK_METHOD(void, executeRules, (), (override));
};

class MockUnit: public Unit
{
public:

    MockUnit(Node& n, City& city)
        : Unit(UnitType("unit"), n, city)
    {}
    MOCK_METHOD(void, executeRules, (), (override));
};

class MockAgent: public Agent
{
public:

    MockAgent(uint32_t id, AgentType const& type, Unit& owner, Resources const& resources,
              std::string const& searchTarget)
        : Agent(id, type, owner, resources, searchTarget)
    {}
    MOCK_METHOD(bool, update, (Dijkstra&), (override));
};

// -----------------------------------------------------------------------------
TEST(TestsCity, update)
{
    City city("Paris");
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));

    // Add two maps
    city.m_maps["map1"] = std::make_unique<MockMap>("map1", city);
    city.m_maps["map2"] = std::make_unique<MockMap>("map2", city);

    // Add two Units
    city.m_units.push_back(std::make_unique<MockUnit>(n1, city));
    city.m_units.push_back(std::make_unique<MockUnit>(n1, city));

    // Add two agents
    city.m_agents.push_back(std::make_unique<MockAgent>(
       0u, AgentType("Worker", 1.0f, 2u, 0xFFFFFF), *(city.m_units[0]), Resources(), "target")
    );
    city.m_agents.push_back(std::make_unique<MockAgent>(
       1u, AgentType("Worker", 1.0f, 2u, 0xFFFFFF), *(city.m_units[0]), Resources(), "target")
    );

    // Each Map will call executeRules() once
    EXPECT_CALL(*(static_cast<MockMap*>(city.m_maps["map1"].get())),
                executeRules()).Times(1);
    EXPECT_CALL(*(static_cast<MockMap*>(city.m_maps["map2"].get())),
                executeRules()).Times(1);

    // Each Unit will call executeRules() once
    EXPECT_CALL(*(static_cast<MockUnit*>(city.m_units[0].get())),
                executeRules()).Times(1);
    EXPECT_CALL(*(static_cast<MockUnit*>(city.m_units[1].get())),
                executeRules()).Times(1);

    // Each Agent will call update() once
    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[0].get())),
                update(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[1].get())),
                update(_)).Times(1).WillOnce(Return(false));
    city.update();
}

// -----------------------------------------------------------------------------
TEST(TestsCity, updateRemoveAgent)
{
    City city("Paris");
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit u1(UnitType("foo"), n1, city);

    // Add two agents
    city.m_agents.push_back(std::make_unique<MockAgent>(
       0u, AgentType("Worker", 1.0f, 2u, 0xFFFFFF), u1, Resources(), "target")
    );
    city.m_agents.push_back(std::make_unique<MockAgent>(
       1u, AgentType("Worker", 1.0f, 2u, 0xFFFFFF), u1, Resources(), "target")
    );
    city.m_agents.push_back(std::make_unique<MockAgent>(
       2u, AgentType("Worker", 1.0f, 2u, 0xFFFFFF), u1, Resources(), "target")
    );

    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[2].get())),
                update(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[1].get())),
                update(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[0].get())),
                update(_)).Times(1).WillOnce(Return(false));
    city.update();

    ASSERT_EQ(city.m_agents.size(), 2u);
    ASSERT_EQ(city.m_agents[0]->m_id, 0u);
    ASSERT_EQ(city.m_agents[1]->m_id, 1u);

    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[1].get())),
                update(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*(static_cast<MockAgent*>(city.m_agents[0].get())),
                update(_)).Times(1).WillOnce(Return(true));
    city.update();

    ASSERT_EQ(city.m_agents.size(), 1u);
    ASSERT_EQ(city.m_agents[0]->m_id, 1u);
}
