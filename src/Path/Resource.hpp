#ifndef RESOURCES_HPP
#  define RESOURCES_HPP

#  include <string>
#  include <limits>
#  include <vector>

// -----------------------------------------------------------------------------
//! \brief The basic currency of the game:
//!  - Citizen, cars, goods ...
//!  - Oil, coal, wood, water, electricity ...
//!  - Money, food, labour, pollution, trash ...
//!  - Happiness, sickness, taxes
// -----------------------------------------------------------------------------
class Resource
{
public:

    using Type = std::string;

public:

    Resource(Resource::Type const& type);
    void add(uint32_t const toAdd);
    void remove(uint32_t const toRemove);
    void transferTo(Resource& target);
    void setCapacity(uint32_t const capacity);

    inline std::string const& type() const
    {
        return m_type;
    }

    inline uint32_t getAmount() const
    {
        return m_amount;
    }

    inline bool hasAmount() const
    {
        return m_amount > 0u;
    }

    inline uint32_t getCapacity() const
    {
        return m_capacity;
    }

    static uint32_t maxCapacity()
    {
        static uint32_t const max_capacity =
                std::numeric_limits<uint32_t>::max();
        return max_capacity;
    }

protected:

    Type     m_type;
    uint32_t m_capacity = Resource::maxCapacity();
    uint32_t m_amount = 0u;
};

// -----------------------------------------------------------------------------
//! \brief Resources come in bin.
//! Bin of resource R, has capacity C. Capacity is fixed
//! Bin value is an integer: 0..C
// -----------------------------------------------------------------------------
class ResourceBin
{
public:

    void addResource(Resource::Type const& type, uint32_t const amount);
    void addResources(ResourceBin const& resourcesToAdd);
    bool canAddSomeResources(ResourceBin const& resourcesToTryAdd);
    void transferResourcesTo(ResourceBin& resourcesTarget);
    void removeResource(Resource::Type const& resourceType, uint32_t const amount);
    uint32_t getAmount(Resource::Type const& resourceType);
    void setCapacity(Resource::Type const& resourceType, uint32_t const capacity);
    void setCapacities(ResourceBin const& resourcesCapacities);
    uint32_t getCapacity(Resource::Type const& resourceType);
    bool isEmpty() const;

    inline bool hasResource(Resource::Type const& resourceType)
    {
        return findResource(resourceType) != nullptr;
    }

private:

    Resource* findResource(Resource::Type const& resourceType);
    Resource& findOrAddResource(Resource::Type const& resourceType);

public:

    std::vector<Resource> m_bin;
};

#endif
