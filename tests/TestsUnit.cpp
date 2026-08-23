#include "main.hpp"

#define protected public
#define private public
#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/RuleCommand.hpp"
#include "OpenGlassBox/Unit.hpp"
#include "TestWorld.hpp"
#undef protected
#undef private

//! \brief Small cells so that the node at (3, 4) lands on the grid cell (1, 2).
static SimulationConfig smallCells()
{
    SimulationConfig config;
    config.gridCellSize = 2.0f;
    return config;
}

// -----------------------------------------------------------------------------
//------------------------------------------------------------------------------
//! \brief Two buildings put up at the same instant must not run their rules on
//! the same tick, or the whole city leaves home at once at eight sharp. The
//! phase comes from the seed, so a run stays reproducible.
TEST(TestsUnit, RulesOfTwoUnitsDoNotFallOnTheSameTick)
{
    SimulationConfig config;
    config.randomSeed = 1234u;

    TestWorld cityWorld("Paris", 8u, 8u, Vector3f(0.0f, 0.0f, 0.0f), config);
    City& city = cityWorld.city;

    UnitType type("Home");
    type.rules.push_back(nullptr);

    std::vector<uint32_t> phases;
    for (uint32_t i = 0u; i < 6u; ++i)
    {
        Unit const& unit = city.addUnit(type, Vector3f(float(i), 0.0f, 0.0f));
        phases.push_back(unit.ticks());
        // One game hour at most, so that a rule counted in days cannot fire
        // the moment the building goes up.
        ASSERT_LT(unit.ticks(), 60u * config.ticksPerMinute);
    }

    // Not all the same. Two out of six may collide; six identical values would
    // mean no phase at all.
    uint32_t identical = 0u;
    for (uint32_t const phase : phases)
        identical += uint32_t(phase == phases[0]);
    ASSERT_LT(identical, phases.size());

    // Same seed, same phases: a bug reported on a run can be replayed.
    TestWorld twin("Paris", 8u, 8u, Vector3f(0.0f, 0.0f, 0.0f), config);
    for (uint32_t i = 0u; i < phases.size(); ++i)
    {
        Unit const& unit =
            twin.city.addUnit(type, Vector3f(float(i), 0.0f, 0.0f));
        ASSERT_EQ(unit.ticks(), phases[i]);
    }

    // Another seed, another set.
    config.randomSeed = 99u;
    TestWorld other("Paris", 8u, 8u, Vector3f(0.0f, 0.0f, 0.0f), config);
    uint32_t same = 0u;
    for (uint32_t i = 0u; i < phases.size(); ++i)
    {
        Unit const& unit =
            other.city.addUnit(type, Vector3f(float(i), 0.0f, 0.0f));
        same += uint32_t(unit.ticks() == phases[i]);
    }
    ASSERT_LT(same, phases.size());
}

