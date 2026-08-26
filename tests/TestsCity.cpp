#include "main.hpp"
#include <iostream>
#include <memory>

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/Path.hpp"
#  include "OpenGlassBox/RuleCommand.hpp"
#  include "OpenGlassBox/RuleValue.hpp"
#undef protected
#undef private

#include "OpenGlassBox/Config.hpp"

// -----------------------------------------------------------------------------
TEST(TestsCity, Constructors)
{
    // Constructor 1
    const uint32_t GRILL = 4u;
    TestWorld cityWorld("Paris", GRILL, GRILL + 1u);
    City& city = cityWorld.city;
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
    ASSERT_EQ(city.maps().size(), 0u);
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
    TestWorld city2World("Marseille", GRILL, GRILL, Vector3f(1.0f, 2.0f, 3.0f));
    City& city2 = city2World.city;
    ASSERT_EQ(int32_t(city2.position().x), 1);
    ASSERT_EQ(int32_t(city2.position().y), 2);
    ASSERT_EQ(int32_t(city2.position().z), 3);
    ASSERT_EQ(city2.gridSizeU(), GRILL);
    ASSERT_EQ(city2.gridSizeV(), GRILL);

    // Constructor 3
    TestWorld city3World("Lyon");
    City& city3 = city3World.city;
    ASSERT_EQ(city3.position().x, 0.0f);
    ASSERT_EQ(city3.position().y, 0.0f);
    ASSERT_EQ(city3.position().z, 0.0f);
    ASSERT_EQ(city3.gridSizeU(), 32u);
    ASSERT_EQ(city3.gridSizeV(), 32u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, GridPosition)
{
    int32_t u, v;
    const uint32_t GRILL = 4u;
    TestWorld cityWorld("Paris", GRILL, GRILL, Vector3f(1.0f, 2.0f, 3.0f));
    City& city = cityWorld.city;
    // Lower bound of the City
    city.world2mapPosition(Vector3f(0.0f, 0.0f, 0.0f), u, v);
    ASSERT_EQ(u, 0); ASSERT_EQ(v, 0);

    // Upper bound of the City
    city.world2mapPosition(Vector3f(100.0f, 100.0f, 100.0f), u, v);
    ASSERT_EQ(u, GRILL - 1u); ASSERT_EQ(v, GRILL - 1u);

    // At the origin of the City
    city.world2mapPosition(Vector3f(1.0f, 2.0f, 3.0f), u, v);
    ASSERT_EQ(u, 0u); ASSERT_EQ(v, 0u);

    // 1 cell from the origin for each axis
    city.world2mapPosition(Vector3f(1.0f + city.gridCellSize(),
                                    2.0f + city.gridCellSize(),
                                    3.0f), u, v);
    ASSERT_EQ(u, 1u); ASSERT_EQ(v, 1u);

    // A little shift from previous test: still in the same cell
    city.world2mapPosition(Vector3f(1.0f + city.gridCellSize() + 0.5f,
                                    2.0f + city.gridCellSize() + 0.5f,
                                    3.0f), u, v);
    ASSERT_EQ(u, 1u); ASSERT_EQ(v, 1u);
}

// -----------------------------------------------------------------------------
#if 0 // FIXME: broken with newer google test
TEST(TestsCity, BuildingCity)
{
    const uint32_t GRILL = 4u;
    TestWorld cityWorld("Paris", GRILL, GRILL, Vector3f(1.0f, 2.0f, 3.0f));
    City& city = cityWorld.city;
    // Add Map1.
    Map& m1 = city.addMap(keep<MapType>("map1"));
    Map& m2 = city.getMap("map1");

    // Check initial values of the newly created Map
    ASSERT_EQ(&m1, &m2);
    ASSERT_STREQ(m1.type().c_str(), "map1");
    ASSERT_EQ(m1.position().x, city.position().x);
    ASSERT_EQ(m1.position().y, city.position().y);
    ASSERT_EQ(m1.position().z, city.position().z);
    ASSERT_EQ(m1.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(m1.color(), 0xFFFFFFu);

    // Add Map2.
    Map& m3 = city.addMap(keep<MapType>("map2", 0x00, 10u));
    Map& m4 = city.getMap("map2");

    // Check initial values of the newly created Map
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m3, &m4);
    ASSERT_STREQ(m4.type().c_str(), "map2");
    ASSERT_EQ(m4.position().x, city.position().x);
    ASSERT_EQ(m4.position().y, city.position().y);
    ASSERT_EQ(m4.position().z, city.position().z);
    ASSERT_EQ(m4.getCapacity(), 10u);
    ASSERT_EQ(m4.color(), 0x00u);

    // Add again Map2. Check previous map has been replaced
    Map& m5 = city.addMap(keep<MapType>("map2"));
    Map& m6 = city.getMap("map2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m5, &m6);
    ASSERT_NE(&m6, &m4);
    // No longer cap 10 and no longer black color
    ASSERT_EQ(m6.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(m6.color(), 0xFFFFFFu);

    // Add a Path
    Path& p1 = city.addPath(keep<PathType>("path1"));
    Path& p2 = city.getPath("path1");
    ASSERT_EQ(&p1, &p2);

    // Check initial values of the newly created Path
    ASSERT_STREQ(p2.type().c_str(), "path1");
    ASSERT_EQ(p2.m_type.color, 0xFFFFFFu);
    ASSERT_EQ(p2.nodes().size(), 0u);
    ASSERT_EQ(p2.ways().size(), 0u);
    ASSERT_EQ(p2.m_nextNodeId, 0u);
    ASSERT_EQ(p2.m_nextWayId, 0u);

    // Replace the Path
    Path& p3 = city.addPath(keep<PathType>("path1", 0xAA));
    Path& p4 = city.getPath("path1");

    // Check previous map has been replaced
    ASSERT_EQ(&p3, &p4);
    ASSERT_NE(&p3, &p1);
    ASSERT_NE(&p4, &p2);
    ASSERT_STREQ(p4.type().c_str(), "path1");
    ASSERT_EQ(p4.m_type.color, 0xAAu);

    // Add units (way 1)
    UnitType type5("unit1");
    type5.color = 0xFF00FF;
    type5.radius = 2u;
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit& u1 = city.addUnit(type5, n1);
    ASSERT_EQ(city.units().size(), 1u);
    Unit& u2 = *(city.units()[0]);
    ASSERT_EQ(&u1, &u2);
    ASSERT_EQ(u2.color(), 0xFF00FFu);

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
#endif

// -----------------------------------------------------------------------------
TEST(TestsCity, AddUnitOnWayDoesNotSplitRoad)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& p1 = city.addPath(keep<PathType>("Road"));
    Node& n1 = p1.addNode(Vector3f(0.0f, 0.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(2.0f, 0.0f, 3.0f));
    Way& w1 = p1.addWay(keep<WayType>("Dirt", 0xAAAAAA), n1, n2);

    ASSERT_EQ(p1.nodes().size(), 2u);
    ASSERT_EQ(p1.ways().size(), 1u);

    Unit& u1 = city.addUnit(keep<UnitType>("unit"), p1, w1, 0.5f);
    ASSERT_EQ(p1.nodes().size(), 2u);
    ASSERT_EQ(p1.ways().size(), 1u);
    ASSERT_EQ(u1.way(), &w1);
    ASSERT_EQ(u1.node(), nullptr);
    ASSERT_EQ(int32_t(u1.position().x), 1);
    ASSERT_EQ(w1.units().size(), 1u);
}

// -----------------------------------------------------------------------------
// Cutting a segment is how the Buildings tool turns a spot on a street into an
// address: the junction is a Node, and agents only ever stop at nodes.
TEST(TestsCity, SplitWayCutsTheSegmentInTwo)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Way& way = path.addWay(keep<WayType>("Dirt", 0xAAAAAA), n1, n2);

    Node& junction = city.splitWay(path, way, 0.5f);

    ASSERT_EQ(path.nodes().size(), 3u);
    ASSERT_EQ(path.ways().size(), 2u);
    ASSERT_EQ(int32_t(junction.position().x), 5);
    ASSERT_EQ(junction.ways().size(), 2u);
    // The first half was shortened rather than replaced.
    ASSERT_EQ(&way.from(), &n1);
    ASSERT_EQ(&way.to(), &junction);
    ASSERT_EQ(int32_t(way.magnitude()), 5);
}

// -----------------------------------------------------------------------------
// A building already standing along the street has to end up on the half that
// runs under it, otherwise it addresses a segment that stops short of it.
TEST(TestsCity, SplitWayKeepsTheBuildingsWhereTheyStand)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Way& way = path.addWay(keep<WayType>("Dirt", 0xAAAAAA), n1, n2);

    Unit& near = city.addUnit(keep<UnitType>("Home"), path, way, 0.2f);
    Unit& far = city.addUnit(keep<UnitType>("Home"), path, way, 0.8f);

    Node& junction = city.splitWay(path, way, 0.5f);

    Way* second = nullptr;
    for (Way* incident: junction.ways())
    {
        if (incident != &way)
            second = incident;
    }
    ASSERT_NE(second, nullptr);

    ASSERT_EQ(near.way(), &way);
    ASSERT_EQ(far.way(), second);
    ASSERT_EQ(way.units().size(), 1u);
    ASSERT_EQ(second->units().size(), 1u);
    ASSERT_EQ(int32_t(near.position().x + 0.5f), 2);
    ASSERT_EQ(int32_t(far.position().x + 0.5f), 8);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, SplitWayOnAnExtremityCutsNothing)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Way& way = path.addWay(keep<WayType>("Dirt", 0xAAAAAA), n1, n2);

    ASSERT_EQ(&city.splitWay(path, way, 0.0f), &n1);
    ASSERT_EQ(&city.splitWay(path, way, 1.0f), &n2);
    ASSERT_EQ(path.nodes().size(), 2u);
    ASSERT_EQ(path.ways().size(), 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, translate)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Map& m1 = city.addMap(keep<MapType>("water"));
    Path& p1 = city.addPath(keep<PathType>("Road"));
    Node& n1 = p1.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    Way& w1 = p1.addWay(keep<WayType>("Dirt", 0xAAAAAA), n1, n2);
    float const initialMagnitude = w1.magnitude();
    Unit& u1 = city.addUnit(keep<UnitType>("unit1"), n1);
    Agent& a1 = city.addAgent(keep<AgentType>("Worker", 1.0f, 2u,
         0xFFFFFF), u1, Resources(), "target");

    // Displace the City
    city.translate(Vector3f(1.0f, 1.0f, 1.0f));
    city.translate(Vector3f(0.0f, 1.0f, -1.0f));

    // Check if all elements have been translated
    ASSERT_EQ(int32_t(city.m_position.x), 1);
    ASSERT_EQ(int32_t(city.m_position.y), 2);
    ASSERT_EQ(int32_t(city.m_position.z), 0);

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
    ASSERT_FLOAT_EQ(w1.magnitude(), initialMagnitude);

    // Position of the Agent == Node1
    ASSERT_EQ(int32_t(a1.m_position.x), 1+1);
    ASSERT_EQ(int32_t(a1.m_position.y), 2+2);
    ASSERT_EQ(int32_t(a1.m_position.z), 3+0);
}

// -----------------------------------------------------------------------------
//! \brief One tick of the City is one tick of every one of its buildings: the
//! rules that fall due are attempted and their effect lands in the resources.
//!
//! This was written with a mock building overriding executeRules(), which is
//! what the VIRTUAL macro used to be for. Reading the outcome instead of
//! counting the calls also catches a rule that runs and does nothing.
TEST(TestsCity, UpdateRunsTheRulesOfEveryBuilding)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;

    // A rule due on every tick, adding one to a resource of the building.
    Resource people("People");
    RuleValueLocal local(people);
    RuleCommandAdd add(local, 1u);
    RuleUnitType ruleType("Fill");
    ruleType.rate = 1u;
    ruleType.commands.push_back(&add);
    RuleUnit rule(ruleType);

    UnitType type("Home");
    type.rules.push_back(&rule);
    type.resources.setCapacity("People", 10u);

    Unit& first = city.addUnit(type, Vector3f(1.0f, 1.0f, 0.0f));
    Unit& second = city.addUnit(type, Vector3f(3.0f, 1.0f, 0.0f));
    ASSERT_EQ(first.resources().getAmount("People"), 0u);
    ASSERT_EQ(second.resources().getAmount("People"), 0u);

    city.update();

    ASSERT_EQ(first.resources().getAmount("People"), 1u);
    ASSERT_EQ(second.resources().getAmount("People"), 1u);

    city.update();

    ASSERT_EQ(first.resources().getAmount("People"), 2u);
    ASSERT_EQ(second.resources().getAmount("People"), 2u);
}

