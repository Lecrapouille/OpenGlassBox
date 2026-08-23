#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/Map.hpp"
#undef protected
#undef private

#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/Types.hpp"
#  include "OpenGlassBox/Config.hpp"

// -----------------------------------------------------------------------------
TEST(TestsMap, Constants)
{
    ASSERT_GT(config::DEFAULT_GRID_CELL_SIZE, 0.0f);
    ASSERT_EQ(Map::CHUNK_SIZE, 16);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, Constructor)
{
    TestWorld cityWorld("Paris", 4u, 5u, Vector3f(1.0f, 2.0f, 3.0f));
    MapType type = { "petrol", 0xFFFFAA, 40u, {} };
    Map map(type, cityWorld.world);

    ASSERT_STREQ(map.type().c_str(), "petrol");
    ASSERT_EQ(map.m_type.color, 0xFFFFAAu);
    ASSERT_EQ(map.m_type.capacity, 40u);
    ASSERT_EQ(map.m_type.rules.size(), 0u);
    ASSERT_EQ(map.m_ticks, 0u);
    ASSERT_EQ(map.allocatedChunks(), 0u);
    ASSERT_EQ(map.totalResource(), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, setResource)
{
    TestWorld cityWorld("Paris", 4u, 5u);
    MapType type("map");
    Map map(type, cityWorld.world);
    ASSERT_EQ(map.m_type.capacity, Resource::MAX_CAPACITY);

    map.setResource(0, 0, 42u);
    ASSERT_EQ(map.getResource(0, 0), 42u);
    ASSERT_EQ(map.allocatedChunks(), 1u);

    map.setResource(0, 0, 0u);
    ASSERT_EQ(map.getResource(0, 0), 0u);

    map.addResource(0, 0, 42u);
    ASSERT_EQ(map.getResource(0, 0), 42u);

    map.addResource(0, 0, 42u);
    ASSERT_EQ(map.getResource(0, 0), 84u);

    map.addResource(0, 0, Resource::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0, 0), Resource::MAX_CAPACITY);

    map.removeResource(0, 0, Resource::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0, 0), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, setCapacity)
{
    TestWorld cityWorld("Paris", 4u, 5u);
    MapType type("map", 0xFFFFFF, 42u);
    Map map(type, cityWorld.world);

    map.addResource(0, 0, 41u);
    ASSERT_EQ(map.getResource(0, 0), 41u);

    map.addResource(0, 0, 10u);
    ASSERT_EQ(map.getResource(0, 0), 42u);

    map.removeResource(0, 0, 10u);
    ASSERT_EQ(map.getResource(0, 0), 32u);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, addResourceRadius)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    Map map(keep<MapType>("map"), cityWorld.world);
    MapRegion const region = city.region();

    map.setResource(3, 4, 34u);
    map.setResource(4, 3, 43u);
    map.setResource(4, 4, 44u);
    map.setResource(4, 5, 45u);
    map.setResource(5, 4, 54u);

    uint32_t total = map.getResource(4, 4, 1u, region);
    ASSERT_EQ(total, 34u + 43u + 44u + 45u + 54u);

    map.addResource(4, 4, 1u, region, 10u, false);
    total = total + 5u * 10u;
    ASSERT_EQ(map.getResource(4, 4, 1u, region), total);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, getWorldPosition)
{
    TestWorld cityWorld("Paris", 4u, 5u);
    City& city = cityWorld.city;
    Map map(keep<MapType>("map"), cityWorld.world);

    Vector3f v = map.getWorldPosition(0, 0);
    ASSERT_EQ(v.x, 0.0f);
    ASSERT_EQ(v.y, 0.0f);

    v = map.getWorldPosition(1, 1);
    ASSERT_EQ(v.x, city.gridCellSize());
    ASSERT_EQ(v.y, city.gridCellSize());
}

