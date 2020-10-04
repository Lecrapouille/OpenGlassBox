#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/Map.hpp"
#undef protected
#undef private

#  include "src/Core/City.hpp"
#  include "src/Core/Types.hpp"

// -----------------------------------------------------------------------------
TEST(TestsCity, Constants)
{
    ASSERT_GE(Resource::MAX_CAPACITY, 65535);
    ASSERT_GT(config::GRID_SIZE, 0.0f);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, Constructor)
{
    const uint32_t GRILL = 4u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL + 1u);

    MapType type = { "petrol", 0xFFFFAA, 40u, {} };
    Map map(type, city);

    ASSERT_STREQ(map.type().c_str(), "petrol");
    ASSERT_EQ(map.m_type.color, 0xFFFFAA);
    ASSERT_EQ(map.m_type.capacity, 40u);
    ASSERT_EQ(map.m_type.rules.size(), 0u);
    ASSERT_EQ(int32_t(map.m_position.x), 1);
    ASSERT_EQ(int32_t(map.m_position.y), 2);
    ASSERT_EQ(int32_t(map.m_position.z), 3);
    ASSERT_EQ(map.m_gridSizeU, GRILL);
    ASSERT_EQ(map.m_gridSizeV, GRILL + 1u);
    ASSERT_EQ(map.m_ticks, 0u);
    ASSERT_EQ(map.m_resources.size(), GRILL * (GRILL + 1u));
}

// -----------------------------------------------------------------------------
TEST(TestsMap, setResource)
{
    const uint32_t GRILL = 4u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL + 1u);

    MapType type("map");
    Map map(type, city);
    ASSERT_EQ(map.m_type.capacity, Resource::MAX_CAPACITY);

    map.setResource(0u, 0u, 42u);
    ASSERT_EQ(map.getResource(0u, 0u), 42u);

    map.setResource(0u, 0u, 42u);
    ASSERT_EQ(map.getResource(0u, 0u), 42u);

    map.setResource(0u, 0u, 0u);
    ASSERT_EQ(map.getResource(0u, 0u), 0u);

    map.addResource(0u, 0u, 42u);
    ASSERT_EQ(map.getResource(0u, 0u), 42u);

    map.addResource(0u, 0u, 42u);
    ASSERT_EQ(map.getResource(0u, 0u), 84u);

    map.addResource(0u, 0u, Resource::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0u, 0u), Resource::MAX_CAPACITY);

    map.addResource(0u, 0u, 42u);
    ASSERT_EQ(map.getResource(0u, 0u), Resource::MAX_CAPACITY);

    map.removeResource(0u, 0u, Resource::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0u, 0u), 0u);

    map.removeResource(0u, 0u, Resource::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0u, 0u), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, setCapacity)
{
    const uint32_t GRILL = 4u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL + 1u);

    MapType type("map", 0xFFFFFF, 42u);
    Map map(type, city);

    map.addResource(0u, 0u, 41u);
    ASSERT_EQ(map.getResource(0u, 0u), 41u);

    map.addResource(0u, 0u, 10u);
    ASSERT_EQ(map.getResource(0u, 0u), 42u);

    map.removeResource(0u, 0u, 10u);
    ASSERT_EQ(map.getResource(0u, 0u), 32u);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, addResourceRadius)
{
    using MCIR = MapCoordinatesInsideRadius;
    uint32_t const RADIUS = 1u;
    uint32_t const GRILL = 8u;

    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL);
    Map map(MapType("map"), city);

    // Clear chached values (that may be set by other tests)
    if (map.m_coordinates.m_relativeCoord != nullptr)
        map.m_coordinates.m_relativeCoord->clear();

    // Set some resources
    map.setResource(3u, 4u, 34u);
    map.setResource(4u, 3u, 43u);
    map.setResource(4u, 4u, 44u);
    map.setResource(4u, 5u, 45u);
    map.setResource(5u, 4u, 54u);

    // Compute Resources
    uint32_t total = map.getResource(4u, 4u, RADIUS);
    ASSERT_EQ(map.m_coordinates.m_offset, 5u);
    ASSERT_EQ(map.m_coordinates.m_centerU, 4u);
    ASSERT_EQ(map.m_coordinates.m_centerV, 4u);
    ASSERT_EQ(map.m_coordinates.m_minU, 0u);
    ASSERT_EQ(map.m_coordinates.m_maxU, GRILL);
    ASSERT_EQ(map.m_coordinates.m_minV, 0u);
    ASSERT_EQ(map.m_coordinates.m_maxV, GRILL);
    ASSERT_NE(map.m_coordinates.m_relativeCoord, nullptr);
    ASSERT_EQ(map.m_coordinates.m_relativeCoord->size(), 5u);
    MCIR::RelativeCoordinates& c = MCIR::relativeCoordinates(RADIUS);
    ASSERT_THAT(c, UnorderedElementsAre(
                    MCIR::compress(-1, 0),
                    MCIR::compress(0, -1),
                    MCIR::compress(0, 0),
                    MCIR::compress(0, 1),
                    MCIR::compress(1, 0)
    ));

    ASSERT_EQ(total, 34u + 43u + 44u + 45u + 54u);

    // Add the same amount of resource for each cells
    map.addResource(4u, 4u, RADIUS, 10u, false);
    total = total + 5u * 10u; // + 5 cells x 10 resource
    ASSERT_EQ(map.getResource(4u, 4u, RADIUS), total);

    // Add shared resource for each cells
    map.addResource(4u, 4u, RADIUS, 10u, true);
    total = total + 10u; // + 10 resource
    ASSERT_EQ(map.getResource(4u, 4u, RADIUS), total);

    // Remove shared resource for each cells
    map.removeResource(4u, 4u, RADIUS, 10u, true);
    total = total - 10u; // - 10 resource
    ASSERT_EQ(map.getResource(4u, 4u, RADIUS), total);

    // Add the same amount of resource for each cells
    map.removeResource(4u, 4u, RADIUS, 10u, false);
    total = total - 5u * 10u; // - 5 cells x 10 resource
    ASSERT_EQ(map.getResource(4u, 4u, RADIUS), total);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, getWorldPosition)
{
    const uint32_t GRILL = 4u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL + 1u);

    MapType type("map");
    Map map(type, city);

    Vector3f v;

    v = map.getWorldPosition(0u, 0u);
    ASSERT_EQ(v.x, 0.0f);
    ASSERT_EQ(v.y, 0.0f);
    ASSERT_EQ(v.z, 0.0f);

    v = map.getWorldPosition(1u, 1u);
    ASSERT_EQ(v.x, config::GRID_SIZE);
    ASSERT_EQ(v.y, config::GRID_SIZE);
    ASSERT_EQ(v.z, 0.0f);

    v = map.getWorldPosition(GRILL, GRILL + 1u);
    ASSERT_EQ(v.x, config::GRID_SIZE * float(GRILL));
    ASSERT_EQ(v.y, config::GRID_SIZE * float(GRILL + 1u));
    ASSERT_EQ(v.z, 0.0f);
}

