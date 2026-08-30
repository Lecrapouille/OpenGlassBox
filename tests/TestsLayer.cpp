#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/Layer.hpp"
#undef protected
#undef private

#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/Types.hpp"
#  include "OpenGlassBox/Config.hpp"

// -----------------------------------------------------------------------------
TEST(TestsLayer, Constants)
{
    ASSERT_GT(defaults::GRID_CELL_SIZE, 0.0f);
    ASSERT_EQ(Layer::CHUNK_SIZE, 16);
}

// -----------------------------------------------------------------------------
TEST(TestsLayer, Constructor)
{
    TestWorld cityWorld("Paris", 4u, 5u, Vector3f(1.0f, 2.0f, 3.0f));
    LayerType type = { "petrol", 0xFFFFAA, 40u, {} };
    Layer layer(type, cityWorld.world);

    ASSERT_STREQ(layer.getTypeName().c_str(), "petrol");
    ASSERT_EQ(layer.m_type.color, 0xFFFFAAu);
    ASSERT_EQ(layer.m_type.capacity, 40u);
    ASSERT_EQ(layer.m_type.rules.size(), 0u);
    ASSERT_EQ(layer.m_ticks, 0u);
    ASSERT_EQ(layer.getBlockCount(), 0u);
    ASSERT_EQ(layer.getTotalResource(), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsLayer, resource)
{
    TestWorld cityWorld("Paris", 4u, 5u);
    LayerType type("layer");
    Layer layer(type, cityWorld.world);
    ASSERT_EQ(layer.m_type.capacity, Resource::MAX_CAPACITY);

    layer.setResource({ 0, 0 }, 42u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 42u);
    ASSERT_EQ(layer.getBlockCount(), 1u);

    layer.setResource({ 0, 0 }, 0u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 0u);

    layer.addResource({ 0, 0 }, 42u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 42u);

    layer.addResource({ 0, 0 }, 42u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 84u);

    layer.addResource({ 0, 0 }, Resource::MAX_CAPACITY);
    ASSERT_EQ(layer.getResource({ 0, 0 }), Resource::MAX_CAPACITY);

    layer.removeResource({ 0, 0 }, Resource::MAX_CAPACITY);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsLayer, capacity)
{
    TestWorld cityWorld("Paris", 4u, 5u);
    LayerType type("layer", 0xFFFFFF, 42u);
    Layer layer(type, cityWorld.world);

    layer.addResource({ 0, 0 }, 41u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 41u);

    layer.addResource({ 0, 0 }, 10u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 42u);

    layer.removeResource({ 0, 0 }, 10u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 32u);
}

// -----------------------------------------------------------------------------
TEST(TestsLayer, addResourceRadius)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    Layer layer(keep<LayerType>("layer"), cityWorld.world);
    CellRegion const region = city.getRegion();

    layer.setResource({ 3, 4 }, 34u);
    layer.setResource({ 4, 3 }, 43u);
    layer.setResource({ 4, 4 }, 44u);
    layer.setResource({ 4, 5 }, 45u);
    layer.setResource({ 5, 4 }, 54u);

    uint32_t total = layer.getResource({ 4, 4 }, 1u, region);
    ASSERT_EQ(total, 34u + 43u + 44u + 45u + 54u);

    layer.addResource({ 4, 4 }, 1u, region, 10u, false);
    total = total + 5u * 10u;
    ASSERT_EQ(layer.getResource({ 4, 4 }, 1u, region), total);
}

// -----------------------------------------------------------------------------
TEST(TestsLayer, worldPosition)
{
    TestWorld cityWorld("Paris", 4u, 5u);
    City& city = cityWorld.city;
    Layer layer(keep<LayerType>("layer"), cityWorld.world);

    Vector3f v = layer.cellToWorld({ 0, 0 });
    ASSERT_EQ(v.x, 0.0f);
    ASSERT_EQ(v.y, 0.0f);

    v = layer.cellToWorld({ 1, 1 });
    ASSERT_EQ(v.x, city.getCellSize());
    ASSERT_EQ(v.y, city.getCellSize());
}

// -----------------------------------------------------------------------------
TEST(TestsLayer, SparseNegativeCells)
{
    TestWorld cityWorld("Paris");
    Layer layer(keep<LayerType>("layer"), cityWorld.world);

    layer.setResource({ -3, -2 }, 7u);
    ASSERT_EQ(layer.getResource({ -3, -2 }), 7u);
    ASSERT_EQ(layer.getResource({ 0, 0 }), 0u);
    ASSERT_GE(layer.getBlockCount(), 1u);
}

