#include "Resource.hpp"

Resource::Resource(Resource::Type const& type)
    : m_type(type)
{}

void Resource::add(uint32_t const toAdd)
{
    m_amount += toAdd; // FIXME integer overflow
    if (m_amount > m_capacity)
        m_amount = m_capacity;
}

void Resource::remove(uint32_t const toRemove)
{
    if (m_amount > toRemove)
        m_amount -= toRemove;
    else
        m_amount = 0u;
}

void Resource::transferTo(Resource& target)
{
    uint32_t toTransfer =
            std::min(m_amount, target.m_capacity - target.m_amount);

    remove(toTransfer);
    target.add(toTransfer);
}

void Resource::setCapacity(uint32_t const capacity)
{
    m_capacity = capacity;
    if (m_amount > capacity)
        m_amount = capacity;
}

// -----------------------------------------------------------------------------

void ResourceBin::addResource(Resource::Type const& type, uint32_t const amount)
{
    Resource& res = findOrAddResource(type);
    res.add(amount);
}

void ResourceBin::addResources(ResourceBin const& resourcesToAdd)
{
    for (auto const& it: resourcesToAdd.m_bin)
        addResource(it.type(), it.getAmount());
}

bool ResourceBin::canAddSomeResources(ResourceBin const& resourcesToTryAdd)
{
    for (auto& it: resourcesToTryAdd.m_bin)
    {
        if (it.hasAmount())
        {
            Resource* res = findResource(it.type());
            if ((res != nullptr) && (res->getAmount() < res->getCapacity()))
                return true;
        }
    }

    return false;
}

void ResourceBin::transferResourcesTo(ResourceBin& resourcesTarget)
{
    for (auto& it: m_bin)
        it.transferTo(resourcesTarget.findOrAddResource(it.type()));
}

void ResourceBin::removeResource(Resource::Type const& resourceType, uint32_t const amount)
{
    Resource* res = findResource(resourceType);

    if (res != nullptr)
        res->remove(amount);
}

uint32_t ResourceBin::getAmount(Resource::Type const& resourceType)
{
    Resource* res = findResource(resourceType);

    return (res != nullptr) ? res->getAmount() : 0u;
}

void ResourceBin::setCapacity(Resource::Type const& resourceType, uint32_t const capacity)
{
    Resource& res = findOrAddResource(resourceType);

    res.setCapacity(capacity);
}

void ResourceBin::setCapacities(ResourceBin const& resourcesCapacities)
{
    for (auto& it: resourcesCapacities.m_bin)
        setCapacity(it.type(), it.getAmount());
}

uint32_t ResourceBin::getCapacity(Resource::Type const& resourceType)
{
    Resource* b = findResource(resourceType);

    return (b != nullptr) ? b->getCapacity() : Resource::maxCapacity();
}

bool ResourceBin::isEmpty() const
{
    for (auto const& it: m_bin)
    {
        if (it.hasAmount())
            return false;
    }

    return true;
}


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
