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
    ASSERT_GT(defaults::TICKS_PER_SECOND, 0.0f);

    Config config;
    ASSERT_EQ(config.time.ticksPerSecond, defaults::TICKS_PER_SECOND);
    ASSERT_EQ(config.time.tickDuration(), 1.0f / defaults::TICKS_PER_SECOND);
}

// -----------------------------------------------------------------------------
TEST(TestsSimulation, TimeControl)
{
    Config config;
    config.time.ticksPerSecond = 10.0f;
    config.time.startHour = 0u;
    Simulation sim(config);

    ASSERT_EQ(sim.getClock().getTicks(), 0u);
    ASSERT_EQ(sim.isPaused(), true);
    ASSERT_EQ(sim.getTimeScale(), 1.0f);

    sim.setPaused(false);

    // One tick lasts 100 ms: 250 ms is worth two ticks and leaves 50 ms.
    sim.update(0.25f);
    ASSERT_EQ(sim.getClock().getTicks(), 2u);

    // Twice as fast: the same elapsed time is worth five ticks.
    sim.setTimeScale(2.0f);
    sim.update(0.25f);
    ASSERT_EQ(sim.getClock().getTicks(), 7u);

    // Paused: time is not accumulated at all.
    sim.setPaused(true);
    sim.update(10.0f);
    ASSERT_EQ(sim.getClock().getTicks(), 7u);

    // Stepping ignores the pause state.
    sim.stepOneTick();
    ASSERT_EQ(sim.getClock().getTicks(), 8u);

    // A huge elapsed time is clamped by TimeConfig::maxTicksPerUpdate.
    sim.setPaused(false);
    sim.setTimeScale(1.0f);
    config.time.maxTicksPerUpdate = 3u;
    sim.setConfig(config);
    sim.update(1000.0f);
    ASSERT_EQ(sim.getClock().getTicks(), 11u);
}

// -----------------------------------------------------------------------------
TEST(TestsSimulation, Constructor)
{
    Simulation sim;

    ASSERT_EQ(sim.m_time, 0.0f);
    ASSERT_EQ(sim.getCities().size(), 0u);

    City& c1 = sim.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    ASSERT_STREQ(c1.getName().c_str(), "Paris");

    City& c2 = sim.getCity("Paris");
    ASSERT_EQ(&c1, &c2);
    ASSERT_STREQ(c2.getName().c_str(), "Paris");

    ASSERT_EQ(sim.getCities().size(), 1u);
    ASSERT_EQ(sim.findCity("Paris"), &c1);
    ASSERT_EQ(sim.findCity("Berlin"), nullptr);

    // The grid of a city defaults to what GridConfig says.
    ASSERT_EQ(c1.getRegion().sizeU, sim.getConfig().grid.defaultCitySizeU);
    ASSERT_EQ(c1.getRegion().sizeV, sim.getConfig().grid.defaultCitySizeV);
}
