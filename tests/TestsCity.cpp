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
    //FIXME: not used ASSERT_EQ(city.m_nextBuildingId, 0u);
    ASSERT_EQ(city.m_nextAgentId, 0u);
    ASSERT_EQ(city.m_globals.m_bin.size(), 0u);
    ASSERT_EQ(city.getLayers().size(), 0u);
    ASSERT_EQ(city.m_paths.size(), 0u);
    ASSERT_EQ(city.m_buildings.size(), 0u);
    ASSERT_EQ(city.m_agents.size(), 0u);

    // Check initial values (getter methods).
    ASSERT_STREQ(city.getName().c_str(), "Paris");
    ASSERT_EQ(city.getPosition().x, 0.0f);
    ASSERT_EQ(city.getPosition().y, 0.0f);
    ASSERT_EQ(city.getPosition().z, 0.0f);
    ASSERT_EQ(city.getRegion().sizeU, GRILL);
    ASSERT_EQ(city.getRegion().sizeV, GRILL + 1u);
    ASSERT_EQ(city.getGlobals().m_bin.size(), 0u);
    ASSERT_EQ(city.getGlobals().isEmpty(), true);
    ASSERT_EQ(city.getLayers().size(), 0u);
    ASSERT_EQ(city.getPaths().size(), 0u);
    ASSERT_EQ(city.getBuildings().size(), 0u);
    ASSERT_EQ(city.getAgents().size(), 0u);

    // Constructor 3
    TestWorld city2World("Marseille", GRILL, GRILL, Vector3f(1.0f, 2.0f, 3.0f));
    City& city2 = city2World.city;
    ASSERT_EQ(int32_t(city2.getPosition().x), 1);
    ASSERT_EQ(int32_t(city2.getPosition().y), 2);
    ASSERT_EQ(int32_t(city2.getPosition().z), 3);
    ASSERT_EQ(city2.getRegion().sizeU, GRILL);
    ASSERT_EQ(city2.getRegion().sizeV, GRILL);

    // Constructor 3
    TestWorld city3World("Lyon");
    City& city3 = city3World.city;
    ASSERT_EQ(city3.getPosition().x, 0.0f);
    ASSERT_EQ(city3.getPosition().y, 0.0f);
    ASSERT_EQ(city3.getPosition().z, 0.0f);
    ASSERT_EQ(city3.getRegion().sizeU, 32u);
    ASSERT_EQ(city3.getRegion().sizeV, 32u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, GridPosition)
{
    const uint32_t GRILL = 4u;
    TestWorld cityWorld("Paris", GRILL, GRILL, Vector3f(1.0f, 2.0f, 3.0f));
    City& city = cityWorld.city;

    // Lower bound of the City
    Cell cell = city.worldToCell({ 0.0f, 0.0f, 0.0f });
    ASSERT_EQ(cell.u, 0); ASSERT_EQ(cell.v, 0);

    // Upper bound of the City
    cell = city.worldToCell({ 100.0f, 100.0f, 100.0f });
    ASSERT_EQ(cell.u, int32_t(GRILL - 1u));
    ASSERT_EQ(cell.v, int32_t(GRILL - 1u));

    // At the origin of the City
    cell = city.worldToCell({ 1.0f, 2.0f, 3.0f });
    ASSERT_EQ(cell.u, 0); ASSERT_EQ(cell.v, 0);

    // 1 cell from the origin for each axis
    cell = city.worldToCell({ 1.0f + city.getCellSize(),
                              2.0f + city.getCellSize(),
                              3.0f });
    ASSERT_EQ(cell.u, 1); ASSERT_EQ(cell.v, 1);

    // A little shift from previous test: still in the same cell
    cell = city.worldToCell({ 1.0f + city.getCellSize() + 0.5f,
                              2.0f + city.getCellSize() + 0.5f,
                              3.0f });
    ASSERT_EQ(cell.u, 1); ASSERT_EQ(cell.v, 1);
}

