//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Resources.hpp
//! \brief Container of several resource stocks carried by units or agents.

#ifndef OPEN_GLASSBOX_RESOURCES_HPP
#define OPEN_GLASSBOX_RESOURCES_HPP

#include "OpenGlassBox/Resource.hpp"
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief Everything one thing holds: a set of stocks, one per kind, looked up
//! by name.
//!
//! This is the state of a building, the load of an Agent, the treasury of a
//! City and the recipe a UnitType hands to a new building. A stock appears the
//! first time it is mentioned and starts empty with the largest capacity
//! allowed, so a rule may add to a resource nobody declared.
//!
//! Example, a house: { People 3/8, Money 1/10, Trash 0/1 }.
//!
//! Lookups are linear over a small vector rather than hashed: a building holds
//! a handful of resources, and walking a contiguous vector beats hashing a
//! string for that size. Names are compared as strings, so they are what has to
//! match, not the order the script declared them in.
//!
//! Example:
//! \code
//! ogb::Resources house;
//! house.setCapacity("People", 8u);
//! house.addResource("People", 8u);
//!
//! ogb::Resources load;
//! load.addResource("People", 1u);
//! house.removeResources(load);            // one leaves for work
//!
//! if (house.getAmount("People") == 0u)
//!     std::cout << "empty house\n";
//! \endcode
//!
//! The matching script, where \c caps sets the ceilings and \c resources the
//! starting stock:
//! \code
//! unit Home color 0xFF00FF caps [ People 8 Trash 1 ] resources [ People 8 ]
//! \endcode
//==============================================================================
class Resources
{
public:

    // -------------------------------------------------------------------------
    //! \brief An empty set: nothing held and nothing capped.
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
    //! \return the reference of the resource already stored or the newly
    //! created.
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
    //! Conditions are: identical resource type and shall be the same
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
    //! \param[in] resourcesTarget: the recipient.
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
    //! \brief Replace the amounts held, keeping the capacities.
    //!
    //! A save stores what a building holds, not how much it can hold: the
    //! capacities belong to the ruleset. Assigning the whole collection instead
    //! used to leave every loaded building with a capacity of zero, which made
    //! it refuse every Agent and every rule that adds anything.
    // -------------------------------------------------------------------------
    void setAmounts(Resources const& amounts);

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
    //! \brief Every stock held, in the order they first appeared. What the
    //! inspector of the demo walks to draw its bars, and what a save writes.
    // -------------------------------------------------------------------------
    std::vector<Resource> const& container() const
    {
        return m_bin;
    }

    // -------------------------------------------------------------------------
    //! \brief Write every stock as "type: amount/capacity", which is what a
    //! failing unit test prints.
    // -------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os,
                                    Resources const& resources);

private:

    //! \brief The stocks, one per kind. Small enough that a linear search over
    //! a contiguous vector is the fastest lookup.
    std::vector<Resource> m_bin;
};

} // namespace ogb

#endif