// -----------------------------------------------------------------------------
TEST(TestsMap, Translate)
{
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), 2u, 2u);

    Map map(MapType("map"), city);
    ASSERT_EQ(int32_t(map.m_position.x), 1);
    ASSERT_EQ(int32_t(map.m_position.y), 2);
    ASSERT_EQ(int32_t(map.m_position.z), 3);

    map.translate(Vector3f(1.0f, 2.0f, 3.0f));
    ASSERT_EQ(int32_t(map.m_position.x), 2);
    ASSERT_EQ(int32_t(map.m_position.y), 4);
    ASSERT_EQ(int32_t(map.m_position.z), 6);
}

// -----------------------------------------------------------------------------
class MockRuleMap: public RuleMap
{
public:

    MockRuleMap(RuleMapType const& type) : RuleMap(type) {}
    ~MockRuleMap() = default;
    MOCK_METHOD(bool, execute, (RuleContext&), (override));
};

// -----------------------------------------------------------------------------
TEST(TestsMap, executeRulesNonRandom)
{
    const uint32_t GRILL = 2u;
    City city("Paris", Vector3f(1.0f, 2.0f, 3.0f), GRILL, GRILL);

    RuleMapType rule_type("rule");
    rule_type.randomTiles = false;

    MockRuleMap rule1(rule_type);
    rule1.m_rate = 2u;
    MapType map_type("map");
    map_type.rules.push_back(&rule1);
    Map map(map_type, city);

    EXPECT_CALL(rule1, execute(_)).Times(0);
    map.executeRules();
    ASSERT_EQ(map.m_ticks, 1u);

    EXPECT_CALL(rule1, execute(_)).Times(GRILL * GRILL);
    map.executeRules();
    ASSERT_EQ(map.m_ticks, 2u);
}