// -----------------------------------------------------------------------------
#if 0 // FIXME: broken with newer google test
TEST(TestsCity, BuildingCity)
{
    const uint32_t GRILL = 4u;
    TestWorld cityWorld("Paris", GRILL, GRILL, Vector3f(1.0f, 2.0f, 3.0f));
    City& city = cityWorld.city;
    // Add Layer1.
    Layer& m1 = city.addLayer(keep<LayerType>("layer1"));
    Layer& m2 = city.getLayer("layer1");

    // Check initial values of the newly created Layer
    ASSERT_EQ(&m1, &m2);
    ASSERT_STREQ(m1.getTypeName().c_str(), "layer1");
    ASSERT_EQ(m1.getPosition().x, city.getPosition().x);
    ASSERT_EQ(m1.getPosition().y, city.getPosition().y);
    ASSERT_EQ(m1.getPosition().z, city.getPosition().z);
    ASSERT_EQ(m1.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(m1.getColor(), 0xFFFFFFu);

    // Add Layer2.
    Layer& m3 = city.addLayer(keep<LayerType>("layer2", 0x00, 10u));
    Layer& m4 = city.getLayer("layer2");

    // Check initial values of the newly created Layer
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m3, &m4);
    ASSERT_STREQ(m4.getTypeName().c_str(), "layer2");
    ASSERT_EQ(m4.getPosition().x, city.getPosition().x);
    ASSERT_EQ(m4.getPosition().y, city.getPosition().y);
    ASSERT_EQ(m4.getPosition().z, city.getPosition().z);
    ASSERT_EQ(m4.getCapacity(), 10u);
    ASSERT_EQ(m4.getColor(), 0x00u);

    // Add again Layer2. Check previous layer has been replaced
    Layer& m5 = city.addLayer(keep<LayerType>("layer2"));
    Layer& m6 = city.getLayer("layer2");
    ASSERT_EQ(&m1, &m2);
    ASSERT_EQ(&m5, &m6);
    ASSERT_NE(&m6, &m4);
    // No longer cap 10 and no longer black color
    ASSERT_EQ(m6.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(m6.getColor(), 0xFFFFFFu);

    // Add a Path
    Path& p1 = city.addPath(keep<PathType>("path1"));
    Path& p2 = city.getPath("path1");
    ASSERT_EQ(&p1, &p2);

    // Check initial values of the newly created Path
    ASSERT_STREQ(p2.getTypeName().c_str(), "path1");
    ASSERT_EQ(p2.m_type.color, 0xFFFFFFu);
    ASSERT_EQ(p2.getNodes().size(), 0u);
    ASSERT_EQ(p2.getSegments().size(), 0u);
    ASSERT_EQ(p2.m_nextNodeId, 0u);
    ASSERT_EQ(p2.m_nextSegmentId, 0u);

    // Replace the Path
    Path& p3 = city.addPath(keep<PathType>("path1", 0xAA));
    Path& p4 = city.getPath("path1");

    // Check previous layer has been replaced
    ASSERT_EQ(&p3, &p4);
    ASSERT_NE(&p3, &p1);
    ASSERT_NE(&p4, &p2);
    ASSERT_STREQ(p4.getTypeName().c_str(), "path1");
    ASSERT_EQ(p4.m_type.color, 0xAAu);

    // Add buildings (way 1)
    BuildingType type5("unit1");
    type5.color = 0xFF00FF;
    type5.radius = 2u;
    Node n1(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Building& b1 = city.addBuilding(type5, n1);
    ASSERT_EQ(city.getBuildings().size(), 1u);
    Building& u2 = *(city.getBuildings()[0]);
    ASSERT_EQ(&u1, &u2);
    ASSERT_EQ(u2.getColor(), 0xFF00FFu);

    // Add agent
    AgentType t("Worker", 1.0f, 2u, 0xFFFFFF);
    Agent& a1 = city.addAgent(t, u2, Resources(), "???");
    Agent& a2 = *(city.getAgents()[0]);
    ASSERT_EQ(&a1, &a2);
    ASSERT_STREQ(a1.m_type.name.c_str(), "Worker");
    ASSERT_EQ(a1.m_type.speed, t.speed);
    ASSERT_EQ(a1.m_type.color, t.color);
    ASSERT_EQ(a1.m_type.radius, t.radius);
    //ASSERT_EQ(&(a1.m_owner), &u1);
}
#endif

// -----------------------------------------------------------------------------
TEST(TestsCity, AddBuildingOnSegmentDoesNotSplitRoad)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& p1 = city.addPath(keep<PathType>("Road"));
    Node& n1 = p1.addNode(Vector3f(0.0f, 0.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(2.0f, 0.0f, 3.0f));
    Segment& w1 = p1.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    ASSERT_EQ(p1.getNodes().size(), 2u);
    ASSERT_EQ(p1.getSegments().size(), 1u);

    Building& b1 = city.addBuilding(keep<BuildingType>("house"), p1, w1, 0.5f);
    ASSERT_EQ(p1.getNodes().size(), 2u);
    ASSERT_EQ(p1.getSegments().size(), 1u);
    ASSERT_EQ(b1.getSegment(), &w1);
    ASSERT_EQ(b1.getNode(), nullptr);
    ASSERT_EQ(int32_t(b1.getPosition().x), 1);
    ASSERT_EQ(w1.getBuildings().size(), 1u);
}

// -----------------------------------------------------------------------------
// Cutting a segment is how the Buildings tool turns a spot on a street into an
// address: the junction is a Node, and agents only ever stop at nodes.
TEST(TestsCity, SplitSegmentCutsTheSegmentInTwo)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    Node& junction = city.splitSegment(path, segment, 0.5f);

    ASSERT_EQ(path.getNodes().size(), 3u);
    ASSERT_EQ(path.getSegments().size(), 2u);
    ASSERT_EQ(int32_t(junction.getPosition().x), 5);
    ASSERT_EQ(junction.getSegments().size(), 2u);
    // The first half was shortened rather than replaced.
    ASSERT_EQ(&segment.getFrom(), &n1);
    ASSERT_EQ(&segment.getTo(), &junction);
    ASSERT_EQ(int32_t(segment.getLength()), 5);
}

// -----------------------------------------------------------------------------
// A building already standing along the street has to end up on the half that
// runs under it, otherwise it addresses a segment that stops short of it.
TEST(TestsCity, SplitSegmentKeepsTheBuildingsWhereTheyStand)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    Building& nearBuilding = city.addBuilding(keep<BuildingType>("Home"), path, segment, 0.2f);
    Building& farBuilding = city.addBuilding(keep<BuildingType>("Home"), path, segment, 0.8f);

    Node& junction = city.splitSegment(path, segment, 0.5f);

    Segment* second = nullptr;
    for (Segment* incident: junction.getSegments())
    {
        if (incident != &segment)
            second = incident;
    }
    ASSERT_NE(second, nullptr);

    ASSERT_EQ(nearBuilding.getSegment(), &segment);
    ASSERT_EQ(farBuilding.getSegment(), second);
    ASSERT_EQ(segment.getBuildings().size(), 1u);
    ASSERT_EQ(second->getBuildings().size(), 1u);
    ASSERT_EQ(int32_t(nearBuilding.getPosition().x + 0.5f), 2);
    ASSERT_EQ(int32_t(farBuilding.getPosition().x + 0.5f), 8);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, SplitSegmentOnAnExtremityCutsNothing)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    ASSERT_EQ(&city.splitSegment(path, segment, 0.0f), &n1);
    ASSERT_EQ(&city.splitSegment(path, segment, 1.0f), &n2);
    ASSERT_EQ(path.getNodes().size(), 2u);
    ASSERT_EQ(path.getSegments().size(), 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, translate)
{
    TestWorld cityWorld("Paris");
    City& city = cityWorld.city;
    city.addLayer(keep<LayerType>("water"));
    Path& p1 = city.addPath(keep<PathType>("Road"));
    Node& n1 = p1.addNode(Vector3f(1.0f, 2.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(3.0f, 3.0f, 3.0f));
    Segment& w1 = p1.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    float const initialMagnitude = w1.getLength();
    Building& b1 = city.addBuilding(keep<BuildingType>("house1"), n1);
    Agent& a1 = city.addAgent(keep<AgentType>("Worker", 1.0f, 2u,
         0xFFFFFF), b1, Resources(), "target");

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

    // Position of the Building == Node1
    ASSERT_EQ(int32_t(n1.m_position.x), 1+1);
    ASSERT_EQ(int32_t(n1.m_position.y), 2+2);
    ASSERT_EQ(int32_t(n1.m_position.z), 3+0);

    // Position of the Segment1 == Node1 and Node2
    ASSERT_EQ(int32_t(w1.getFromPosition().x), 1+1);
    ASSERT_EQ(int32_t(w1.getFromPosition().y), 2+2);
    ASSERT_EQ(int32_t(w1.getFromPosition().z), 3+0);
    ASSERT_EQ(int32_t(w1.getToPosition().x), 3+1);
    ASSERT_EQ(int32_t(w1.getToPosition().y), 3+2);
    ASSERT_EQ(int32_t(w1.getToPosition().z), 3+0);
    ASSERT_FLOAT_EQ(w1.getLength(), initialMagnitude);

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
    RuleBuildingType ruleType("Fill");
    ruleType.rate = 1u;
    ruleType.commands.push_back(&add);
    RuleBuilding rule(ruleType);

    BuildingType type("Home");
    type.rules.push_back(&rule);
    type.resources.setCapacity("People", 10u);

    Building& first = city.addBuilding(type, Vector3f(1.0f, 1.0f, 0.0f));
    Building& second = city.addBuilding(type, Vector3f(3.0f, 1.0f, 0.0f));
    ASSERT_EQ(first.getResources().getAmount("People"), 0u);
    ASSERT_EQ(second.getResources().getAmount("People"), 0u);

    city.update();

    ASSERT_EQ(first.getResources().getAmount("People"), 1u);
    ASSERT_EQ(second.getResources().getAmount("People"), 1u);

    city.update();

    ASSERT_EQ(first.getResources().getAmount("People"), 2u);
    ASSERT_EQ(second.getResources().getAmount("People"), 2u);
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
    path.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    BuildingType homeType("Home");
    Building& home = city.addBuilding(homeType, n1);

    AgentType worker("Worker", 1.0f, 2u, 0xFFFFFF);
    Resources load;
    load.addResource("People", 1u);

    // Nothing in the city answers to "nowhere", so these two keep looking.
    Agent& looking0 = city.addAgent(worker, home, load, "nowhere");
    Agent& looking1 = city.addAgent(worker, home, load, "nowhere");
    // Nothing to deliver: done as soon as it is asked to drive.
    city.addAgent(worker, home, Resources(), "nowhere");

    size_t const id0 = looking0.getId();
    size_t const id1 = looking1.getId();
    ASSERT_EQ(city.getAgents().size(), 3u);

    city.update();

    ASSERT_EQ(city.getAgents().size(), 2u);
    ASSERT_EQ(city.getAgents()[0]->getId(), id0);
    ASSERT_EQ(city.getAgents()[1]->getId(), id1);

    // Empty the load of the first one and it too is done, the second one taking
    // its place without changing identity. Not on the very next tick: having
    // found nothing, it drove off towards a random crossroads, and an Agent on
    // its way somewhere does not stop to look around.
    city.getAgents()[0]->getResources().removeResource("People", 1u);
    for (uint32_t tick = 0u; (tick < 600u) && (city.getAgents().size() > 1u); ++tick)
    {
        city.update();
    }

    ASSERT_EQ(city.getAgents().size(), 1u);
    ASSERT_EQ(city.getAgents()[0]->getId(), id1);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemoveBuildingDetachesItFromItsNode)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    SegmentType segmentType("Dirt", 0xAAAAAA);
    BuildingType buildingType("Home");

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addSegment(segmentType, n1, n2);

    Building& building = city.addBuilding(buildingType, n1);
    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_EQ(n1.getBuildings().size(), 1u);

    city.removeBuilding(building);

    ASSERT_EQ(city.getBuildings().size(), 0u);
    ASSERT_EQ(n1.getBuildings().size(), 0u);
    // The road it stood on is untouched.
    ASSERT_EQ(path.getNodes().size(), 2u);
    ASSERT_EQ(path.getSegments().size(), 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemoveNodeTakesTheBuildingsSittingOnIt)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    SegmentType segmentType("Dirt", 0xAAAAAA);
    BuildingType buildingType("Home");

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addSegment(segmentType, n1, n2);

    city.addBuilding(buildingType, n1);
    city.addBuilding(buildingType, n2);
    ASSERT_EQ(city.getBuildings().size(), 2u);

    // Demolishing the node has to take the building with it, otherwise the Building
    // would be left holding a reference to freed memory.
    city.removeNode(path, n1);

    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_EQ(path.getNodes().size(), 1u);
    ASSERT_EQ(path.getSegments().size(), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemovingTheLastSegmentTakesTheAgentsWithIt)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    SegmentType segmentType("Dirt", 0xAAAAAA);
    BuildingType buildingType("Home");
    AgentType agentType("Worker", 10.0f, 1u, 0xFFFFFF);

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Segment& segment = path.addSegment(segmentType, n1, n2);

    Building& building = city.addBuilding(buildingType, n1);
    building.getResources().setCapacity("People", 10u);
    Resources resources;
    resources.addResource("People", 2u);
    city.addAgent(agentType, building, resources, "Home");
    ASSERT_EQ(city.getAgents().size(), 1u);

    // n1 survives because it carries the building, but it no longer leads
    // anywhere: an Agent standing there could neither move nor deliver, so it
    // goes with the road rather than float over the grid.
    city.removeSegment(path, segment);
    ASSERT_EQ(path.getSegments().size(), 0u);
    ASSERT_EQ(city.getAgents().size(), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemovingOneSegmentKeepsTheAgentsOnTheRest)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    SegmentType segmentType("Dirt", 0xAAAAAA);
    BuildingType buildingType("Home");
    AgentType agentType("Worker", 10.0f, 1u, 0xFFFFFF);

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    Node& n3 = path.addNode(Vector3f(20.0f, 0.0f, 0.0f));
    path.addSegment(segmentType, n1, n2);
    Segment& far = path.addSegment(segmentType, n2, n3);

    Building& building = city.addBuilding(buildingType, n1);
    building.getResources().setCapacity("People", 10u);
    Resources resources;
    resources.addResource("People", 2u);
    city.addAgent(agentType, building, resources, "Home");
    ASSERT_EQ(city.getAgents().size(), 1u);

    // The Agent waits on n1, which keeps a road: demolishing the far segment
    // only invalidates its itinerary.
    city.removeSegment(path, far);
    ASSERT_EQ(path.getSegments().size(), 1u);
    ASSERT_EQ(city.getAgents().size(), 1u);
    ASSERT_EQ(city.getAgents()[0]->getPreviousNode(), &n1);
}

