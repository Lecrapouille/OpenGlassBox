//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Resources.hpp"

// -----------------------------------------------------------------------------
Resource& Resources::addResource(ResourceType const& type, uint32_t const amount)
{
    Resource& res = findOrAddResource(type);
    res.add(amount);

    return res;
}

// -----------------------------------------------------------------------------
void Resources::addResources(Resources const& resourcesToAdd)
{
    if (this == &resourcesToAdd)
        return ;

    for (auto const& it: resourcesToAdd.m_bin)
        addResource(it.type(), it.getAmount());
}

// -----------------------------------------------------------------------------
bool Resources::canAddSomeResources(Resources const& resourcesToTryAdd)
{
    if (this == &resourcesToTryAdd)
        return false;

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

// -----------------------------------------------------------------------------
void Resources::transferResourcesTo(Resources& resourcesTarget)
{
    if (this == &resourcesTarget)
        return ;

    for (auto& it: m_bin)
        it.transferTo(resourcesTarget.findOrAddResource(it.type()));
}

// -----------------------------------------------------------------------------
bool Resources::removeResource(ResourceType const& type, uint32_t const amount)
{
    Resource* res = findResource(type);

    if (res != nullptr)
        res->remove(amount);

    return res != nullptr;
}

// -----------------------------------------------------------------------------
void Resources::removeResources(Resources const& resourcesToReduce)
{
    if (this == &resourcesToReduce)
        return ;

    for (auto const& it: resourcesToReduce.m_bin)
        removeResource(it.type(), it.getAmount());
}

// -----------------------------------------------------------------------------
uint32_t Resources::getAmount(ResourceType const& type) const
{
    const Resource* res = findResource(type);
    return (res != nullptr) ? res->getAmount() : 0u;
}

// -----------------------------------------------------------------------------
void Resources::setCapacity(ResourceType const& type, uint32_t const capacity)
{
    Resource& res = findOrAddResource(type);
    res.setCapacity(capacity);
}

// -----------------------------------------------------------------------------
void Resources::setCapacities(Resources const& resourcesCapacities)
{
    for (auto& it: resourcesCapacities.m_bin)
        setCapacity(it.type(), it.getCapacity());
}

// -----------------------------------------------------------------------------
uint32_t Resources::getCapacity(ResourceType const& type) const
{
    const Resource* b = findResource(type);
    return (b != nullptr) ? b->getCapacity() : 0u; // Resource::MAX_CAPACITY;
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
const Resource* Resources::findResource(ResourceType const& type) const
{
    for (auto& it: m_bin)
    {
        if (it.type() == type)
            return &it;
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
Resource* Resources::findResource(ResourceType const& type)
{
    for (auto& it: m_bin)
    {
        if (it.type() == type)
            return &it;
    }

    return nullptr;
}

// -----------------------------------------------------------------------------
Resource& Resources::findOrAddResource(ResourceType const& type)
{
    Resource* b = findResource(type);

    if (b != nullptr)
        return *b;

    m_bin.push_back(Resource(type));
    return m_bin.back();
}

// -------------------------------------------------------------------------
//! \brief Pretty print the content of a resource (debug purpose).
// -------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, Resources const& resources)
{
    os << resources.m_bin.size() << " Resources:\n";
    for (auto const& it: resources.m_bin)
        os << "  " << it;
    return os;
}
