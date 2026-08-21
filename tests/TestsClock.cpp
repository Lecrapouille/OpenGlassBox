#include "main.hpp"
#include "OpenGlassBox/SimulationClock.hpp"

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