TEST(TestsUnit, Constructor)
{
    TestWorld cityWorld(
        "Paris", 4u, 4u, Vector3f(0.0f, 0.0f, 0.0f), smallCells());
    City& city = cityWorld.city;
    Node node(42u, Vector3f(3.0f, 4.0f, 5.0f));
    UnitType unit_type("unit");
    unit_type.color = 42u;
    unit_type.radius = 2u;
    unit_type.resources.addResource("car", 5u);
    unit_type.targets.emplace_back("foo");

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
    ASSERT_EQ(u.m_node, &node);
    ASSERT_EQ(u.m_resources.m_bin.size(), 1u);
    ASSERT_STREQ(u.m_resources.m_bin[0].type().c_str(), "car");
    ASSERT_EQ(u.m_resources.m_bin[0].m_amount, 5u);
    ASSERT_EQ(u.m_context.city, &city);
    ASSERT_EQ(u.m_context.unit, &u);
    ASSERT_EQ(u.m_context.locals, &u.m_resources);
    ASSERT_EQ(u.m_context.globals, &city.globals());
    ASSERT_EQ(u.m_context.u, 1u); // node.position.x / city.gridCellSize()
    ASSERT_EQ(u.m_context.v, 2u); // node.position.y / city.gridCellSize()
    ASSERT_EQ(u.m_context.radius, 2u);
    ASSERT_EQ(u.m_ticks, 0u);

    // Check initial values (getter methods).
    ASSERT_STREQ(u.type().c_str(), "unit");
    ASSERT_EQ(u.color(), 42u);
    ASSERT_EQ(u.node(), &node);
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
    TestWorld cityWorld(
        "Paris", 4u, 4u, Vector3f(0.0f, 0.0f, 0.0f), smallCells());
    City& city = cityWorld.city;
    Node node(42u, Vector3f(3.0f, 4.0f, 5.0f));
    UnitType unit_type("unit");
    unit_type.resources.addResource("car", 5u);
    unit_type.targets.emplace_back("foo");
    Unit u(unit_type, node, city);

    // Check accept
    Resources r0;
    Resources r1;
    r1.addResource("car", 5u);
    Resources r2;
    r2.addResource("oil", 5u);

    ASSERT_EQ(u.accepts("foo", r0), false);
    ASSERT_EQ(u.accepts("foo", r1), true);
    ASSERT_EQ(u.accepts("bar", r1), false);
    ASSERT_EQ(u.accepts("foo", r2), false);

    r2.addResource("car", 5u);
    ASSERT_EQ(u.accepts("foo", r2), true);
}

// -----------------------------------------------------------------------------
//! \brief A building is shut outside the hours its rules keep, and saying so is
//! what keeps a sleeping shop from reading as a broken one.
TEST(TestsUnit, OpeningHoursComeFromTheRulesOfTheBuilding)
{
    TestWorld cityWorld("Paris", 4u, 4u);
    City& city = cityWorld.city;
    Node node(1u, Vector3f(0.0f, 0.0f, 0.0f));

    RuleCommandHour morning(8u, 10u);
    RuleUnitType commuteType("SendPeopleToWork");
    commuteType.commands.push_back(&morning);
    RuleUnit commute(commuteType);

    RuleCommandHour afternoon(14u, 18u);
    RuleUnitType shoppingType("ShopForGoods");
    shoppingType.commands.push_back(&afternoon);
    RuleUnit shopping(shoppingType);

    UnitType homeType("Home");
    homeType.rules.push_back(&commute);
    homeType.rules.push_back(&shopping);
    Unit home(homeType, node, city);

    OpeningHours const hours = home.openingHours();
    ASSERT_TRUE(hours.bounded());

    // The two windows add up, and what lies between them is closed: the house
    // has nobody going anywhere at noon.
    ASSERT_FALSE(hours.isOpen(7u));
    ASSERT_TRUE(hours.isOpen(8u));
    ASSERT_TRUE(hours.isOpen(9u));
    ASSERT_FALSE(hours.isOpen(10u));
    ASSERT_FALSE(hours.isOpen(12u));
    ASSERT_TRUE(hours.isOpen(14u));
    ASSERT_TRUE(hours.isOpen(17u));
    ASSERT_FALSE(hours.isOpen(18u));
    ASSERT_FALSE(hours.isOpen(0u));

    ASSERT_EQ(hours.nextOpening(7u), 8u);
    ASSERT_EQ(hours.nextOpening(9u), 9u);
    ASSERT_EQ(hours.nextOpening(10u), 14u);
    // Past the last window the answer is on the next day.
    ASSERT_EQ(hours.nextOpening(20u), 8u);

    ASSERT_EQ(hours.closingAfter(8u), 9u);
    ASSERT_EQ(hours.closingAfter(14u), 17u);
    ASSERT_EQ(hours.closingAfter(12u), OpeningHours::NEVER);
}