// -----------------------------------------------------------------------------
TEST(TestsMap, SparseNegativeCells)
{
    TestWorld cityWorld("Paris");
    Map map(keep<MapType>("map"), cityWorld.world);

    map.setResource(-3, -2, 7u);
    ASSERT_EQ(map.getResource(-3, -2), 7u);
    ASSERT_EQ(map.getResource(0, 0), 0u);
    ASSERT_GE(map.allocatedChunks(), 1u);
}

// -----------------------------------------------------------------------------
//! \brief The sum of a block is kept as cells are written, because the panels
//! and the renderer ask for it on every frame and walking the grid would cost
//! the whole city.
// -----------------------------------------------------------------------------
TEST(TestsMap, TotalAndRegionIterationFollowWrites)
{
    TestWorld cityWorld("Paris", 4u, 4u);
    Map map(keep<MapType>("map"), cityWorld.world);

    map.setResource(1, 1, 10u);
    map.setResource(2, 1, 5u);
    ASSERT_EQ(map.totalResource(), 15u);

    map.setResource(1, 1, 3u);
    ASSERT_EQ(map.totalResource(), 8u);

    map.removeResource(2, 1, 5u);
    ASSERT_EQ(map.totalResource(), 3u);

    // A far away cell allocates its own block, and only the cells inside the
    // asked region are visited.
    map.setResource(100, 100, 7u);
    ASSERT_EQ(map.totalResource(), 10u);

    uint64_t visited = 0u;
    uint64_t sum = 0u;
    map.forEachBlockInRegion(MapRegion{ 0, 0, 4u, 4u },
                             1,
                             [&](int32_t, int32_t, int32_t, uint32_t amount) {
                                 ++visited;
                                 sum += amount;
                             });
    ASSERT_EQ(visited, 1u);
    ASSERT_EQ(sum, 3u);

    // Coarser squares of the same region hand out the average over the cells
    // of the square that lie inside it, which is the one cell holding three.
    visited = 0u;
    sum = 0u;
    map.forEachBlockInRegion(MapRegion{ 0, 0, 4u, 4u },
                             4,
                             [&](int32_t, int32_t, int32_t side,
                                 uint32_t amount) {
                                 ++visited;
                                 sum += amount;
                                 ASSERT_EQ(side, 4);
                             });
    ASSERT_EQ(visited, 1u);
    // Three spread over the sixteen cells of the square rounds down to zero:
    // a lone cell fades as the picture gets coarser, which is the point.
    ASSERT_EQ(sum, 0u);

    // A square as wide as a block visits each of the two blocks once, which is
    // what the renderer draws a layer with when it is zoomed all the way out.
    uint64_t blocks = 0u;
    map.forEachBlockInRegion(MapRegion{ -1000, -1000, 4000u, 4000u },
                             Map::CHUNK_SIZE,
                             [&](int32_t, int32_t, int32_t, uint32_t)
                             { ++blocks; });
    ASSERT_EQ(blocks, 2u);
}

// -----------------------------------------------------------------------------
class MockRuleMap: public RuleMap
{
public:

    MockRuleMap(RuleMapType const& type) : RuleMap(type) {}
    MOCK_METHOD(bool, execute, (RuleContext&), (override));
};

// -----------------------------------------------------------------------------
TEST(TestsMap, executeRulesNonRandom)
{
    const uint32_t GRILL = 2u;
    TestWorld cityWorld("Paris", GRILL, GRILL);
    RuleMapType rule_type("rule");
    rule_type.randomTiles = false;

    MockRuleMap rule1(rule_type);
    rule1.m_rate = 2u;
    MapType map_type("map");
    map_type.rules.push_back(&rule1);
    Map map(map_type, cityWorld.world);

    EXPECT_CALL(rule1, execute(_)).Times(0);
    map.executeRules(cityWorld.world.cities());
    ASSERT_EQ(map.m_ticks, 1u);

    EXPECT_CALL(rule1, execute(_)).Times(GRILL * GRILL);
    map.executeRules(cityWorld.world.cities());
    ASSERT_EQ(map.m_ticks, 2u);
}
