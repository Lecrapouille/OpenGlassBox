#include "main.hpp"
#include "OpenGlassBox/SimulationClock.hpp"
#include "OpenGlassBox/Simulation.hpp"

TEST(TestsClock, HourBetween)
{
    SimulationClock clock(20u);
    ASSERT_EQ(clock.hourOfDay(), 0u);
    ASSERT_EQ(clock.minuteOfHour(), 0u);
    ASSERT_EQ(clock.day(), 0u);

    // 8 hours of game time: 8 * 60 minutes * 20 ticks.
    for (uint32_t i = 0u; i < 8u * 60u * 20u; ++i)
        clock.tick();

    ASSERT_EQ(clock.hourOfDay(), 8u);
    ASSERT_TRUE(clock.hourBetween(8u, 18u));
    ASSERT_FALSE(clock.hourBetween(18u, 22u));

    for (uint32_t i = 0u; i < 14u * 60u * 20u; ++i)
        clock.tick();

    ASSERT_EQ(clock.hourOfDay(), 22u);
    ASSERT_TRUE(clock.hourBetween(18u, 6u));
    ASSERT_FALSE(clock.hourBetween(8u, 18u));
}

//------------------------------------------------------------------------------
// A save carries a tick count, and rules that keep office hours do nothing
// outside their window: the calendar has to be settable.
TEST(TestsClock, SetTimeOfDay)
{
    SimulationClock clock(20u);

    clock.setTimeOfDay(0u, 8u, 30u);
    ASSERT_EQ(clock.ticks(), 8u * 60u * 20u + 30u * 20u);
    ASSERT_EQ(clock.hourOfDay(), 8u);
    ASSERT_EQ(clock.minuteOfHour(), 30u);
    ASSERT_EQ(clock.day(), 0u);
    ASSERT_TRUE(clock.hourBetween(8u, 18u));

    clock.setTimeOfDay(3u, 23u, 59u);
    ASSERT_EQ(clock.day(), 3u);
    ASSERT_EQ(clock.hourOfDay(), 23u);
    ASSERT_EQ(clock.minuteOfHour(), 59u);
    clock.tick();
    ASSERT_EQ(clock.day(), 3u);
    for (uint32_t i = 1u; i < 20u; ++i)
        clock.tick();
    ASSERT_EQ(clock.day(), 4u);
    ASSERT_EQ(clock.hourOfDay(), 0u);

    // The scale is the clock's own: the same time of day is twice the ticks
    // when a minute is worth twice as many.
    clock.setTicksPerMinute(40u);
    clock.setTimeOfDay(0u, 8u, 0u);
    ASSERT_EQ(clock.ticks(), 8u * 60u * 40u);
    ASSERT_EQ(clock.hourOfDay(), 8u);
}

//------------------------------------------------------------------------------
// The default start hour spares the player the wait until the city wakes up.
TEST(TestsClock, SimulationOpensAtTheConfiguredHour)
{
    SimulationConfig config;
    ASSERT_EQ(config.startHour, 8u);

    Simulation simulation(4u, 4u, config);
    ASSERT_EQ(simulation.clock().hourOfDay(), 8u);
    ASSERT_EQ(simulation.clock().minuteOfHour(), 0u);

    config.startHour = 19u;
    Simulation evening(4u, 4u, config);
    ASSERT_EQ(evening.clock().hourOfDay(), 19u);
}
