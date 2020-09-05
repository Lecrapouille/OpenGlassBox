#include "Resources.hpp"

// -----------------------------------------------------------------------------
Resource& Resources::addResource(Resource::Type const& type, uint32_t const amount)
{
    Resource& res = findOrAddResource(type);
    res.add(amount);

    return res;
}

// -----------------------------------------------------------------------------
void Resources::addResources(Resources const& resourcesToAdd)
{
    for (auto const& it: resourcesToAdd.m_bin)
        addResource(it.type(), it.amount());
}

// -----------------------------------------------------------------------------
bool Resources::canAddSomeResources(Resources const& resourcesToTryAdd)
{
    for (auto& it: resourcesToTryAdd.m_bin)
    {
        if (it.hasAmount())
        {
            Resource* res = findResource(it.type());
            if ((res != nullptr) && (res->amount() < res->capacity()))
                return true;
        }
    }

    return false;
}

// -----------------------------------------------------------------------------
void Resources::transferResourcesTo(Resources& resourcesTarget)
{
    for (auto& it: m_bin)
        it.transferTo(resourcesTarget.findOrAddResource(it.type()));
}

// -----------------------------------------------------------------------------
bool Resources::removeResource(Resource::Type const& resourceType, uint32_t const amount)
{
    Resource* res = findResource(resourceType);

    if (res != nullptr)
        res->remove(amount);

    return res != nullptr;
}

// -----------------------------------------------------------------------------
void Resources::removeResources(Resources const& resourcesToReduce)
{
    for (auto const& it: resourcesToReduce.m_bin)
        removeResource(it.type(), it.amount());
}

// -----------------------------------------------------------------------------
uint32_t Resources::getAmount(Resource::Type const& resourceType)
{
    Resource* res = findResource(resourceType);

    return (res != nullptr) ? res->amount() : 0u;
}

// -----------------------------------------------------------------------------
void Resources::setCapacity(Resource::Type const& resourceType, uint32_t const capacity)
{
    Resource& res = findOrAddResource(resourceType);
    res.setCapacity(capacity);
}

// -----------------------------------------------------------------------------
void Resources::setCapacities(Resources const& resourcesCapacities)
{
    for (auto& it: resourcesCapacities.m_bin)
        setCapacity(it.type(), it.capacity());
}

// -----------------------------------------------------------------------------
uint32_t Resources::getCapacity(Resource::Type const& resourceType)
{
    Resource* b = findResource(resourceType);
    return (b != nullptr) ? b->capacity() : 0u; // Resource::MAX_CAPACITY;
}

// -----------------------------------------------------------------------------
bool Resources::isEmpty() const
{
    for (auto const& it: m_bin)
    {
        if (it.hasAmount())
            return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
Resource* Resources::findResource(Resource::Type const& resourceType)
{
    for (auto& it: m_bin)
    {
        if (it.type() == resourceType)
            return &it;
    }

    return nullptr;
}

Resource& Resources::findOrAddResource(Resource::Type const& resourceType)
{
    Resource* b = findResource(resourceType);

    if (b != nullptr)
        return *b;

    m_bin.push_back(Resource(resourceType));
    return m_bin.back();
}