// -----------------------------------------------------------------------------
//! \brief One rule with no timetable is enough to keep the doors open, and a
//! building with no rule at all was never shut to begin with.
TEST(TestsUnit, ABuildingWithoutATimetableNeverCloses)
{
    TestWorld cityWorld("Paris", 4u, 4u);
    City& city = cityWorld.city;
    Node node(1u, Vector3f(0.0f, 0.0f, 0.0f));

    UnitType bareType("Road");
    Unit bare(bareType, node, city);
    ASSERT_FALSE(bare.openingHours().bounded());
    ASSERT_TRUE(bare.openingHours().isOpen(3u));

    // A shop that sells at any hour, and sends its customers home at any hour,
    // keeps no office hours even though one of its rules does.
    RuleCommandHour lunch(12u, 14u);
    RuleUnitType servingType("ServeLunch");
    servingType.commands.push_back(&lunch);
    RuleUnit serving(servingType);

    RuleUnitType sellingType("SellGoods");
    RuleUnit selling(sellingType);

    UnitType shopType("Shop");
    shopType.rules.push_back(&serving);
    shopType.rules.push_back(&selling);
    Unit shop(shopType, node, city);

    ASSERT_FALSE(shop.openingHours().bounded());
    ASSERT_TRUE(shop.openingHours().isOpen(3u));
}

// -----------------------------------------------------------------------------
//! \brief Two windows on the same rule narrow each other down: the rule only
//! fires when both agree, so the shop opens on the overlap alone.
TEST(TestsUnit, TwoWindowsOnOneRuleAreAnIntersection)
{
    TestWorld cityWorld("Paris", 4u, 4u);
    City& city = cityWorld.city;
    Node node(1u, Vector3f(0.0f, 0.0f, 0.0f));

    RuleCommandHour wide(8u, 20u);
    RuleCommandHour narrow(12u, 14u);
    RuleUnitType ruleType("Lunch");
    ruleType.commands.push_back(&wide);
    ruleType.commands.push_back(&narrow);
    RuleUnit rule(ruleType);

    UnitType type("Restaurant");
    type.rules.push_back(&rule);
    Unit unit(type, node, city);

    OpeningHours const hours = unit.openingHours();
    ASSERT_TRUE(hours.bounded());
    ASSERT_FALSE(hours.isOpen(9u));
    ASSERT_TRUE(hours.isOpen(12u));
    ASSERT_TRUE(hours.isOpen(13u));
    ASSERT_FALSE(hours.isOpen(14u));
}

// -----------------------------------------------------------------------------
//! \brief A window given the other way round runs through midnight, the way
//! SimulationClock::hourBetween reads it.
TEST(TestsUnit, ATimetableCanRunThroughMidnight)
{
    TestWorld cityWorld("Paris", 4u, 4u);
    City& city = cityWorld.city;
    Node node(1u, Vector3f(0.0f, 0.0f, 0.0f));

    RuleCommandHour night(22u, 6u);
    RuleUnitType ruleType("NightShift");
    ruleType.commands.push_back(&night);
    RuleUnit rule(ruleType);

    UnitType type("Bakery");
    type.rules.push_back(&rule);
    Unit unit(type, node, city);

    OpeningHours const hours = unit.openingHours();
    ASSERT_TRUE(hours.isOpen(23u));
    ASSERT_TRUE(hours.isOpen(0u));
    ASSERT_TRUE(hours.isOpen(5u));
    ASSERT_FALSE(hours.isOpen(6u));
    ASSERT_FALSE(hours.isOpen(21u));
    ASSERT_EQ(hours.nextOpening(21u), 22u);
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

    explicit MockRuleUnit(RuleUnitType const& type) : RuleUnit(type) {}
    ~MockRuleUnit() = default;

    MOCK_METHOD(bool, execute, (RuleContext&), (override));
};

// -----------------------------------------------------------------------------
TEST(TestsUnit, ExecuteRules)
{
    TestWorld cityWorld(
        "Paris", 4u, 4u, Vector3f(0.0f, 0.0f, 0.0f), smallCells());
    City& city = cityWorld.city;
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
    MockRuleUnit onFail(keep<RuleUnitType>("ru3"));
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
