#include "main.hpp"
#include <iostream>

#define protected public
#define private public
#  include "src/Resources.hpp"
#undef protected
#undef private

TEST(TestsResource, Constants)
{
    ASSERT_GE(Resource::MAX_CAPACITY, 65535);
}

TEST(TestsResource, Constructor)
{
    Resource oil("oil");

    // Initial values
    ASSERT_STREQ(oil.m_type.c_str(), "oil");
    ASSERT_EQ(oil.m_amount, 0u);
    ASSERT_EQ(oil.m_capacity, Resource::MAX_CAPACITY);

    // Initial values
    ASSERT_EQ(oil.amount(), 0u);
    ASSERT_EQ(oil.hasAmount(), false);
    ASSERT_EQ(oil.capacity(), Resource::MAX_CAPACITY);
    ASSERT_STREQ(oil.type().c_str(), "oil");
}

TEST(TestsResource, AddAmount)
{
    Resource oil("oil");

    // Initial values
    ASSERT_EQ(oil.m_amount, 0u);
    ASSERT_EQ(oil.m_capacity, Resource::MAX_CAPACITY);

    // +32 resources, infinite capacity
    oil.add(32u);
    ASSERT_EQ(oil.m_amount, 32u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // +32 resources, infinite capacity
    oil.add(32u);
    ASSERT_EQ(oil.m_amount, 64u);
    ASSERT_EQ(oil.amount(), 64u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Capacity limited to 32
    oil.setCapacity(32u);
    ASSERT_EQ(oil.m_capacity, 32u);
    ASSERT_EQ(oil.capacity(), 32u);
    ASSERT_EQ(oil.m_amount, 32u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // +32 resources, capacity limited to 32
    oil.add(32u);
    ASSERT_EQ(oil.m_capacity, 32u);
    ASSERT_EQ(oil.capacity(), 32u);
    ASSERT_EQ(oil.m_amount, 32u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // Capacity limited to 0
    oil.setCapacity(0u);
    ASSERT_EQ(oil.m_capacity, 0u);
    ASSERT_EQ(oil.capacity(), 0u);
    ASSERT_EQ(oil.m_amount, 0u);
    ASSERT_EQ(oil.amount(), 0u);
    ASSERT_EQ(oil.hasAmount(), false);
}

TEST(TestsResource, AddAmountPathologicalCase)
{
    Resource oil("oil");

    // 32 resources, infinite capacity
    oil.add(32u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // +inf resources, infinite capacity
    oil.add(Resource::MAX_CAPACITY);
    ASSERT_EQ(oil.amount(), Resource::MAX_CAPACITY);
    ASSERT_EQ(oil.hasAmount(), true);

    // +inf resources, capacity limited to 32
    oil.m_amount = 32u;
    oil.setCapacity(32u);
    oil.add(Resource::MAX_CAPACITY);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);
}

TEST(TestsResource, RemoveAmount)
{
    Resource oil("oil");

    // 32 resources, infinite capacity
    oil.add(32u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.hasAmount(), true);

    // -16 resources
    oil.remove(16u);
    ASSERT_EQ(oil.amount(), 16u);
    ASSERT_EQ(oil.hasAmount(), true);

    // -18 resources, saturated to 0
    oil.remove(18u);
    ASSERT_EQ(oil.amount(), 0u);
    ASSERT_EQ(oil.hasAmount(), false);
}

TEST(TestsResource, Transfert)
{
    Resource oil("oil");
    Resource gaz("gaz");

    // 0 resource, infinite capacity
    ASSERT_EQ(oil.amount(), 0u);
    ASSERT_EQ(oil.capacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.amount(), 0u);
    ASSERT_EQ(gaz.capacity(), Resource::MAX_CAPACITY);

    // 32 resources, infinite capacity
    oil.add(32u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.capacity(), Resource::MAX_CAPACITY);

    // Transfert 32 resources, infinite capacity
    oil.transferTo(gaz);
    ASSERT_EQ(oil.amount(), 0u);
    ASSERT_EQ(oil.capacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.amount(), 32u);
    ASSERT_EQ(gaz.capacity(), Resource::MAX_CAPACITY);

    // Transfert 0 resources, infinite capacity
    oil.transferTo(gaz);
    ASSERT_EQ(oil.amount(), 0u);
    ASSERT_EQ(oil.capacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.amount(), 32u);
    ASSERT_EQ(gaz.capacity(), Resource::MAX_CAPACITY);

    // No transfert: recipient is full
    oil.add(32u);
    gaz.setCapacity(16u);
    ASSERT_EQ(oil.amount(), 32u);
    ASSERT_EQ(oil.capacity(), Resource::MAX_CAPACITY);
    ASSERT_EQ(gaz.amount(), 16u);
    ASSERT_EQ(gaz.capacity(), 16u);

    // Transfert limited to capacity of the recipient
    gaz.remove(1u);
    ASSERT_EQ(gaz.amount(), 15u);
    ASSERT_EQ(gaz.capacity(), 16u);
    oil.transferTo(gaz);
    ASSERT_EQ(oil.amount(), 31u);
    ASSERT_EQ(gaz.amount(), 16u);
    ASSERT_EQ(gaz.capacity(), 16u);
}
