#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/Map.hpp"
#undef protected
#undef private

#  include "src/Core/City.hpp"
#  include "src/Core/Types.hpp"

TEST(TestsCity, Constants)
{
    ASSERT_GE(Resource::MAX_CAPACITY, 65535);
    ASSERT_GT(config::GRID_SIZE, 0.0f);
}

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

TEST(TestsMap, executeRules)
{
    // TODO
}
