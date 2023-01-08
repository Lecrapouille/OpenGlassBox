#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "OpenGlassBox/Resources.hpp"
#undef protected
#undef private

// Check the collection is empty.
TEST(TestsResources, Constructor)
{
    Resources house;

    ASSERT_EQ(house.m_bin.size(), 0u);
    ASSERT_EQ(house.isEmpty(), true);
}

// Check if request on empty collection does nothing.
TEST(TestsResources, EmptyCollection)
{
    Resources house;

    ASSERT_EQ(house.findResource("people"), nullptr);
    ASSERT_EQ(house.hasResource("people"), false);

    house.removeResource("people", 10u);
    ASSERT_EQ(house.m_bin.size(), 0u);
    ASSERT_EQ(house.isEmpty(), true);

    ASSERT_EQ(house.canAddSomeResources(house), false);
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getCapacity("people"), 0u);

    house.removeResources(house);
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getCapacity("people"), 0u);
}

TEST(TestsResources, Nominal)
{
    Resources house;

    // Add a new resource "people". Check if the new resource has been added.
    Resource& r1 = house.addResource("people", 0u);
    ASSERT_EQ(house.m_bin.size(), 1u);
    ASSERT_STREQ(r1.type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getCapacity("people"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.isEmpty(), true);
    ASSERT_EQ(house.canAddSomeResources(house), false);

    // Add a new resource "car". Check if the new resource has been added.
    Resource& r2 = house.addResource("car", 2u);
    ASSERT_EQ(house.m_bin.size(), 2u);
    ASSERT_STREQ(r2.type().c_str(), "car");
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[1].type().c_str(), "car");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 2u);
    ASSERT_EQ(house.getCapacity("people"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.getCapacity("car"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.isEmpty(), false);
    ASSERT_EQ(house.canAddSomeResources(house), false);

    // Check self transfer of resources does nothing
    house.removeResources(house);
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 2u);
    house.addResources(house);
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 2u);

    // Add a new resource "car". Check if the amount of resource has changed.
    Resource& r3 = house.addResource("car", 8u);
    ASSERT_EQ(house.m_bin.size(), 2u);
    ASSERT_STREQ(r3.type().c_str(), "car");
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[1].type().c_str(), "car");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 10u);
    ASSERT_EQ(house.getCapacity("people"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.getCapacity("car"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.isEmpty(), false);

    // Reduce amount resource "people". Check if the amount of resource has
    // changed.
    bool res = house.removeResource("car", 5u);
    ASSERT_EQ(res, true);
    ASSERT_EQ(house.m_bin.size(), 2u);
    ASSERT_STREQ(house.m_bin[0].type().c_str(), "people");
    ASSERT_STREQ(house.m_bin[1].type().c_str(), "car");
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("car"), 5u);
    ASSERT_EQ(house.getCapacity("people"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.getCapacity("car"), Resource::MAX_CAPACITY);
    ASSERT_EQ(house.isEmpty(), false);

    // Change capacity for resource "car". Check if the amount of resource has
    // been changed.
    ASSERT_EQ(house.getCapacity("car"), Resource::MAX_CAPACITY);
    house.setCapacity("car", 2u);
    ASSERT_EQ(house.getCapacity("car"), 2u);
    ASSERT_EQ(house.getAmount("car"), 2u);

    // Change capacity for all resources in the collection. Check if the amount
    // of resource has been changed or resources created.
    Resources caps;
    caps.setCapacity("car", 20u);
    caps.setCapacity("people", 10u);
    caps.setCapacity("trash", 10u);
    ASSERT_EQ(caps.m_bin.size(), 3u);
    ASSERT_STREQ(caps.m_bin[0].type().c_str(), "car");
    ASSERT_STREQ(caps.m_bin[1].type().c_str(), "people");
    ASSERT_STREQ(caps.m_bin[2].type().c_str(), "trash");
    ASSERT_EQ(caps.getCapacity("car"), 20u);
    ASSERT_EQ(caps.getCapacity("people"), 10u);
    ASSERT_EQ(caps.getCapacity("trash"), 10u);

    ASSERT_EQ(house.m_bin.size(), 2u); // before
    ASSERT_EQ(house.getCapacity("car"), 2u);
    ASSERT_EQ(house.getCapacity("people"), Resource::MAX_CAPACITY);
    house.setCapacities(caps);
    ASSERT_EQ(house.m_bin.size(), 3u); // after
    ASSERT_EQ(house.getCapacity("car"), 20u);
    ASSERT_EQ(house.getCapacity("people"), 10u);
    ASSERT_EQ(house.getCapacity("trash"), 10u);

    // Change amount for all resources in the collection. Check if the amount
    // of resource has been changed or resources created.
    Resources toAdd;
    toAdd.addResource("car", 5u);
    toAdd.addResource("people", 2u);
    toAdd.addResource("trash", 10u);
    ASSERT_EQ(toAdd.getAmount("car"), 5u);
    ASSERT_EQ(toAdd.getAmount("people"), 2u);
    ASSERT_EQ(toAdd.getAmount("trash"), 10u);

    ASSERT_EQ(house.getAmount("car"), 2u); // before
    ASSERT_EQ(house.getAmount("people"), 0u);
    house.addResources(toAdd);
    ASSERT_EQ(house.getAmount("car"), 7u); // after
    ASSERT_EQ(house.getAmount("people"), 2u);
    ASSERT_EQ(house.getAmount("trash"), 10u);
    house.removeResources(toAdd);
    ASSERT_EQ(house.getAmount("car"), 2u);
    ASSERT_EQ(house.getAmount("people"), 0u);
    ASSERT_EQ(house.getAmount("trash"), 0u);
}

TEST(TestsResources, canAddSomeResources)
{
    Resources house;
    Resources foo;

    bool ret = house.canAddSomeResources(foo);
    ASSERT_EQ(ret, false);

    foo.addResource("car", 5u);
    ret = house.canAddSomeResources(foo);
    ASSERT_EQ(ret, false);

    house.addResource("car", 5u);
    ret = house.canAddSomeResources(foo);
    ASSERT_EQ(ret, true);

    house.setCapacity("car", 5u);
    ret = house.canAddSomeResources(foo);
    ASSERT_EQ(ret, false);

    ret = house.canAddSomeResources(house);
    ASSERT_EQ(ret, false);
}

TEST(TestsResources, transferResourcesTo)
{
    Resources house;
    Resources foo;

    house.addResource("car", 5u);
    foo.addResource("oil", 5u);
    foo.addResource("car", 5u);

    house.transferResourcesTo(foo);
    ASSERT_EQ(house.getAmount("car"), 0u);
    ASSERT_EQ(foo.getAmount("car"), 10u);
    ASSERT_EQ(foo.getAmount("oil"), 5u);

    // Full ! No transfer !
    house.addResource("car", 5u);
    foo.setCapacity("car", 12u);
    house.transferResourcesTo(foo);
    ASSERT_EQ(house.getAmount("car"), 3u);
    ASSERT_EQ(foo.getAmount("car"), 12u);
}
