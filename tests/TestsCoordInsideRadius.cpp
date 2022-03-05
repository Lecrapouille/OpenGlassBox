#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/MapCoordinatesInsideRadius.hpp"
#undef protected
#undef private

using MCIR = MapCoordinatesInsideRadius;

// -----------------------------------------------------------------------------
// Check compressing coordinates then decompressing them return the original
// coordinate values.
TEST(TestsMapCoordinatesInsideRadius, CompressUncompressIdentity)
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

// -----------------------------------------------------------------------------
// Radius = 0
TEST(TestsMapCoordinatesInsideRadius, ConstructorZeroUnitRadius)
{
    MCIR coord1;
    MCIR coord2;
    uint32_t const RADIUS = 0u;
    int32_t const ZERO = MCIR::compress(0, 0);

    // Static member variable
    ASSERT_EQ(coord1.m_relativeCoord, coord2.m_relativeCoord);
    if (coord1.m_relativeCoord != nullptr)
    {
        coord1.m_relativeCoord->clear();
        ASSERT_EQ(coord1.m_relativeCoord->size(), 0u);
        ASSERT_EQ(coord2.m_relativeCoord->size(), 0u);
    }

    // Without random
    coord1.init(RADIUS, 2u, 3u, 4u, 5u, 6u, 7u, false);
    ASSERT_NE(coord1.m_relativeCoord, nullptr);
    ASSERT_EQ(coord1.m_centerU, 2u);
    ASSERT_EQ(coord1.m_centerV, 3u);
    ASSERT_EQ(coord1.m_offset, 0u);
    ASSERT_EQ(coord1.m_minU, 4u);
    ASSERT_EQ(coord1.m_maxU, 5u);
    ASSERT_EQ(coord1.m_minV, 6u);
    ASSERT_EQ(coord1.m_maxV, 7u);
    ASSERT_EQ(coord1.m_startingIndex, 0u);
    ASSERT_EQ(MCIR::relativeCoordinates(RADIUS).size(), 1u);
    ASSERT_EQ(MCIR::relativeCoordinates(RADIUS)[0], ZERO);
    ASSERT_EQ(&MCIR::relativeCoordinates(RADIUS), coord1.m_relativeCoord);
    ASSERT_EQ(coord1.m_relativeCoord->size(), 1u);

    // With random
    coord2.init(RADIUS, 2u, 3u, 4u, 5u, 6u, 7u, true);
    ASSERT_NE(coord2.m_relativeCoord, nullptr);
    ASSERT_EQ(coord2.m_centerU, 2u);
    ASSERT_EQ(coord2.m_centerV, 3u);
    ASSERT_EQ(coord2.m_offset, 0u);
    ASSERT_EQ(coord2.m_minU, 4u);
    ASSERT_EQ(coord2.m_maxU, 5u);
    ASSERT_EQ(coord2.m_minV, 6u);
    ASSERT_EQ(coord2.m_maxV, 7u);
    ASSERT_TRUE((coord2.m_startingIndex == 0u) || (coord2.m_startingIndex == 1u));
    ASSERT_EQ(MCIR::relativeCoordinates(RADIUS).size(), 1u);
    ASSERT_EQ(MCIR::relativeCoordinates(RADIUS)[0], ZERO);
    ASSERT_EQ(&MCIR::relativeCoordinates(RADIUS), coord2.m_relativeCoord);
    ASSERT_EQ(coord2.m_relativeCoord->size(), 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsMapCoordinatesInsideRadius, relativeCoordinates)
{
    uint32_t u, v;
    MCIR coord;
    uint32_t RADIUS = 1u;
    uint32_t centerU = 3u;
    uint32_t centerV = 3u;

    // Because of the previous test: m_relativeCoord is static
    if (coord.m_relativeCoord != nullptr)
    {
        coord.m_relativeCoord->clear();
        ASSERT_EQ(coord.m_relativeCoord->size(), 0u);
    }

    coord.init(RADIUS, centerU, centerV, 0u, 10u, 0u, 10u, false);
    MCIR::RelativeCoordinates& c = MCIR::relativeCoordinates(RADIUS);

    // Because of the previous test: m_relativeCoord is static
    ASSERT_NE(coord.m_relativeCoord, nullptr);
    //ASSERT_EQ(coord.m_relativeCoord->size(), 2u);

    ASSERT_EQ(c.size(), 5u);
    EXPECT_THAT(c, ElementsAre(
                    MCIR::compress(-1, 0),
                    MCIR::compress(0, -1),
                    MCIR::compress(0, 0),
                    MCIR::compress(0, 1),
                    MCIR::compress(1, 0)
               ));

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU - 1); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV - 1);

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV + 1);

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU + 1); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord.next(u, v), false);
    ASSERT_EQ(u, 0u); ASSERT_EQ(v, 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsMapCoordinatesInsideRadius, cachedRelativeCoordinatesClipped)
{
    uint32_t u, v;
    MCIR coord;
    uint32_t RADIUS = 1u;
    uint32_t centerU = 3u;
    uint32_t centerV = 3u;

    // Because of the previous test: m_relativeCoord is static
    if (coord.m_relativeCoord != nullptr)
    {
        coord.m_relativeCoord->clear();
        ASSERT_EQ(coord.m_relativeCoord->size(), 0u);
    }

    coord.init(RADIUS, centerU, centerV, 2u, 4u, 2u, 4u, false);
    MCIR::RelativeCoordinates& c = MCIR::relativeCoordinates(RADIUS);

    // Because of the previous test: m_relativeCoord is static
    //ASSERT_EQ(coord.m_relativeCoord->size(), 2u);

    ASSERT_EQ(c.size(), 5u);
    EXPECT_THAT(c, ElementsAre(
                    MCIR::compress(-1, 0),
                    MCIR::compress(0, -1),
                    MCIR::compress(0, 0),
                    MCIR::compress(0, 1),
                    MCIR::compress(1, 0)
    ));

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU - 1); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV - 1);

    ASSERT_EQ(coord.next(u, v), true);
    ASSERT_EQ(u, centerU + 0); ASSERT_EQ(v, centerV + 0);

    ASSERT_EQ(coord.next(u, v), false);
    ASSERT_EQ(u, 0u); ASSERT_EQ(v, 0u);
}
