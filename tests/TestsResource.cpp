#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/Core/Resources.hpp"
#undef protected
#undef private

TEST(TestsResource, Constants)
{
    // Check maximal quantity of resource.
    ASSERT_GE(Resource::MAX_CAPACITY, 65535u);
}

TEST(TestsResource, Constructor)
{
    Resource oil("oil");

    // Check initial values (member variables).
    ASSERT_STREQ(oil.m_type.c_str(), "oil");
    ASSERT_EQ(oil.m_amount, 0u);
    ASSERT_EQ(oil.m_capacity, Resource::MAX_CAPACITY);

    // Check initial values (getter methods).
    ASSERT_EQ(oil.getAmount(), 0u);
    ASSERT_EQ(oil.hasAmount(), false);
    ASSERT_EQ(oil.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_STREQ(oil.type().c_str(), "oil");
}

TEST(TestsResource, AddAmount)
{
    Resource oil("oil");

    // Check initial values.
    ASSERT_EQ(oil.m_amount, 0u);
    ASSERT_EQ(oil.m_capacity, Resource::MAX_CAPACITY);

    // The capacity is infinite. Increment resources of 32.
    // Check 32 amount of resource is present.
    oil.add(32u);
    ASSERT_EQ(oil.m_amount, 32u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // The capacity is infinite. Increment resources of 32.
    // Check 64 amount of resource is present.
    oil.add(32u);
    ASSERT_EQ(oil.m_amount, 64u);
    ASSERT_EQ(oil.getAmount(), 64u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Capacity is now limited to 32. Check the amount has been reduced.
    oil.setCapacity(32u);
    ASSERT_EQ(oil.m_capacity, 32u);
    ASSERT_EQ(oil.getCapacity(), 32u);
    ASSERT_EQ(oil.m_amount, 32u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // The capacity is infinite. Increment resources of 32.
    // Check the amount of resource did not changed.
    oil.add(32u);
    ASSERT_EQ(oil.m_capacity, 32u);
    ASSERT_EQ(oil.getCapacity(), 32u);
    ASSERT_EQ(oil.m_amount, 32u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Capacity is now limited to 32. Check resource is empty.
    oil.setCapacity(0u);
    ASSERT_EQ(oil.m_capacity, 0u);
    ASSERT_EQ(oil.getCapacity(), 0u);
    ASSERT_EQ(oil.m_amount, 0u);
    ASSERT_EQ(oil.getAmount(), 0u);
    ASSERT_EQ(oil.hasAmount(), false);
}

TEST(TestsResource, AddAmountPathologicalCase)
{
    Resource oil("oil");

    // Initial states: 32 resources, infinite capacity.
    oil.add(32u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Add an infity amount of resources, infinite capacity.
    // Check the amount of resource is infinite.
    oil.add(Resource::MAX_CAPACITY);
    ASSERT_EQ(oil.getAmount(), Resource::MAX_CAPACITY);
    ASSERT_EQ(oil.hasAmount(), true);

    // Add an infity amount of resources while capacity is limited.
    // Check the amount did not changed.
    oil.m_amount = 32u;
    oil.setCapacity(32u);
    oil.add(Resource::MAX_CAPACITY);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);
}

TEST(TestsResource, RemoveAmount)
{
    Resource oil("oil");

    // Initial states: 32 resources, infinite capacity.
    oil.add(32u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Decrease amount of resources by 16. Check the amount has been reduced.
    oil.remove(16u);
    ASSERT_EQ(oil.getAmount(), 16u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Decrease amount of resources by 18. Check the amount is constrained to 0
    // and is not negative.
    oil.remove(18u);
    ASSERT_EQ(oil.getAmount(), 0u);
    ASSERT_EQ(oil.hasAmount(), false);
}

TEST(TestsResource, Transfert)
{
    Resource oil("oil");
    Resource gaz("gaz");

    // Initial states: 0 resource, infinite capacity.
    ASSERT_EQ(oil.getAmount(), 0u);
    ASSERT_EQ(oil.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.getAmount(), 0u);
    ASSERT_EQ(gaz.getCapacity(), Resource::MAX_CAPACITY);

    // +32 resources, infinite capacity.
    oil.add(32u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.getCapacity(), Resource::MAX_CAPACITY);

    // Transfert 32 resources, infinite capacity.
    // Check all resources have been transfered.
    oil.transferTo(gaz);
    ASSERT_EQ(oil.getAmount(), 0u);
    ASSERT_EQ(oil.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.getAmount(), 32u);
    ASSERT_EQ(gaz.getCapacity(), Resource::MAX_CAPACITY);

    // Try again! Check nothing happened.
    oil.transferTo(gaz);
    ASSERT_EQ(oil.getAmount(), 0u);
    ASSERT_EQ(oil.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.getAmount(), 32u);
    ASSERT_EQ(gaz.getCapacity(), Resource::MAX_CAPACITY);

    // Full recipient. Check that transfert has not been made.
    oil.add(32u);
    gaz.setCapacity(16u);
    ASSERT_EQ(oil.getAmount(), 32u);
    ASSERT_EQ(oil.getCapacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.getAmount(), 16u);
    ASSERT_EQ(gaz.getCapacity(), 16u);

    // Check transfert has been limited to the capacity of the recipient.
    gaz.remove(1u);
    ASSERT_EQ(gaz.getAmount(), 15u);
    ASSERT_EQ(gaz.getCapacity(), 16u);
    oil.transferTo(gaz);
    ASSERT_EQ(oil.getAmount(), 31u);
    ASSERT_EQ(gaz.getAmount(), 16u);
    ASSERT_EQ(gaz.getCapacity(), 16u);
}