// -----------------------------------------------------------------------------
//! \brief The sum of a block is kept as cells are written, because the panels
//! and the renderer ask for it on every frame and walking the grid would cost
//! the whole city.
// -----------------------------------------------------------------------------
TEST(TestsLayer, TotalAndRegionIterationFollowWrites)
{
    TestWorld cityWorld("Paris", 4u, 4u);
    Layer layer(keep<LayerType>("layer"), cityWorld.world);

    layer.setResource({ 1, 1 }, 10u);
    layer.setResource({ 2, 1 }, 5u);
    ASSERT_EQ(layer.getTotalResource(), 15u);

    layer.setResource({ 1, 1 }, 3u);
    ASSERT_EQ(layer.getTotalResource(), 8u);

    layer.removeResource({ 2, 1 }, 5u);
    ASSERT_EQ(layer.getTotalResource(), 3u);

    // A far away cell allocates its own block, and only the cells inside the
    // asked region are visited.
    layer.setResource({ 100, 100 }, 7u);
    ASSERT_EQ(layer.getTotalResource(), 10u);

    uint64_t visited = 0u;
    uint64_t sum = 0u;
    layer.forEachBlockInRegion(CellRegion{ 0, 0, 4u, 4u },
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
    layer.forEachBlockInRegion(CellRegion{ 0, 0, 4u, 4u },
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
    layer.forEachBlockInRegion(CellRegion{ -1000, -1000, 4000u, 4000u },
                             Layer::CHUNK_SIZE,
                             [&](int32_t, int32_t, int32_t, uint32_t)
                             { ++blocks; });
    ASSERT_EQ(blocks, 2u);
}

// -----------------------------------------------------------------------------
class MockRuleLayer: public RuleLayer
{
public:

    MockRuleLayer(RuleLayerType const& type) : RuleLayer(type) {}
    MOCK_METHOD(bool, execute, (RuleContext&), (override));
};

// -----------------------------------------------------------------------------
TEST(TestsLayer, executeRulesNonRandom)
{
    const uint32_t GRILL = 2u;
    TestWorld cityWorld("Paris", GRILL, GRILL);
    RuleLayerType rule_type("rule");
    rule_type.randomTiles = false;

    MockRuleLayer rule1(rule_type);
    rule1.m_rate = 2u;
    LayerType layer_type("layer");
    layer_type.rules.push_back(&rule1);
    Layer layer(layer_type, cityWorld.world);

    EXPECT_CALL(rule1, execute(_)).Times(0);
    layer.executeRules(cityWorld.world.getCities());
    ASSERT_EQ(layer.m_ticks, 1u);

    EXPECT_CALL(rule1, execute(_)).Times(GRILL * GRILL);
    layer.executeRules(cityWorld.world.getCities());
    ASSERT_EQ(layer.m_ticks, 2u);
}

//------------------------------------------------------------------------------
//! \brief A layer that only diffuses moves its amounts and keeps their total.
//! Smoke travels to the streets nearby, and nothing is created on the way.
//------------------------------------------------------------------------------
TEST(TestsLayer, DiffusionMovesTheAmountAndKeepsTheTotal)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    LayerType type("Pollution");
    type.capacity = 1000u;
    type.diffusion = 100u;
    Layer layer(type, cityWorld.world);

    Cell const source{ 5, 5 };
    layer.setResource(source, 400u);
    ASSERT_EQ(layer.getTotalResource(), 400u);

    layer.spreadAndFade();

    // The whole amount left, and the four neighbours took an equal share.
    ASSERT_EQ(layer.getResource(source), 0u);
    ASSERT_EQ(layer.getResource(Cell{ 4, 5 }), 100u);
    ASSERT_EQ(layer.getResource(Cell{ 6, 5 }), 100u);
    ASSERT_EQ(layer.getResource(Cell{ 5, 4 }), 100u);
    ASSERT_EQ(layer.getResource(Cell{ 5, 6 }), 100u);
    ASSERT_EQ(layer.getTotalResource(), 400u);

    // A diagonal cell is reached on the second pass, not on the first: an amount
    // travels one cell per period, whatever the order the grid is written in.
    ASSERT_EQ(layer.getResource(Cell{ 4, 4 }), 0u);
    layer.spreadAndFade();
    // Two of the four cells filled above are next to this one, and each gives it
    // a quarter of what it holds.
    ASSERT_EQ(layer.getResource(Cell{ 4, 4 }), 50u);
    ASSERT_EQ(layer.getTotalResource(), 400u);
}

//------------------------------------------------------------------------------
//! \brief The remainder of the division by four stays in the cell, so an amount
//! that four neighbours cannot share equally is still not lost.
//------------------------------------------------------------------------------
TEST(TestsLayer, DiffusionLosesNothingToRounding)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    LayerType type("Noise");
    type.capacity = 1000u;
    type.diffusion = 50u;
    Layer layer(type, cityWorld.world);

    layer.setResource(Cell{ 8, 8 }, 999u);

    for (uint32_t i = 0u; i < 20u; ++i)
    {
        layer.spreadAndFade();
        ASSERT_EQ(layer.getTotalResource(), 999u);
    }
}

