#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/MapCoordinatesInsideRadius.hpp"
#undef protected
#undef private

using MCIR = MapCoordinatesInsideRadius;

TEST(TestsMapCoordinatesInsideRadius, Constructor)
{
    MCIR coord1;
    MCIR coord2;
    uint32_t const radius = 0u;
    uint32_t const v = (MCIR::MAX_RADIUS << 16u) + MCIR::MAX_RADIUS;

    //
    coord1.init(radius, 2u, 3u, 4u, 5u, 6u, 7u, false);
    ASSERT_NE(coord1.m_values, nullptr);
    ASSERT_EQ(coord1.m_centerU, 2u);
    ASSERT_EQ(coord1.m_centerV, 3u);
    ASSERT_EQ(coord1.m_offset, 0u);
    ASSERT_EQ(coord1.m_minU, 4u);
    ASSERT_EQ(coord1.m_maxU, 5u);
    ASSERT_EQ(coord1.m_minV, 6u);
    ASSERT_EQ(coord1.m_maxV, 7u);

    ASSERT_GE(coord2.m_startingIndex, 0u);
    ASSERT_LE(coord2.m_startingIndex, 1u);
    ASSERT_EQ(MCIR::cachedCoordinates(radius).size(), 1u);
    ASSERT_EQ(MCIR::cachedCoordinates(radius)[0], v);

    //
    coord2.init(radius, 2u, 3u, 4u, 5u, 6u, 7u, true);
    ASSERT_NE(coord2.m_values, nullptr);
    ASSERT_EQ(coord2.m_centerU, 2u);
    ASSERT_EQ(coord2.m_centerV, 3u);
    ASSERT_EQ(coord2.m_offset, 0u);
    ASSERT_EQ(coord2.m_minU, 4u);
    ASSERT_EQ(coord2.m_maxU, 5u);
    ASSERT_EQ(coord2.m_minV, 6u);
    ASSERT_EQ(coord2.m_maxV, 7u);

    ASSERT_GE(coord2.m_startingIndex, 0u);
    ASSERT_LE(coord2.m_startingIndex, 1u);
    ASSERT_EQ(MCIR::cachedCoordinates(radius).size(), 1u);
    ASSERT_EQ(MCIR::cachedCoordinates(radius)[0], v);
}

TEST(TestsMapCoordinatesInsideRadius, compress)
{
    int32_t u, v;

    for (int32_t i = -128; i < 128; ++i)
    {
        for (int32_t j = -128; j < 128; ++j)
        {
            MCIR::uncompress(MCIR::compress(i, j), u, v);
            ASSERT_EQ(i, u);
            ASSERT_EQ(j, v);
        }
    }
}

TEST(TestsMapCoordinatesInsideRadius, cachedCoordinates)
{
    uint32_t u, v;
    MCIR coord1;
    uint32_t radius = 1u;
    uint32_t centerU = 3u;
    uint32_t centerV = 3u;

    coord1.init(radius, centerU, centerV, 2u, 4u, 2u, 4u, false);
    MCIR::Coordinates& c = MCIR::cachedCoordinates(radius);

    ASSERT_EQ(c.size(), 5u);
    EXPECT_THAT(c, ::testing::ElementsAre(
                    MCIR::compress(-1, 0),
                    MCIR::compress(0, -1),
                    MCIR::compress(0, 0),
                    MCIR::compress(0, 1),
                    MCIR::compress(1, 0)
               ));

    ASSERT_EQ(coord1.next(u, v), true);
    ASSERT_EQ(u, centerU - 1); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord1.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV - 1);

    ASSERT_EQ(coord1.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord1.next(u, v), false);
    ASSERT_EQ(u, 0u); ASSERT_EQ(v, 0u);
}
