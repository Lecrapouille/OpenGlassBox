#include "main.hpp"

#define protected public
#define private public
#  include "TestWorld.hpp"
#  include "OpenGlassBox/RuleValue.hpp"
#  include "OpenGlassBox/City.hpp"
#undef protected
#undef private

// -----------------------------------------------------------------------------
TEST(TestsValue, TestsValue)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit unit(keep<UnitType>("unit"), n, city);
    Resources locals, globals;
    RuleContext context;

    locals.addResource("oil", 5u);
    locals.setCapacity("oil", 50u);
    globals.addResource("money", 5u);
    globals.setCapacity("money", 50u);
    context.city = &city;
    context.unit = &unit;
    context.locals = &locals;
    context.globals = &globals;
    context.u = context.v = 4u;
    context.radius = 1.0;

    //
    RuleValueGlobal g(Resource("money"));
    EXPECT_EQ(g.get(context), 5u);

    g.add(context, 10u);
    EXPECT_EQ(g.get(context), 15u);
    EXPECT_EQ(globals.getAmount("money"), 15u);
    EXPECT_EQ(globals.getCapacity("money"), 50u);

    g.remove(context, 5u);
    EXPECT_EQ(g.get(context), 10u);
    EXPECT_EQ(globals.getAmount("money"), 10u);
    EXPECT_EQ(globals.getCapacity("money"), 50u);

    EXPECT_EQ(g.capacity(context), 50u);
    EXPECT_EQ(g.get(context), 10u);
    EXPECT_EQ(globals.getAmount("money"), 10u);
    EXPECT_EQ(globals.getCapacity("money"), 50u);

    //
    RuleValueLocal l(Resource("oil"));
    EXPECT_EQ(l.get(context), 5u);

    l.add(context, 10u);
    EXPECT_EQ(l.get(context), 15u);
    EXPECT_EQ(locals.getAmount("oil"), 15u);
    EXPECT_EQ(locals.getCapacity("oil"), 50u);

    l.remove(context, 5u);
    EXPECT_EQ(l.get(context), 10u);
    EXPECT_EQ(locals.getAmount("oil"), 10u);
    EXPECT_EQ(locals.getCapacity("oil"), 50u);

    EXPECT_EQ(l.capacity(context), 50u);
    EXPECT_EQ(l.get(context), 10u);
    EXPECT_EQ(locals.getAmount("oil"), 10u);
    EXPECT_EQ(locals.getCapacity("oil"), 50u);

    //
    MapType map_type("water");
    map_type.capacity = 50u;
    Map& map = city.addMap(map_type);
    map.setResource(context.u, context.v, 5u);

    RuleValueMap m("water");
    EXPECT_EQ(m.get(context), 5u);

#if 0
    // FIXME not sure of ALL VALUES
    m.add(context, 10u);
    EXPECT_EQ(m.get(context), 20u);
    EXPECT_EQ(map.getResource(context.u, context.v), 5u);
    EXPECT_EQ(map.getCapacity(/*context.u, context.v*/), 50u);

    m.remove(context, 5u);
    EXPECT_EQ(m.get(context), 15u);
    EXPECT_EQ(map.getResource(context.u, context.v), 5u);
    EXPECT_EQ(map.getCapacity(/*context.u, context.v*/), 50u);

    EXPECT_EQ(m.capacity(context), 50u);
    EXPECT_EQ(m.get(context), 15u);
    EXPECT_EQ(map.getResource(context.u, context.v), 5u);
    EXPECT_EQ(map.getCapacity(/*context.u, context.v*/), 50u);
#endif
}

// -----------------------------------------------------------------------------
//! \brief A map value read over a radius sums several cells, so what it is
//! compared against has to be the capacity of the same cells. Comparing a whole
//! footprint against one cell kept "map Pollution add 1" from ever validating
//! in a neighbourhood that already held some, and a rule is all or nothing.
TEST(TestsValue, MapCapacityCoversTheWholeRadius)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    Resources locals, globals;
    RuleContext context;
    context.city = &city;
    context.locals = &locals;
    context.globals = &globals;
    context.u = context.v = 4;

    MapType type("pollution");
    type.capacity = 10u;
    Map& map = city.addMap(type);

    RuleValueMap value("pollution");

    // One cell: unchanged.
    context.radius = 0u;
    EXPECT_EQ(value.capacity(context), 10u);

    // A cross of five cells holds five times as much.
    context.radius = 1u;
    uint32_t const cells = map.cellsInRadius(context.u, context.v,
                                             context.radius, city.region());
    EXPECT_EQ(cells, 5u);
    EXPECT_EQ(value.capacity(context), 5u * 10u);

    // Filling four of them out of five leaves room, and the rule may fire.
    map.setResource(3, 4, 10u);
    map.setResource(5, 4, 10u);
    map.setResource(4, 3, 10u);
    map.setResource(4, 5, 10u);
    EXPECT_EQ(value.get(context), 40u);
    EXPECT_LT(value.get(context), value.capacity(context));

    // Full: it may not.
    map.setResource(4, 4, 10u);
    EXPECT_EQ(value.get(context), 50u);
    EXPECT_EQ(value.get(context), value.capacity(context));
}

// -----------------------------------------------------------------------------
//! \brief The script has no syntax to cap a global stock, so a treasury the
//! ruleset never mentioned is unbounded rather than full.
TEST(TestsValue, UndeclaredGlobalIsUnbounded)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    Resources locals;
    RuleContext context;
    context.city = &city;
    context.locals = &locals;
    context.globals = &city.globals();

    RuleValueGlobal money(Resource("Money"));
    EXPECT_EQ(money.get(context), 0u);
    EXPECT_EQ(money.capacity(context), Resource::MAX_CAPACITY);
    EXPECT_LT(money.get(context), money.capacity(context));

    money.add(context, 3u);
    EXPECT_EQ(money.get(context), 3u);
    EXPECT_LT(money.get(context), money.capacity(context));
}
