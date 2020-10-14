#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/Unit.hpp"
#  include "src/Core/City.hpp"
#undef protected
#undef private

// -----------------------------------------------------------------------------
TEST(TestsUnit, Constructor)
{
    City city("Paris", 4u, 4u);
    Node node(42u, Vector3f(3.0f, 4.0f, 5.0f));
    UnitType unit_type("unit");
    unit_type.color = 42u;
    unit_type.radius = 2u;
    unit_type.resources.addResource("car", 5u);
    unit_type.targets.push_back("foo");

    // Constructor
    Unit u(unit_type, node, city);

    // Check initial values (member variables).
    ASSERT_STREQ(u.m_type.name.c_str(), "unit");
    ASSERT_EQ(u.m_type.color, 42u);
    ASSERT_EQ(u.m_type.radius, 2u);
    ASSERT_EQ(u.m_type.resources.m_bin.size(), 1u);
    ASSERT_STREQ(u.m_type.resources.m_bin[0].type().c_str(), "car");
    ASSERT_EQ(u.m_type.resources.m_bin[0].m_amount, 5u);
    ASSERT_EQ(u.m_type.rules.size(), 0u);
    ASSERT_EQ(u.m_type.targets.size(), 1u);
    ASSERT_STREQ(u.m_type.targets[0].c_str(), "foo");
    ASSERT_EQ(&u.m_node, &node);
    ASSERT_EQ(u.m_resources.m_bin.size(), 1u);
    ASSERT_STREQ(u.m_resources.m_bin[0].type().c_str(), "car");
    ASSERT_EQ(u.m_resources.m_bin[0].m_amount, 5u);
    ASSERT_EQ(u.m_context.city, &city);
    ASSERT_EQ(u.m_context.unit, &u);
    ASSERT_EQ(u.m_context.locals, &u.m_resources);
    ASSERT_EQ(u.m_context.globals, &city.globals());
    ASSERT_EQ(u.m_context.u, 1u);
    ASSERT_EQ(u.m_context.v, 2u);
    ASSERT_EQ(u.m_context.radius, 2u);
    ASSERT_EQ(u.m_ticks, 0u);

    // Check initial values (getter methods).
    ASSERT_STREQ(u.type().c_str(), "unit");
    ASSERT_EQ(u.color(), 42u);
    ASSERT_EQ(&(u.node()), &node);
    ASSERT_EQ(int32_t(u.position().x), int32_t(node.position().x));
    ASSERT_EQ(int32_t(u.position().y), int32_t(node.position().y));
    ASSERT_EQ(int32_t(u.position().z), int32_t(node.position().z));
    ASSERT_EQ(u.m_type.resources.getAmount("car"), 5u);
    ASSERT_EQ(u.m_resources.m_bin.size(), 1u);
    ASSERT_EQ(u.resources().getAmount("car"), 5u);
}

// -----------------------------------------------------------------------------
TEST(TestsUnit, Accept)
{
    City city("Paris", 4u, 4u);
    Node node(42u, Vector3f(3.0f, 4.0f, 5.0f));
    UnitType unit_type("unit");
    unit_type.resources.addResource("car", 5u);
    unit_type.targets.push_back("foo");
    Unit u(unit_type, node, city);

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
}

// -----------------------------------------------------------------------------
class MockIRuleCommand: public IRuleCommand
{
public:

    MockIRuleCommand() = default;
    ~MockIRuleCommand() = default;
    MOCK_METHOD(bool, validate, (RuleContext&), (override));
    MOCK_METHOD(void, execute, (RuleContext&), (override));
    MOCK_METHOD(std::string, type, (), (override));
};

class MockRuleUnit: public RuleUnit
{
public:

    MockRuleUnit(RuleUnitType const& type)
        : RuleUnit(type)
    {}
    ~MockRuleUnit() = default;

    MOCK_METHOD(bool, execute, (RuleContext&), (override));
};

// -----------------------------------------------------------------------------
TEST(TestsUnit, ExecuteRules)
{
    City city("Paris", 4u, 4u);
    Node node(42u, Vector3f(3.0f, 4.0f, 5.0f));

    // OnFail() callback is nullptr
    MockIRuleCommand cmd1;
    RuleUnitType ruleunit_type1("ru1");
    ruleunit_type1.rate = 4u;
    ruleunit_type1.onFail = nullptr;
    ruleunit_type1.commands.push_back(&cmd1);
    RuleUnit rule1(ruleunit_type1);
    UnitType unit_type1("unit1");
    unit_type1.rules.push_back(&rule1);
    Unit u1(unit_type1, node, city);

    // Single rule to run but tocks does not match yet rate
    u1.m_ticks = 2u;
    EXPECT_CALL(cmd1, validate(_)).Times(0);
    EXPECT_CALL(cmd1, execute(_)).Times(0);
    u1.executeRules();
    ASSERT_EQ(u1.m_ticks, 3u);

    // Single rule and ticks matches rate
    u1.m_ticks = 3u;
    EXPECT_CALL(cmd1, validate(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(cmd1, execute(_)).Times(0);
    u1.executeRules();
    ASSERT_EQ(u1.m_ticks, 4u);

    // Single rule and ticks matches rate
    u1.m_ticks = 3u;
    EXPECT_CALL(cmd1, validate(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(cmd1, execute(_)).Times(1);
    u1.executeRules();
    ASSERT_EQ(u1.m_ticks, 4u);

    // OnFail() callback
    MockIRuleCommand cmd2;
    RuleUnitType ruleunit_type2("ru2");
    MockRuleUnit onFail(RuleUnitType("ru3"));
    ruleunit_type2.rate = 4u;
    ruleunit_type2.onFail = &onFail;
    ruleunit_type2.commands.push_back(&cmd2);
    RuleUnit rule2(ruleunit_type2);
    UnitType unit_type2("unit2");
    unit_type2.rules.push_back(&rule2);
    Unit u2(unit_type2, node, city);

    // Single rule and ticks matches rate
    u2.m_ticks = 3u;
    EXPECT_CALL(cmd2, validate(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(cmd2, execute(_)).Times(0);
    EXPECT_CALL(onFail, execute(_)).Times(1);
    u2.executeRules();
    ASSERT_EQ(u2.m_ticks, 4u);
}
