#include "main.hpp"

#define protected public
#define private public
#  include "OpenGlassBox/RuleValue.hpp"
#  include "OpenGlassBox/City.hpp"
#undef protected
#undef private

// -----------------------------------------------------------------------------
TEST(TestsValue, TestsValue)
{
    City city("Paris", 8u, 8u);
    Node n(42u, Vector3f(1.0f, 2.0f, 3.0f));
    Unit unit(UnitType("unit"), n, city);
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
