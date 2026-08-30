#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/RuleCommand.hpp"
#  include "OpenGlassBox/City.hpp"
#  include "OpenGlassBox/SimulationClock.hpp"
#undef protected
#undef private

// -----------------------------------------------------------------------------
// For testing City::update()
class MockIRuleValue: public IRuleValue
{
public:

    MockIRuleValue() : IRuleValue() {}
    MOCK_METHOD(uint32_t, get,(RuleContext&), (override));
    MOCK_METHOD(uint32_t, getCapacity, (RuleContext&), (override));
    MOCK_METHOD(void, add,(RuleContext&, uint32_t), (override));
    MOCK_METHOD(void, remove,(RuleContext&, uint32_t), (override));
    MOCK_METHOD(Name const&, getTypeName, (), (const, override));
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
    RuleCommandAgent ra(keep<AgentType>("Worker", 1.0f, 2u, 0xFFFFFF), "home", r);
    ASSERT_STREQ(ra.getAgentType().name.c_str(), "Worker");
    ASSERT_EQ(ra.getAgentType().speed, 1.0f);
    ASSERT_EQ(ra.getAgentType().radius, 2u);
    ASSERT_EQ(ra.getAgentType().color, 0xFFFFFFu);
    ASSERT_STREQ(ra.getTarget().c_str(), "home");
    ASSERT_EQ(ra.getResources().m_bin.size(), 1u);
    ASSERT_STREQ(ra.getResources().m_bin[0].m_type.c_str(), "oil");
    ASSERT_EQ(ra.getResources().m_bin[0].m_amount, 5u);
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
    EXPECT_CALL(target, getCapacity(_)).Times(1).WillOnce(Return(10u));
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(10u));
    EXPECT_CALL(target, getCapacity(_)).Times(1).WillOnce(Return(5u));
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(1);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
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
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(1);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
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
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
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
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(2u));
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
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
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), true);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(1).WillOnce(Return(10u));
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    ASSERT_EQ(cmd.validate(context), false);

    //
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    cmd.execute(context);
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandAgent)
{
    RuleContext context;
    MockIRuleValue target;
    Resources r; r.addResource("oil", 5u);
    RuleCommandAgent cmd(keep<AgentType>("Worker", 1.0f, 2u, 0xFFFFFF), "home", r);

    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    // An empty context carries no Building to spawn the Agent from.
    ASSERT_EQ(cmd.validate(context), false);

    Resources locals, globals;
    TestWorld cityWorld("Paris", 2u, 2u);
    City& city = cityWorld.city;
    Path& p1 = city.addPath(keep<PathType>("Road"));
    Node& n1 = p1.addNode(Vector3f(0.0f, 0.0f, 3.0f));
    Node& n2 = p1.addNode(Vector3f(2.0f, 0.0f, 3.0f));
    Building building(keep<BuildingType>("house"), n1, city);
    context.city = &city;
    context.building = &building;
    context.locals = &locals;
    context.globals = &globals;
    context.cell.u = context.cell.v = 0u;
    context.radius = 1.0;

    // No ways linked to the node => no agent created (else ill-formed
    // simulation since Agents cannot travel).
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    EXPECT_EQ(city.getAgents().size(), 0u);
    cmd.execute(context);
    EXPECT_EQ(city.getAgents().size(), 0u);
    cmd.execute(context);
    EXPECT_EQ(city.getAgents().size(), 0u);

    // Add ways => can execute command => agents created
    p1.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);
    EXPECT_CALL(target, add(_,_)).Times(0);
    EXPECT_CALL(target, remove(_,_)).Times(0);
    EXPECT_CALL(target, get(_)).Times(0);
    EXPECT_CALL(target, getCapacity(_)).Times(0);
    cmd.execute(context);
    EXPECT_EQ(city.getAgents().size(), 1u);
    cmd.execute(context);
    EXPECT_EQ(city.getAgents().size(), 2u);
}

// -----------------------------------------------------------------------------
// A Rule may send an Agent to a Building whose type has another name, as long as
// that name is in its targets. This is what lets a ruleset give a house several
// tiers: Shack, House and Villa all answer to "Home", and none of them is called
// "Home". The command used to compare the name of the type first and refused
// every such delivery, while the router accepted it.
TEST(TestsCommand, RuleCommandAgentTargetIsNotTheTypeName)
{
    // Declared before the world so that they outlive every Building using them.
    BuildingType senderType("Factory");
    BuildingType receiverType("Villa");
    receiverType.targets.emplace_back("Home");
    receiverType.resources.setCapacity("People", 4u);

    TestWorld cityWorld("Paris", 4u, 4u);
    City& city = cityWorld.city;
    Path& road = city.addPath(keep<PathType>("Road"));
    Node& n1 = road.addNode(Vector3f(0.0f, 0.0f, 0.0f));
    Node& n2 = road.addNode(Vector3f(2.0f, 0.0f, 0.0f));
    road.addSegment(keep<SegmentType>("Dirt", 0xAAAAAA), n1, n2);

    Building& sender = city.addBuilding(senderType, n1);
    sender.getResources().setCapacity("People", 8u);
    sender.getResources().addResource("People", 4u);

    Resources load;
    load.addResource("People", 1u);
    RuleCommandAgent cmd(
        keep<AgentType>("Worker", 1.0f, 1u, 0xFFFFFF), "Home", load);

    Resources globals;
    RuleContext context;
    context.city = &city;
    context.building = &sender;
    context.locals = &(sender.getResources());
    context.globals = &globals;

    // Nothing answers to "Home" yet.
    ASSERT_FALSE(cmd.validate(context));

    // A Villa is not called Home, but it accepts Home.
    Building& villa = city.addBuilding(receiverType, n2);
    ASSERT_TRUE(villa.accepts("Home", load));
    ASSERT_TRUE(cmd.validate(context));

    // A full Villa refuses the load again, which is the only reason left to say
    // no: the room, not the name.
    villa.getResources().addResource("People", 4u);
    ASSERT_FALSE(cmd.validate(context));
}

// -----------------------------------------------------------------------------
TEST(TestsCommand, RuleCommandHour)
{
    SimulationClock clock(20u);
    RuleContext context;
    context.clock = &clock;

    RuleCommandHour day(8u, 18u);
    ASSERT_FALSE(day.validate(context));

    for (uint32_t i = 0u; i < 8u * 60u * 20u; ++i)
        clock.tick();

    ASSERT_TRUE(day.validate(context));
    ASSERT_EQ(day.getDescription(), std::string("Hour between 8 and 18"));

    RuleCommandHour night(22u, 6u);
    ASSERT_FALSE(night.validate(context));
}
