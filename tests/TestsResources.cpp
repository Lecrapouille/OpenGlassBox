#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/Resources.hpp"
#undef protected
#undef private

TEST(TestsResources, Constructor)
{
    Resources house;

    ASSERT_EQ(house.m_bin.size(), 0u);
    ASSERT_EQ(house.isEmpty(), true);
}

TEST(TestsResources, Nomila)
{
    Resources house;

    // Add a new resource
    Resource& r1 = house.addResource("people", 0u);
    ASSERT_EQ(house.m_bin.size(), 1u);
    ASSERT_STREQ(r1.type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.isEmpty(), true);

    // Add a new resource
    Resource& r2 = house.addResource("car", 2u);
    ASSERT_EQ(house.m_bin.size(), 2u);
    ASSERT_STREQ(r2.type().c_str(), "car");
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[1].type().c_str(), "car");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 2u);
    ASSERT_EQ(house.isEmpty(), false);

    // Add a new resource
    Resource& r3 = house.addResource("car", 8u);
    ASSERT_EQ(house.m_bin.size(), 2u);
    ASSERT_STREQ(r3.type().c_str(), "car");
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[1].type().c_str(), "car");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 10u);
    ASSERT_EQ(house.isEmpty(), false);

    // Remove resource "poeple"
    bool res = house.removeResource("car", 5u);
    ASSERT_EQ(res, true);
    ASSERT_EQ(house.m_bin.size(), 2u);
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[1].type().c_str(), "car");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 5u);
    ASSERT_EQ(house.isEmpty(), false);

    // Change capacity
    ASSERT_EQ(house.getCapacity("car"), Resource::MAX_CAPACITY);
    house.setCapacity("car", 2u);
    ASSERT_EQ(house.getCapacity("car"), 2u);
    ASSERT_EQ(house.getAmount("car"), 2u);

    // Change all capacities
    Resources caps;
    caps.setCapacity("car", 20u);
    caps.setCapacity("people", 10u);
    ASSERT_EQ(caps.m_bin.size(), 2u);
    ASSERT_STREQ(caps.m_bin[0].type().c_str(), "car");
    ASSERT_STREQ(caps.m_bin[1].type().c_str(), "people");
    ASSERT_EQ(caps.getCapacity("car"), 20u);
    ASSERT_EQ(caps.getCapacity("people"), 10u);

    ASSERT_EQ(house.getCapacity("car"), 2u);
    ASSERT_EQ(house.getCapacity("people"), Resource::MAX_CAPACITY);
    house.setCapacities(caps);
    ASSERT_EQ(house.getCapacity("car"), 20u);
    ASSERT_EQ(house.getCapacity("people"), 10u);

    // Change all toAdd
    Resources toAdd;
    toAdd.addResource("car", 5u);
    toAdd.addResource("people", 2u);
    ASSERT_EQ(toAdd.getAmount("car"), 5u);
    ASSERT_EQ(toAdd.getAmount("people"), 2u);

    ASSERT_EQ(house.getAmount("car"), 2u);
    ASSERT_EQ(house.getAmount("people"), 0u);
    house.addResources(toAdd);
    ASSERT_EQ(house.getAmount("car"), 7u);
    ASSERT_EQ(house.getAmount("people"), 2u);
    house.removeResources(toAdd);
    ASSERT_EQ(house.getAmount("car"), 2u);
    ASSERT_EQ(house.getAmount("people"), 0u);
}

TEST(TestsResources, PathologicalCases)
{
    Resources house;

    bool res = house.removeResource("car", 5u);
    ASSERT_EQ(res, false);

    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getCapacity("foo"), 0u);
}

/*


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
*/
