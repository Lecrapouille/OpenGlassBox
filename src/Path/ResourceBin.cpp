#include "ResourceBin.hpp"

// -----------------------------------------------------------------------------
Resource& ResourceBin::addResource(Resource::Type const& type, uint32_t const amount)
{
    Resource& res = findOrAddResource(type);
    res.add(amount);

    return res;
}

// -----------------------------------------------------------------------------
void ResourceBin::addResources(ResourceBin const& resourcesToAdd)
{
    for (auto const& it: resourcesToAdd.m_bin)
        addResource(it.type(), it.amount());
}

// -----------------------------------------------------------------------------
bool ResourceBin::canAddSomeResources(ResourceBin const& resourcesToTryAdd)
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
void ResourceBin::transferResourcesTo(ResourceBin& resourcesTarget)
{
    for (auto& it: m_bin)
        it.transferTo(resourcesTarget.findOrAddResource(it.type()));
}

// -----------------------------------------------------------------------------
void ResourceBin::removeResource(Resource::Type const& resourceType, uint32_t const amount)
{
    Resource* res = findResource(resourceType);

    if (res != nullptr)
        res->remove(amount);
}

// -----------------------------------------------------------------------------
uint32_t ResourceBin::amount(Resource::Type const& resourceType)
{
    Resource* res = findResource(resourceType);

    return (res != nullptr) ? res->amount() : 0u;
}

// -----------------------------------------------------------------------------
void ResourceBin::setCapacity(Resource::Type const& resourceType, uint32_t const capacity)
{
    Resource& res = findOrAddResource(resourceType);

    res.setCapacity(capacity);
}

// -----------------------------------------------------------------------------
void ResourceBin::setCapacities(ResourceBin const& resourcesCapacities)
{
    for (auto& it: resourcesCapacities.m_bin)
        setCapacity(it.type(), it.amount());
}

// -----------------------------------------------------------------------------
uint32_t ResourceBin::capacity(Resource::Type const& resourceType)
{
    Resource* b = findResource(resourceType);

    return (b != nullptr) ? b->capacity() : Resource::maxCapacity();
}

// -----------------------------------------------------------------------------
bool ResourceBin::isEmpty() const
{
    for (auto const& it: m_bin)
    {
        if (it.hasAmount())
            return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
Resource* ResourceBin::findResource(Resource::Type const& resourceType)
{
    for (auto& it: m_bin)
    {
        if (it.type() == resourceType)
            return &it;
    }

    return nullptr;
}

Resource& ResourceBin::findOrAddResource(Resource::Type const& resourceType)
{
    Resource* b = findResource(resourceType);

    if (b != nullptr)
        return *b;

    m_bin.push_back(Resource(resourceType));
    return m_bin.back();
}