// -----------------------------------------------------------------------------
TEST(TestsCity, RemovingANodeLeavesNoAgentPointingAtIt)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    PathType pathType("Road");
    SegmentType segmentType("Dirt", 0xAAAAAA);
    BuildingType buildingType("Home");
    AgentType agentType("Worker", 10.0f, 1u, 0xFFFFFF);

    Path& path = city.addPath(pathType);
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addSegment(segmentType, n1, n2);

    Building& building = city.addBuilding(buildingType, n2);
    building.getResources().setCapacity("People", 10u);
    Resources resources;
    resources.addResource("People", 2u);
    city.addAgent(agentType, building, resources, "Home");
    ASSERT_EQ(city.getAgents().size(), 1u);

    // The Agent was standing on n2. Demolishing it used to leave the Agent
    // holding a pointer on freed memory, and the next tick read through it.
    city.removeNode(path, n2);
    ASSERT_EQ(city.getAgents().size(), 0u);
    ASSERT_EQ(city.getBuildings().size(), 0u);
    ASSERT_EQ(path.getNodes().size(), 0u);

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
    SegmentType segmentType("Dirt", 0xAAAAAA);

    Path& path = city.addPath(keep<PathType>("Road"));
    Node& n1 = path.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = path.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    path.addSegment(segmentType, n1, n2);
    city.addBuilding(keep<BuildingType>("Home"), n1);
    city.addZone(keep<ZoneType>("Residential"), city.getRegion());

    city.clear();

    ASSERT_EQ(city.getBuildings().size(), 0u);
    ASSERT_EQ(city.getZones().size(), 0u);
    ASSERT_EQ(city.getPaths().size(), 1u);

    Path& road = city.getPath("Road");
    ASSERT_EQ(road.getNodes().size(), 0u);
    ASSERT_EQ(road.getSegments().size(), 0u);

    // And the emptied graph takes new segments.
    Node& n3 = road.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n4 = road.addNode(Vector3f(10.0f, 0.0f, 0.0f));
    road.addSegment(segmentType, n3, n4);
    ASSERT_EQ(road.getSegments().size(), 1u);
}