// -----------------------------------------------------------------------------
//! \brief An Agent that has handed its load over is taken away, and the others
//! keep their identity and their order. An Agent with nothing to deliver is
//! done on the very first tick.
TEST(TestsCity, UpdateTakesAwayTheAgentsThatAreDone)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;

    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addWay(keep<WayType>("Dirt", 0xAAAAAA), n1, n2);

    UnitType homeType("Home");
    Unit& home = city.addUnit(homeType, n1);

    AgentType worker("Worker", 1.0f, 2u, 0xFFFFFF);
    Resources load;
    load.addResource("People", 1u);

    // Nothing in the city answers to "nowhere", so these two keep looking.
    Agent& looking0 = city.addAgent(worker, home, load, "nowhere");
    Agent& looking1 = city.addAgent(worker, home, load, "nowhere");
    // Nothing to deliver: done as soon as it is asked to drive.
    city.addAgent(worker, home, Resources(), "nowhere");

    uint32_t const id0 = looking0.id();
    uint32_t const id1 = looking1.id();
    ASSERT_EQ(city.agents().size(), 3u);

    city.update();

    ASSERT_EQ(city.agents().size(), 2u);
    ASSERT_EQ(city.agents()[0]->id(), id0);
    ASSERT_EQ(city.agents()[1]->id(), id1);

    // Empty the load of the first one and it too is done, the second one taking
    // its place without changing identity. Not on the very next tick: having
    // found nothing, it drove off towards a random crossroads, and an Agent on
    // its way somewhere does not stop to look around.
    city.agents()[0]->resources().removeResource("People", 1u);
    for (uint32_t tick = 0u; (tick < 600u) && (city.agents().size() > 1u); ++tick)
    {
        city.update();
    }

    ASSERT_EQ(city.agents().size(), 1u);
    ASSERT_EQ(city.agents()[0]->id(), id1);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemoveUnitDetachesItFromItsNode)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    WayType wayType("Dirt", 0xAAAAAA);
    UnitType unitType("Home");

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addWay(wayType, n1, n2);

    Unit& unit = city.addUnit(unitType, n1);
    ASSERT_EQ(city.units().size(), 1u);
    ASSERT_EQ(n1.units().size(), 1u);

    city.removeUnit(unit);

    ASSERT_EQ(city.units().size(), 0u);
    ASSERT_EQ(n1.units().size(), 0u);
    // The road it stood on is untouched.
    ASSERT_EQ(path.nodes().size(), 2u);
    ASSERT_EQ(path.ways().size(), 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemoveNodeTakesTheUnitsSittingOnIt)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    WayType wayType("Dirt", 0xAAAAAA);
    UnitType unitType("Home");

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addWay(wayType, n1, n2);

    city.addUnit(unitType, n1);
    city.addUnit(unitType, n2);
    ASSERT_EQ(city.units().size(), 2u);

    // Demolishing the node has to take the building with it, otherwise the Unit
    // would be left holding a reference to freed memory.
    city.removeNode(path, n1);

    ASSERT_EQ(city.units().size(), 1u);
    ASSERT_EQ(path.nodes().size(), 1u);
    ASSERT_EQ(path.ways().size(), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemovingTheLastWayTakesTheAgentsWithIt)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    WayType wayType("Dirt", 0xAAAAAA);
    UnitType unitType("Home");
    AgentType agentType("Worker", 10.0f, 1u, 0xFFFFFF);

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Way& way = path.addWay(wayType, n1, n2);

    Unit& unit = city.addUnit(unitType, n1);
    unit.resources().setCapacity("People", 10u);
    Resources resources;
    resources.addResource("People", 2u);
    city.addAgent(agentType, unit, resources, "Home");
    ASSERT_EQ(city.agents().size(), 1u);

    // n1 survives because it carries the building, but it no longer leads
    // anywhere: an Agent standing there could neither move nor deliver, so it
    // goes with the road rather than float over the map.
    city.removeWay(path, way);
    ASSERT_EQ(path.ways().size(), 0u);
    ASSERT_EQ(city.agents().size(), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemovingOneWayKeepsTheAgentsOnTheRest)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    WayType wayType("Dirt", 0xAAAAAA);
    UnitType unitType("Home");
    AgentType agentType("Worker", 10.0f, 1u, 0xFFFFFF);

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(20.0f, 0.0f, 0.0f));
    path.addWay(wayType, n1, n2);
    Way& far = path.addWay(wayType, n2, n3);

    Unit& unit = city.addUnit(unitType, n1);
    unit.resources().setCapacity("People", 10u);
    Resources resources;
    resources.addResource("People", 2u);
    city.addAgent(agentType, unit, resources, "Home");
    ASSERT_EQ(city.agents().size(), 1u);

    // The Agent waits on n1, which keeps a road: demolishing the far segment
    // only invalidates its itinerary.
    city.removeWay(path, far);
    ASSERT_EQ(path.ways().size(), 1u);
    ASSERT_EQ(city.agents().size(), 1u);
    ASSERT_EQ(city.agents()[0]->lastNode(), &n1);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemovingANodeLeavesNoAgentPointingAtIt)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    WayType wayType("Dirt", 0xAAAAAA);
    UnitType unitType("Home");
    AgentType agentType("Worker", 10.0f, 1u, 0xFFFFFF);

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addWay(wayType, n1, n2);

    Unit& unit = city.addUnit(unitType, n2);
    unit.resources().setCapacity("People", 10u);
    Resources resources;
    resources.addResource("People", 2u);
    city.addAgent(agentType, unit, resources, "Home");
    ASSERT_EQ(city.agents().size(), 1u);

    // The Agent was standing on n2. Demolishing it used to leave the Agent
    // holding a pointer on freed memory, and the next tick read through it.
    city.removeNode(path, n2);
    ASSERT_EQ(city.agents().size(), 0u);
    ASSERT_EQ(city.units().size(), 0u);
    ASSERT_EQ(path.nodes().size(), 0u);

    // Nothing dangles: a tick over the emptied city has to be harmless.
    city.update(1.0f);
}

// -----------------------------------------------------------------------------
//! \brief Clearing a City throws away what the player built, not the kinds of
//! network the ruleset declares: roads have to remain buildable afterwards.
// -----------------------------------------------------------------------------
TEST(TestsCity, ClearKeepsTheGraphsAndEmptiesThem)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    WayType wayType("Dirt", 0xAAAAAA);

    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addWay(wayType, n1, n2);
    city.addUnit(keep<UnitType>("Home"), n1);
    city.addArea(keep<AreaType>("Residential"), city.region());

    city.clear();

    ASSERT_EQ(city.units().size(), 0u);
    ASSERT_EQ(city.areas().size(), 0u);
    ASSERT_EQ(city.paths().size(), 1u);

    Path& road = city.getPath("Road");
    ASSERT_EQ(road.nodes().size(), 0u);
    ASSERT_EQ(road.ways().size(), 0u);

    // And the emptied graph takes new segments.
    Node& n3 = road.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n4 = road.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    road.addWay(wayType, n3, n4);
    ASSERT_EQ(road.ways().size(), 1u);
}