//------------------------------------------------------------------------------
//! \brief Decay makes an amount fade where it stands. Without it every source
//! would fill the grid to its capacity and the layer would say nothing.
//------------------------------------------------------------------------------
TEST(TestsLayer, DecayFadesTheAmountToNothing)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    LayerType type("FireHazard");
    type.capacity = 100u;
    type.decay = 50u;
    Layer layer(type, cityWorld.world);

    Cell const cell{ 3, 7 };
    layer.setResource(cell, 100u);

    layer.spreadAndFade();
    ASSERT_EQ(layer.getResource(cell), 50u);
    layer.spreadAndFade();
    ASSERT_EQ(layer.getResource(cell), 25u);
    layer.spreadAndFade();
    ASSERT_EQ(layer.getResource(cell), 13u);

    // A share of a small amount rounds down to nothing, so the fade would stop
    // at one. Removing at least one unit is what empties the cell in the end.
    for (uint32_t i = 0u; i < 20u; ++i)
        layer.spreadAndFade();
    ASSERT_EQ(layer.getResource(cell), 0u);
    ASSERT_EQ(layer.getTotalResource(), 0u);
}

//------------------------------------------------------------------------------
//! \brief Diffusion crosses the border of a storage block, which means the pass
//! has to allocate the block next to it.
//------------------------------------------------------------------------------
TEST(TestsLayer, DiffusionCrossesABlockBorder)
{
    TestWorld cityWorld("Paris", 64u, 64u);
    LayerType type("Pollution");
    type.capacity = 1000u;
    type.diffusion = 100u;
    Layer layer(type, cityWorld.world);

    // The last column of the first block. Its right neighbour is in the next one.
    Cell const border{ Layer::CHUNK_SIZE - 1, 4 };
    layer.setResource(border, 400u);
    ASSERT_EQ(layer.getBlockCount(), 1u);

    layer.spreadAndFade();

    ASSERT_EQ(layer.getResource(Cell{ Layer::CHUNK_SIZE, 4 }), 100u);
    ASSERT_EQ(layer.getBlockCount(), 2u);
    ASSERT_EQ(layer.getTotalResource(), 400u);
}

//------------------------------------------------------------------------------
//! \brief A cell already full refuses what its neighbours push, so a layer with
//! a low capacity saturates instead of counting past it.
//------------------------------------------------------------------------------
TEST(TestsLayer, DiffusionStopsAtTheCapacityOfACell)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    LayerType type("Pollution");
    type.capacity = 10u;
    type.diffusion = 100u;
    Layer layer(type, cityWorld.world);

    layer.setResource(Cell{ 6, 6 }, 10u);
    layer.setResource(Cell{ 7, 6 }, 10u);
    layer.setResource(Cell{ 5, 6 }, 10u);
    layer.setResource(Cell{ 6, 5 }, 10u);
    layer.setResource(Cell{ 6, 7 }, 10u);

    layer.spreadAndFade();

    // Every cell stays at or below the capacity of the layer.
    for (int32_t v = 4; v < 9; ++v)
    {
        for (int32_t u = 4; u < 9; ++u)
            ASSERT_LE(layer.getResource(Cell{ u, v }), 10u);
    }
}

//------------------------------------------------------------------------------
//! \brief Transport follows the period of the layer, not the tick.
//------------------------------------------------------------------------------
TEST(TestsLayer, TransportFollowsTheLayerPeriod)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    LayerType type("Pollution");
    type.capacity = 100u;
    type.decay = 50u;
    type.rate = 3u;
    Layer layer(type, cityWorld.world);

    Cell const cell{ 2, 2 };
    layer.setResource(cell, 100u);

    layer.executeRules(cityWorld.world.getCities());
    ASSERT_EQ(layer.getResource(cell), 100u);
    layer.executeRules(cityWorld.world.getCities());
    ASSERT_EQ(layer.getResource(cell), 100u);
    layer.executeRules(cityWorld.world.getCities());
    ASSERT_EQ(layer.getResource(cell), 50u);
}

//------------------------------------------------------------------------------
//! \brief A layer with neither diffusion nor decay keeps every amount where a
//! rule put it, which is what every layer written so far expects.
//------------------------------------------------------------------------------
TEST(TestsLayer, WithoutDiffusionNorDecayNothingMoves)
{
    TestWorld cityWorld("Paris", 32u, 32u);
    LayerType type("Water");
    type.capacity = 100u;
    Layer layer(type, cityWorld.world);

    ASSERT_FALSE(type.spreads());

    Cell const cell{ 9, 9 };
    layer.setResource(cell, 42u);

    for (uint32_t i = 0u; i < 10u; ++i)
        layer.executeRules(cityWorld.world.getCities());

    ASSERT_EQ(layer.getResource(cell), 42u);
    ASSERT_EQ(layer.getTotalResource(), 42u);
    ASSERT_EQ(layer.getBlockCount(), 1u);
}
