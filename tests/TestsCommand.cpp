#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/RuleCommand.hpp"
#  include "src/Core/City.hpp"
#undef protected
#undef private

// -----------------------------------------------------------------------------
// For testing City::update()
class MockIRuleValue: public IRuleValue
{
public:

    MockIRuleValue() : IRuleValue() {}
    MOCK_METHOD(uint32_t, get,(RuleContext&), (override));
    MOCK_METHOD(uint32_t, capacity,(RuleContext&), (override));
    MOCK_METHOD(void, add,(RuleContext&, uint32_t), (override));
    MOCK_METHOD(void, remove,(RuleContext&, uint32_t), (override));
    MOCK_METHOD(std::string const&, type, (), (const override));
};

// -----------------------------------------------------------------------------
TEST(TestsCommand, Constructor)
{
    //
    MockIRuleValue target;
    RuleCommandAdd rca(target, 5u);
    ASSERT_EQ(&rca.m_target, &target);
    ASSERT_EQ(rca.m_amount, 5u);

    //
    RuleCommandRemove rcr(target, 5u);
    ASSERT_EQ(&rcr.m_target, &target);
    ASSERT_EQ(rcr.m_amount, 5u);

    //
    RuleCommandTest rct(target, RuleCommandTest::Comparison::EQUALS, 5u);
    ASSERT_EQ(&rct.m_target, &target);
    ASSERT_EQ(rct.m_amount, 5u);
    ASSERT_EQ(rct.m_comparison, RuleCommandTest::Comparison::EQUALS);

    //
    Resources r; r.addResource("oil", 5u);
    RuleCommandAgent ra(AgentType("Worker", 1.0f, 2u, 0xFFFFFF), "home", r);
    ASSERT_STREQ(ra.name.c_str(), "Worker");
    ASSERT_EQ(ra.speed, 1.0f);
    ASSERT_EQ(ra.radius, 2u);
    ASSERT_EQ(ra.color, 0xFFFFFFu);
    ASSERT_STREQ(ra.m_target.c_str(), "home");
    ASSERT_EQ(ra.m_resources.m_bin.size(), 1u);
    ASSERT_STREQ(ra.m_resources.m_bin[0].m_type.c_str(), "oil");
    ASSERT_EQ(ra.m_resources.m_bin[0].m_amount, 5u);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandAdd)
{
    RuleContext context;
    MockIRuleValue target;
    RuleCommandAdd cmd(target, 5u);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(5u));
    EXPECT_CALL(target, capacity(_)).Times(1).WillOnce(Return(10u));
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(10u));
    EXPECT_CALL(target, capacity(_)).Times(1).WillOnce(Return(5u));
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(1);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    cmd.execute(context);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandRemove)
{
    RuleContext context;
    MockIRuleValue target;
    RuleCommandRemove cmd(target, 5u);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(10u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(1);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    cmd.execute(context);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandTestEqual)
{
    RuleContext context;
    MockIRuleValue target;
    RuleCommandTest cmd(target, RuleCommandTest::Comparison::EQUALS, 5u);
    ASSERT_EQ(cmd.m_comparison, RuleCommandTest::Comparison::EQUALS);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(5u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    cmd.execute(context);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandTestGreater)
{
    RuleContext context;
    MockIRuleValue target;
    RuleCommandTest cmd(target, RuleCommandTest::Comparison::GREATER, 5u);
    ASSERT_EQ(cmd.m_comparison, RuleCommandTest::Comparison::GREATER);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(10u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    cmd.execute(context);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandTestLess)
{
    RuleContext context;
    MockIRuleValue target;
    RuleCommandTest cmd(target, RuleCommandTest::Comparison::LESS, 5u);
    ASSERT_EQ(cmd.m_comparison, RuleCommandTest::Comparison::LESS);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(10u));
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    cmd.execute(context);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandAgent)
{
    RuleContext context;
    MockIRuleValue target;
    Resources r; r.addResource("oil", 5u);
    RuleCommandAgent cmd(AgentType("Worker", 1.0f, 2u, 0xFFFFFF), "home", r);

    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    Resources locals, globals;
    City city("Paris", 2u, 2u);
    Path& p1 = city.addPath(PathType("Road"));
    Node& n1 = p1.addNode(Vector3f(0.0f, 0.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(2.0f, 0.0f, 3.0f));
    Unit unit(UnitType("unit"), n1, city);
    context.city = &city;
    context.unit = &unit;
    context.locals = &locals;
    context.globals = &globals;
    context.u = context.v = 0u;
    context.radius = 1.0;

    // No ways linked to the node => no agent created (else ill-formed
    // simulation since Agents cannot travel).
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    EXPECT_EQ(city.agents().size(), 0u);
    cmd.execute(context);
    EXPECT_EQ(city.agents().size(), 0u);
    cmd.execute(context);
    EXPECT_EQ(city.agents().size(), 0u);

    // Add ways => can execute command => agents created
    p1.addWay(WayType("Dirt", 0xAAAAAA), n1, n2);
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, capacity(_)).Times(0);
    cmd.execute(context);
    EXPECT_EQ(city.agents().size(), 1u);
    cmd.execute(context);
    EXPECT_EQ(city.agents().size(), 2u);
}
