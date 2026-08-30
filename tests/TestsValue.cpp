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
    Building building(keep<BuildingType>("house"), n, city);
    Resources locals, globals;
    RuleContext context;

    locals.addResource("oil", 5u);
    locals.setCapacity("oil", 50u);
    globals.addResource("money", 5u);
    globals.setCapacity("money", 50u);
    context.city = &city;
    context.building = &building;
    context.locals = &locals;
    context.globals = &globals;
    context.cell.u = context.cell.v = 4u;
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

    EXPECT_EQ(g.getCapacity(context), 50u);
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

    EXPECT_EQ(l.getCapacity(context), 50u);
    EXPECT_EQ(l.get(context), 10u);
    EXPECT_EQ(locals.getAmount("oil"), 10u);
    EXPECT_EQ(locals.getCapacity("oil"), 50u);

    //
    LayerType layer_type("water");
    layer_type.capacity = 50u;
    Layer& layer = city.addLayer(layer_type);
    layer.setResource({ context.cell.u, context.cell.v }, 5u);

    RuleValueLayer m("water");
    EXPECT_EQ(m.get(context), 5u);

#if 0
    // FIXME not sure of ALL VALUES
    m.add(context, 10u);
    EXPECT_EQ(m.get(context), 20u);
    EXPECT_EQ(layer.getResource({ context.cell.u, context.cell.v }), 5u);
    EXPECT_EQ(layer.setCapacity(/*context.cell.u, context.cell.v*/), 50u);

    m.remove(context, 5u);
    EXPECT_EQ(m.get(context), 15u);
    EXPECT_EQ(layer.getResource({ context.cell.u, context.cell.v }), 5u);
    EXPECT_EQ(layer.setCapacity(/*context.cell.u, context.cell.v*/), 50u);

    EXPECT_EQ(m.getCapacity(context), 50u);
    EXPECT_EQ(m.get(context), 15u);
    EXPECT_EQ(layer.getResource({ context.cell.u, context.cell.v }), 5u);
    EXPECT_EQ(layer.setCapacity(/*context.cell.u, context.cell.v*/), 50u);
#endif
}

// -----------------------------------------------------------------------------
//! \brief A layer value read over a radius sums several cells, so what it is
//! compared against has to be the capacity of the same cells. Comparing a whole
//! footprint against one cell kept "layer Pollution add 1" from ever validating
//! in a neighbourhood that already held some, and a rule is all or nothing.
TEST(TestsValue, LayerCapacityCoversTheWholeRadius)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    City& city = cityWorld.city;
    Resources locals, globals;
    RuleContext context;
    context.city = &city;
    context.locals = &locals;
    context.globals = &globals;
    context.cell.u = context.cell.v = 4;

    LayerType type("pollution");
    type.capacity = 10u;
    Layer& layer = city.addLayer(type);

    RuleValueLayer value("pollution");

    // One cell: unchanged.
    context.radius = 0u;
    EXPECT_EQ(value.getCapacity(context), 10u);

    // A cross of five cells holds five times as much.
    context.radius = 1u;
    uint32_t const cells = layer.countCellsInRadius({ context.cell.u, context.cell.v },
                                             context.radius, city.getRegion());
    EXPECT_EQ(cells, 5u);
    EXPECT_EQ(value.getCapacity(context), 5u * 10u);

    // Filling four of them out of five leaves room, and the rule may fire.
    layer.setResource({ 3, 4 }, 10u);
    layer.setResource({ 5, 4 }, 10u);
    layer.setResource({ 4, 3 }, 10u);
    layer.setResource({ 4, 5 }, 10u);
    EXPECT_EQ(value.get(context), 40u);
    EXPECT_LT(value.get(context), value.getCapacity(context));

    // Full: it may not.
    layer.setResource({ 4, 4 }, 10u);
    EXPECT_EQ(value.get(context), 50u);
    EXPECT_EQ(value.get(context), value.getCapacity(context));
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
    context.globals = &city.getGlobals();

    RuleValueGlobal money(Resource("Money"));
    EXPECT_EQ(money.get(context), 0u);
    EXPECT_EQ(money.getCapacity(context), Resource::MAX_CAPACITY);
    EXPECT_LT(money.get(context), money.getCapacity(context));

    money.add(context, 3u);
    EXPECT_EQ(money.get(context), 3u);
    EXPECT_LT(money.get(context), money.getCapacity(context));
}

// -----------------------------------------------------------------------------
//! \brief A local stock belongs to the Building or the Agent the rule runs on. A
//! Layer runs its rules on cells of the map, and a cell owns nothing, so it
//! leaves RuleContext::locals empty. Reading it read a null pointer.
//!
//! The parser refuses "local" inside a layerRule, which closes the door for
//! scripts. This keeps it closed for a Rule built from C++.
TEST(TestsValue, LocalWithoutAnEntityReadsZero)
{
    TestWorld cityWorld("Paris", 8u, 8u);
    Resources globals;
    RuleContext context;
    context.city = &cityWorld.city;
    context.globals = &globals;
    context.locals = nullptr;

    RuleValueLocal people(Resource("People"));

    EXPECT_EQ(people.get(context), 0u);
    EXPECT_EQ(people.getCapacity(context), 0u);

    // Writing does nothing rather than crash, and reading still says zero.
    people.add(context, 5u);
    EXPECT_EQ(people.get(context), 0u);
    people.remove(context, 5u);
    EXPECT_EQ(people.get(context), 0u);
}
