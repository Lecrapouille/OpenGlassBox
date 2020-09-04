#ifndef RESOURCEBIN_HPP
#  define RESOURCEBIN_HPP

#  include "Resource.hpp"

//==============================================================================
//! \brief Resources come in a container. This class manages a collection of
//! Resources. A Resources starts with no resource.
//==============================================================================
class Resources
{
public:

    // -------------------------------------------------------------------------
    //! \brief Find for an existing resource in the collection. If not found
    //! create and store a new resource. Set increase the amount of resource.
    //!
    //! \param type: the type of resource.
    //! \param amount: increase the current amount of resource by the given
    //! quantity.
    //! \return the found resource or newly created.
    // -------------------------------------------------------------------------
    Resource& addResource(Resource::Type const& type, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Add a collection of resources. Apply addResource() for each type
    //! of resource.
    // -------------------------------------------------------------------------
    void addResources(Resources const& resourcesToAdd);

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
    void removeResource(Resource::Type const& resourceType, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getAmount(Resource::Type const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setCapacity(Resource::Type const& resourceType, uint32_t const capacity);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    void setCapacities(Resources const& resourcesCapacities);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    uint32_t getCapacity(Resource::Type const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    bool isEmpty() const;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    inline bool hasResource(Resource::Type const& resourceType)
    {
        return findResource(resourceType) != nullptr;
    }

private:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resource* findResource(Resource::Type const& resourceType);

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resource& findOrAddResource(Resource::Type const& resourceType);

public:

    std::vector<Resource> m_bin;
};

#endif
