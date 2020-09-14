#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/RuleCommand.hpp"
#  include "src/Core/City.hpp"
#undef protected
#undef private

TEST(TestsCommand, Constructor)
{
    City city("Paris", 2u, 2u);
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit unit("unit", n);
    Resources locals, globals;

    RuleCommandAgent ra;
    RuleContext context;
    context.city = &city;
    context.unit = &unit;
    context.locals = &locals;
    context.globals = &globals;
    context.u = context.v = 0u;
    context.radius = 1.0;
    ra.target = "foo";
    ra.resources.addResource("oil", 5u);

    // Execute. Check new agent created
    EXPECT_EQ(ra.validate(context), true);
    EXPECT_EQ(city.getAgents().size(), 0u);
    ra.execute(context);

    // See TestsAgent.cpp TEST(TestsAgent, Constructor)
    EXPECT_EQ(city.getAgents().size(), 1u);
    Agent& a = *(city.getAgents()[0]);
    ASSERT_EQ(a.m_id, 0u);
    ASSERT_EQ(&(a.m_owner), &unit);
    ASSERT_EQ(a.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(a.m_resources.getAmount("oil"), 5u);
    ASSERT_EQ(a.m_searchTarget, "foo");
    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentSegment, nullptr); // FIXME temporary
    ASSERT_EQ(a.m_lastNode, &n);
    ASSERT_EQ(a.m_nextNode, nullptr);

    // Default Agent values // FIXME
    ASSERT_EQ(a.m_speed, 1.0f);
    ASSERT_EQ(a.m_radius, 1.0f);
    ASSERT_EQ(a.m_color, 0xFFFFFF);
}
