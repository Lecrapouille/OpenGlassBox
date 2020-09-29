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
    Resources r; r.addResource("oil", 5u);
    UnitType unit_type("unit");
    unit_type.color = 0xFF00FF;
    unit_type.radius = 2u;
    unit_type.resources = r;
    Unit unit(unit_type, n, city);
    Resources locals, globals;

    AgentType agent_type("Worker", 1.0f, 2u, 0xFFFFFF);
    RuleCommandAgent ra(agent_type, "home", r);
    ASSERT_STREQ(ra.name.c_str(), "Worker");
    ASSERT_EQ(ra.speed, 1.0f);
    ASSERT_EQ(ra.radius, 2u);
    ASSERT_EQ(ra.color, 0xFFFFFF);
    ASSERT_STREQ(ra.m_target.c_str(), "home");
    ASSERT_EQ(ra.m_resources.m_bin.size(), 1u);
    ASSERT_STREQ(ra.m_resources.m_bin[0].m_type.c_str(), "oil");

    RuleContext context;
    context.city = &city;
    context.unit = &unit;
    context.locals = &locals;
    context.globals = &globals;
    context.u = context.v = 0u;
    context.radius = 1.0;

    // Execute. Check new agent created
    EXPECT_EQ(ra.validate(context), true);
    EXPECT_EQ(city.getAgents().size(), 0u);
    ra.execute(context);

    // See TestsAgent.cpp TEST(TestsAgent, Constructor)
    EXPECT_EQ(city.getAgents().size(), 1u);
    Agent& a = *(city.getAgents()[0]);
    ASSERT_EQ(a.m_id, 0u);
    //ASSERT_EQ(&(a.m_owner), &unit);
    ASSERT_EQ(a.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(a.m_resources.getAmount("oil"), 5u);
    //FIXME ASSERT_EQ(a.m_searchTarget, "home");
    ASSERT_EQ(a.m_position.x, 1.0f);
    ASSERT_EQ(a.m_position.y, 2.0f);
    ASSERT_EQ(a.m_position.z, 3.0f);
    ASSERT_EQ(a.m_offset, 0.0f);
    ASSERT_EQ(a.m_currentWay, nullptr); // FIXME temporary
    ASSERT_EQ(a.m_lastNode, &n);
    ASSERT_EQ(a.m_nextNode, nullptr);

    // Default Agent values // FIXME
    ASSERT_EQ(a.m_type.speed, 1.0f);
    ASSERT_EQ(a.m_type.radius, 2u);
    ASSERT_EQ(a.m_type.color, 0xFFFFFF);
}
