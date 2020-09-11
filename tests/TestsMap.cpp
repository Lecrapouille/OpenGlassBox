#include "main.hpp"

#define protected public
#define private public
#  include "src/Map.hpp"
#undef protected
#undef private

TEST(TestsCity, Constants)
{
    ASSERT_GE(Map::MAX_CAPACITY, 65535);
    ASSERT_GT(config::GRID_SIZE, 0.0f);
}

TEST(TestsMap, Constructor)
{
    const uint32_t GRILL = 4u;

    Map map("map", GRILL, GRILL + 1u, 42u);

    ASSERT_STREQ(map.id().c_str(), "map");
    ASSERT_EQ(map.m_sizeU, GRILL);
    ASSERT_EQ(map.m_sizeV, GRILL + 1u);
    ASSERT_EQ(map.m_ticks, 0u);
    ASSERT_EQ(map.m_capacity, 42u);
    ASSERT_EQ(map.m_resources.size(), GRILL * (GRILL + 1u));
}

TEST(TestsMap, setResource)
{
    const uint32_t GRILL = 4u;

    Map map("map", GRILL, GRILL);
    ASSERT_EQ(map.m_capacity, Map::MAX_CAPACITY);

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

    map.addResource(0u, 0u, Map::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0u, 0u), Map::MAX_CAPACITY);

    map.addResource(0u, 0u, 42u);
    ASSERT_EQ(map.getResource(0u, 0u), Map::MAX_CAPACITY);

    map.removeResource(0u, 0u, Map::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0u, 0u), 0u);

    map.removeResource(0u, 0u, Map::MAX_CAPACITY);
    ASSERT_EQ(map.getResource(0u, 0u), 0u);
}

TEST(TestsMap, setCapacity)
{
    const uint32_t GRILL = 4u;

    Map map("map", GRILL, GRILL, 42u);

    map.addResource(0u, 0u, 41u);
    ASSERT_EQ(map.getResource(0u, 0u), 41u);

    map.addResource(0u, 0u, 10u);
    ASSERT_EQ(map.getResource(0u, 0u), 42u);

    map.removeResource(0u, 0u, 10u);
    ASSERT_EQ(map.getResource(0u, 0u), 32u);
}

TEST(TestsMap, getWorldPosition)
{
    const uint32_t GRILL = 4u;

    Map map("map", GRILL, GRILL + 1u);

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

TEST(TestsMap, executeRules)
{
    // TODO
}
