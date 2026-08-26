#include "main.hpp"

#define protected public
#define private public
#  include "OpenGlassBox/Simulation.hpp"
#undef protected
#undef private

#  include "OpenGlassBox/Config.hpp"

// -----------------------------------------------------------------------------
TEST(TestsSimulation, Constants)
{
    ASSERT_GT(config::DEFAULT_TICKS_PER_SECOND, 0.0f);

    SimulationConfig config;
    ASSERT_EQ(config.ticksPerSecond, config::DEFAULT_TICKS_PER_SECOND);
    ASSERT_EQ(config.tickDuration(), 1.0f / config::DEFAULT_TICKS_PER_SECOND);
}

// -----------------------------------------------------------------------------
TEST(TestsSimulation, TimeControl)
{
    SimulationConfig config;
    config.ticksPerSecond = 10.0f;
    Simulation sim(4u, 4u, config);

    ASSERT_EQ(sim.totalTicks(), 0u);
    ASSERT_EQ(sim.paused(), true);
    ASSERT_EQ(sim.timeScale(), 1.0f);

    sim.setPaused(false);

    // One tick lasts 100 ms: 250 ms is worth two ticks and leaves 50 ms.
    sim.update(0.25f);
    ASSERT_EQ(sim.totalTicks(), 2u);

    // Twice as fast: the same elapsed time is worth five ticks.
    sim.setTimeScale(2.0f);
    sim.update(0.25f);
    ASSERT_EQ(sim.totalTicks(), 7u);

    // Paused: time is not accumulated at all.
    sim.setPaused(true);
    sim.update(10.0f);
    ASSERT_EQ(sim.totalTicks(), 7u);

    // Stepping ignores the pause state.
    sim.stepOneTick();
    ASSERT_EQ(sim.totalTicks(), 8u);

    // A huge elapsed time is clamped by maxTicksPerUpdate.
    sim.setPaused(false);
    sim.setTimeScale(1.0f);
    sim.config().maxTicksPerUpdate = 3u;
    sim.update(1000.0f);
    ASSERT_EQ(sim.totalTicks(), 11u);
}

// -----------------------------------------------------------------------------
TEST(TestsSimulation, Constructor)
{
    Simulation sim(4u, 5u);

    ASSERT_EQ(sim.m_gridSizeU, 4u);
    ASSERT_EQ(sim.m_gridSizeV, 5u);
    ASSERT_EQ(sim.m_time, 0.0f);
    ASSERT_EQ(sim.m_world.m_cities.size(), 0u);

    City& c1 = sim.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    ASSERT_STREQ(c1.name().c_str(), "Paris");

    City& c2 = sim.getCity("Paris");
    ASSERT_EQ(&c1, &c2);
    ASSERT_STREQ(c2.name().c_str(), "Paris");

    ASSERT_EQ(sim.m_world.m_cities.size(), 1u);
    ASSERT_EQ(sim.m_world.m_cities["Paris"].get(), &c1);
    ASSERT_STREQ(sim.m_world.m_cities["Paris"]->name().c_str(), "Paris");
}
