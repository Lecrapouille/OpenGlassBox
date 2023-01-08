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
    ASSERT_GT(config::TICKS_PER_SECOND, 0u);
}

// -----------------------------------------------------------------------------
TEST(TestsSimulation, Constructor)
{
    Simulation sim(4u, 5u);

    ASSERT_EQ(sim.m_gridSizeU, 4u);
    ASSERT_EQ(sim.m_gridSizeV, 5u);
    ASSERT_EQ(sim.m_time, 0.0f);
    ASSERT_EQ(sim.m_cities.size(), 0u);

    City& c1 = sim.addCity("Paris", Vector3f(0.0f, 0.0f, 0.0f));
    ASSERT_STREQ(c1.name().c_str(), "Paris");

    City& c2 = sim.getCity("Paris");
    ASSERT_EQ(&c1, &c2);
    ASSERT_STREQ(c2.name().c_str(), "Paris");

    ASSERT_EQ(sim.m_cities.size(), 1u);
    ASSERT_EQ(sim.m_cities["Paris"].get(), &c1);
    ASSERT_STREQ(sim.m_cities["Paris"]->name().c_str(), "Paris");
}
