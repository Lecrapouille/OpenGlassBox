#include "main.hpp"

#define protected public
#define private public
#  include "src/Resources.hpp"
#undef protected
#undef private

TEST(TestsResource, NominalCase)
{
    Resources house;

    house.addResource("pepole", 8u);
    ASSERT_EQ(house.getAmount("pepole"), 8u);

    house.addResource("pepole", 8u);
    ASSERT_EQ(house.getAmount("pepole"), 16u);

    ASSERT_EQ(house.hasResource("pepole"), true);
    ASSERT_EQ(house.hasResource("foo"), false);

    Resources house2;
    house2.addResources(house);
    ASSERT_EQ(house2.getAmount("pepole"), 16u);
    house.transferResourcesTo(house2);
    ASSERT_EQ(house.getAmount("pepole"), 0u);
    ASSERT_EQ(house2.getAmount("pepole"), 32u);
}
