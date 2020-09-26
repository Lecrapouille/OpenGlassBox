#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/Unit.hpp"
#  include "src/Core/City.hpp"
#undef protected
#undef private

class MockIRuleCommand: public IRuleCommand
{
public:
    MockIRuleCommand() = default;
    ~MockIRuleCommand() = default;
    MOCK_METHOD(bool, validate, (RuleContext const&), (const override));
    MOCK_METHOD(void, execute, (RuleContext&), (override));
};

TEST(TestsUnit, NominalCase)
{
    City city("Paris", 4u, 4u);
    Node node(42u, Vector3f(4.0f, 4.0f, 4.0f));

    UnitType unit_type("unit");
    unit_type.m_color = 42u;
    unit_type.m_radius = 2u;
    unit_type.m_resources.addResource("car", 5u);
    unit_type.m_targets.push_back("foo");
    Unit u(unit_type, node, city);

    ASSERT_STREQ(u.m_type.name.c_str(), "unit");
    ASSERT_EQ(u.m_type.color, 42u);
    ASSERT_EQ(u.m_type.resources.m_bin.size(), 1u);
    ASSERT_STREQ(u.m_type.resources.m_bin[0].type().c_str(), "car");
    ASSERT_EQ(u.m_type.resources.getAmount("car"), 5u);
    ASSERT_EQ(u.m_type.rules.size(), 0u);
    ASSERT_EQ(u.m_type.targets.size(), 1u);
    ASSERT_STREQ(u.m_type.targets[0].c_str(), "foo");
    ASSERT_EQ(&u.m_type.node, &node);
    ASSERT_EQ(u.m_type.ticks, 0u);
    ASSERT_EQ(u.m_type.context.radius, 2u);
    ASSERT_EQ(u.m_type.context.unit, &u);
    ASSERT_EQ(u.m_type.context.city, &city);
    ASSERT_EQ(u.m_type.context.globals, &city.globalResources());
    ASSERT_EQ(u.m_type.context.locals, &u.m_type.resources);
    ASSERT_EQ(u.m_type.context.u, 2u);
    ASSERT_EQ(u.m_type.context.v, 2u);

    // Check accept
    Resources r0;
    Resources r1; r1.addResource("car", 5u);
    Resources r2; r2.addResource("oil", 5u);

    ASSERT_EQ(u.accepts("foo", r0), false);
    ASSERT_EQ(u.accepts("foo", r1), true);
    ASSERT_EQ(u.accepts("bar", r1), false);
    ASSERT_EQ(u.accepts("foo", r2), false);

    r2.addResource("car", 5u);
    ASSERT_EQ(u.accepts("foo", r2), true);

    // No rules to run: Check if execute nothing happens
    u.executeRules();
    ASSERT_EQ(u.m_type.ticks, 1u);
    u.executeRules();
    ASSERT_EQ(u.m_type.ticks, 2u);

    // Add rule
    MockIRuleCommand cmd1;
    RuleUnitType ruleunit_type("ru");
    ruleunit_type.m_rate = 4u;
    ruleunit_type.m_onFail = nullptr;
    ruleunit_type.m_commands.push_back(&cmd1);
    RuleUnit ru(ruleunit_type);
    u.m_type.rules.push_back(&ru);

    // Single rule to run but tocks does not match yet rate
    u.m_type.ticks = 2u;
    EXPECT_CALL(cmd1, validate(_)).Times(0);
    EXPECT_CALL(cmd1, execute(_)).Times(0);
    u.executeRules();
    ASSERT_EQ(u.m_type.ticks, 3u);

    // Single rule and ticks matches rate
    u.m_type.ticks = 3u;
    EXPECT_CALL(cmd1, validate(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(cmd1, execute(_)).Times(0);
    u.executeRules();
    ASSERT_EQ(u.m_type.ticks, 4u);

    // Single rule and ticks matches rate
    u.m_type.ticks = 3u;
    EXPECT_CALL(cmd1, validate(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(cmd1, execute(_)).Times(1);
    u.executeRules();
    ASSERT_EQ(u.m_type.ticks, 4u);
}
