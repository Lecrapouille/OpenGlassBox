#include "Core/Resource.hpp"

// -----------------------------------------------------------------------------
const uint32_t Resource::MAX_CAPACITY = std::numeric_limits<uint32_t>::max();

// -----------------------------------------------------------------------------
Resource::Resource(ResourceType const& type)
    : m_type(type)
{}

// -----------------------------------------------------------------------------
void Resource::add(uint32_t const toAdd)
{
    // Avoid integer overflow
    if (m_amount >= Resource::MAX_CAPACITY - toAdd)
        m_amount = Resource::MAX_CAPACITY;
    else
        m_amount += toAdd;

    if (m_amount > m_capacity)
        m_amount = m_capacity;
}

// -----------------------------------------------------------------------------
void Resource::remove(uint32_t const toRemove)
{
    if (m_amount > toRemove)
        m_amount -= toRemove;
    else
        m_amount = 0u;
}

// -----------------------------------------------------------------------------
void Resource::transferTo(Resource& target)
{
    uint32_t toTransfer = std::min(m_amount, target.m_capacity - target.m_amount);

    remove(toTransfer);
    target.add(toTransfer);
}

// -----------------------------------------------------------------------------
void Resource::setCapacity(uint32_t const capacity)
{
    m_capacity = capacity;
    if (m_amount > capacity)
        m_amount = capacity;
}
