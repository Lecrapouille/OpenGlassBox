#ifndef RESOURCEBIN_HPP
#  define RESOURCEBIN_HPP

#  include "Core/Resource.hpp"

//==============================================================================
//! \brief Resources come in a container. This class manages a collection of
//! Resources. A Resources starts with no resource.
//==============================================================================
class Resources
{
public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resources(Resources const&) = default;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resources() = default;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resource* findResource(ResourceType const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resource& findOrAddResource(ResourceType const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief Find for an existing resource in the collection. If not found
    //! create and store a new resource with the current amount. If the resource
    //! already existes then increase its amount of resource (limited by its
    //! capacity).
    //!
    //! \param type: the type of resource.
    //! \param amount: increase the current amount of resource by the given
    //! quantity.
    //! \return the found resource or newly created.
    // -------------------------------------------------------------------------
    Resource& addResource(ResourceType const& type, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Reduce a given quantity of resource.
    //! \note this method does not delete the resource.
    //! \return boolean indicating if the desired resource has been found.
    // -------------------------------------------------------------------------
    bool removeResource(ResourceType const& resourceType, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Add a collection of resources. Apply addResource() for each type
    //! of resource.
    // -------------------------------------------------------------------------
    void addResources(Resources const& resourcesToAdd);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void removeResources(Resources const& resourcesToReduce);

    // -------------------------------------------------------------------------
    //! \brief Check if we can add at least one resource
    //!
    //! Conditions are: identitical resource type and shall be the same
    //! and recipient shall not be full.
    //! \return true if it possible to add resources, else return false.
    // -------------------------------------------------------------------------
    bool canAddSomeResources(Resources const& resourcesToTryAdd);

    // -------------------------------------------------------------------------
    //! \brief Transfer all resources to the recipient. For each resource the
    //! amount of resource is limited by the capacity of the recipient.
    // -------------------------------------------------------------------------
    void transferResourcesTo(Resources& resourcesTarget);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getAmount(ResourceType const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief Find for an existing resource in the collection. If not found
    //! create and store a new resource with the current capacity. If the
    //! resource already existes then its capacity is changed and the current
    //! amount of resource is limited to the new capacity.
    //!
    //! \param type: the type of resource.
    //! \param capacity: new capacity.
    //! \return the found resource or newly created.
    // -------------------------------------------------------------------------
    void setCapacity(ResourceType const& resourceType, uint32_t const capacity);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setCapacities(Resources const& resourcesCapacities);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getCapacity(ResourceType const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    bool isEmpty() const;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline bool hasResource(ResourceType const& resourceType)
    {
        return findResource(resourceType) != nullptr;
    }

private:

    std::vector<Resource> m_bin;
};

#endif
