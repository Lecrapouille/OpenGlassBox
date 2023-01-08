//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_RESOURCES_HPP
#  define OPEN_GLASSBOX_RESOURCES_HPP

#  include "OpenGlassBox/Resource.hpp"

//==============================================================================
//! \brief Resources come in a container. This class manages a collection of
//! heterogeneous resources. A Resource are initialy empty but with inifinite
//! capacity.
//! Example: House = { Citizen 0/2, Money 1/10, Electricity 3/3, Trash 0/1 }.
//==============================================================================
class Resources
{
public:

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    //Resources(Resources const&) = default;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    Resources() = default;

    // -------------------------------------------------------------------------
    //! \brief Search for a resource given its name.
    //! \param[in] type: the type of resource (ie "Water").
    //! \return the address of the resource if present. Return nullptr if not
    //! found.
    // -------------------------------------------------------------------------
    Resource* findResource(ResourceType const& type);
    const Resource* findResource(ResourceType const& type) const;

    // -------------------------------------------------------------------------
    //! \brief Search for a resource given its name. If the resource is not
    //! present create one and store it before returning its reference.
    //! \param[in] type: the type of resource (ie "Water").
    //! \return the reference of the resource already stored or the newly created.
    // -------------------------------------------------------------------------
    Resource& findOrAddResource(ResourceType const& type);

    // -------------------------------------------------------------------------
    //! \brief Find for an existing resource in the collection. If not found
    //! create and store a new resource with the current amount. If the resource
    //! already existes then increase its amount of resource (limited by its
    //! capacity).
    //!
    //! \param[in] type: the type of resource.
    //! \param[in] amount: increase the current amount of resource by the given
    //! quantity.
    //! \return the found resource or newly created.
    // -------------------------------------------------------------------------
    Resource& addResource(ResourceType const& type, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Reduce a given quantity of resource. If the resource does not
    //! exist this function does nothing.
    //!
    //! \note this method does not delete a type of resource but acts on the
    //! amount of resource.
    //!
    //! \param[in] type: the type of resource.
    //! \param[in] amount: increase the current amount of resource by the given
    //! quantity.
    //! \return boolean indicating if the desired resource has been found.
    // -------------------------------------------------------------------------
    bool removeResource(ResourceType const& type, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Add a collection of resources. Apply addResource() for each type
    //! of resource.
    //!
    //! \param[in] resourcesToAdd: what resources and what amount to increase.
    // -------------------------------------------------------------------------
    void addResources(Resources const& resourcesToAdd);

    // -------------------------------------------------------------------------
    //! \brief Apply removeResource() for each resources.
    //!
    //! \param[in] resourcesToReduce: what resources and what amount to reduce.
    // -------------------------------------------------------------------------
    void removeResources(Resources const& resourcesToReduce);

    // -------------------------------------------------------------------------
    //! \brief Check if we can add at least one resource.
    //!
    //! Conditions are: identitical resource type and shall be the same
    //! and recipient shall not be full.
    //!
    //! \param[in] resourcesToTryAdd: what resources and what amount to add.
    //! \return true if it possible to add at least one resource, else false.
    // -------------------------------------------------------------------------
    bool canAddSomeResources(Resources const& resourcesToTryAdd);

    // -------------------------------------------------------------------------
    //! \brief Transfer all resources to the recipient. For each resource the
    //! amount of resource is limited by the capacity of the recipient.
    //!
    //! \param[in] resourcesTarget: the receipient.
    // -------------------------------------------------------------------------
    void transferResourcesTo(Resources& resourcesTarget);

    // -------------------------------------------------------------------------
    //! \brief Return the amount of resource of the given type. If the resource
    //! does not exist return 0.
    // -------------------------------------------------------------------------
    uint32_t getAmount(ResourceType const& type) const;

    // -------------------------------------------------------------------------
    //! \brief Find for an existing resource in the collection and change its
    //! capacity. If the resource has not been found then create and store a new
    //! resource with the current capacity. If the resource is already present
    //! then its capacity is changed and the current amount of resource is
    //! limited to the newly capacity.
    //!
    //! \param[in] type: the type of resource.
    //! \param[in] capacity: the new capacity.
    //! \return the found resource or newly created.
    // -------------------------------------------------------------------------
    void setCapacity(ResourceType const& type, uint32_t const capacity);

    // -------------------------------------------------------------------------
    //! \brief Apply setCapacity() to a collection of resources.
    // -------------------------------------------------------------------------
    void setCapacities(Resources const& resourcesCapacities);

    // -------------------------------------------------------------------------
    //! \brief Return the maximal amount of resource of the given type. If the
    //! resource does not exist return 0.
    // -------------------------------------------------------------------------
    uint32_t getCapacity(ResourceType const& type) const;

    // -------------------------------------------------------------------------
    //! \brief Return true if all resources are empty.
    // -------------------------------------------------------------------------
    bool isEmpty() const;

    // -------------------------------------------------------------------------
    //! \brief Return true if the resource of the given type is present in the
    //! collection.
    // -------------------------------------------------------------------------
    inline bool hasResource(ResourceType const& type)
    {
        return findResource(type) != nullptr;
    }

    // -------------------------------------------------------------------------
    //! \brief Return in read only access resources (for debug purpose).
    // -------------------------------------------------------------------------
    std::vector<Resource> const& container() const { return m_bin; }

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Resources const& resources);

private:

    std::vector<Resource> m_bin;
};

#endif
