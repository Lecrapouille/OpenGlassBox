//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Resources.hpp
//! \brief Container of Resource stocks for Buildings or Agents.

#ifndef OPEN_GLASSBOX_RESOURCES_HPP
#define OPEN_GLASSBOX_RESOURCES_HPP

#include "OpenGlassBox/Resource.hpp"
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief All Resources held by one object, looked up by name.
//!
//! Used for a Building, an Agent load, a City treasury, or a BuildingType template.
//! A Resource appears on first use, empty with maximum capacity.
//! A Rule may add to a Resource the script never declared.
//!
//! Example for a house: { People 3/8, Money 1/10, Trash 0/1 }.
//!
//! Lookup is linear over a small vector, not hashed. A Building holds few Resources.
//! Names are compared as strings. Order in the script does not matter.
//!
//! Example:
//! \code
//! ogb::Resources house;
//! house.capacity("People", 8u);
//! house.addResource("People", 8u);
//!
//! ogb::Resources load;
//! load.addResource("People", 1u);
//! house.removeAll(load);            // one leaves for work
//!
//! if (house.amount("People") == 0u)
//!     std::cout << "empty house\n";
//! \endcode
//!
//! The matching script. \c caps sets capacities. \c resources sets starting amounts:
//! \code
//! building Home color 0xFF00FF caps [ People 8 Trash 1 ] resources [ People 8 ]
//! \endcode
//==============================================================================
class Resources
{
public:

    // -------------------------------------------------------------------------
    //! \brief Empty container: no Resources and no capacities set.
    // -------------------------------------------------------------------------
    Resources() = default;

    // -------------------------------------------------------------------------
    //! \brief Find a Resource by name.
    //! \param[in] type Resource type (e.g. "Water").
    //! \return pointer to the Resource, or nullptr if not found.
    // -------------------------------------------------------------------------
    [[nodiscard]] Resource* findResource(ResourceType const& type);
    [[nodiscard]] Resource const* findResource(ResourceType const& type) const;

    // -------------------------------------------------------------------------
    //! \brief Find a Resource by name. Create it if missing.
    //! \param[in] type Resource type (e.g. "Water").
    //! \return reference to the existing or new Resource.
    // -------------------------------------------------------------------------
    [[nodiscard]] Resource& findOrAddResource(ResourceType const& type);

    // -------------------------------------------------------------------------
    //! \brief Find or create a Resource and add to its amount (clamped by capacity).
    //! \param[in] type Resource type.
    //! \param[in] amount amount to add.
    //! \return the Resource found or created.
    // -------------------------------------------------------------------------
    Resource& addResource(ResourceType const& type, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Remove amount from a Resource. Does nothing if the Resource is missing.
    //!
    //! \note Does not remove the Resource type. Only reduces the amount.
    //!
    //! \param[in] type Resource type.
    //! \param[in] amount amount to remove.
    //! \return true if the Resource was found.
    // -------------------------------------------------------------------------
    bool removeResource(ResourceType const& type, uint32_t const amount);

    // -------------------------------------------------------------------------
    //! \brief Add all Resources from another container. Calls addResource() for each.
    //! \param[in] resourcesToAdd Resources and amounts to add.
    // -------------------------------------------------------------------------
    void addAll(Resources const& resourcesToAdd);

    // -------------------------------------------------------------------------
    //! \brief Remove all Resources from another container. Calls removeResource() for each.
    //! \param[in] resourcesToReduce Resources and amounts to remove.
    // -------------------------------------------------------------------------
    void removeAll(Resources const& resourcesToReduce);

    // -------------------------------------------------------------------------
    //! \brief Check if at least one Resource from another container can be added.
    //!
    //! Requires the same Resource type and free capacity in the recipient.
    //!
    //! \param[in] resourcesToTryAdd Resources and amounts to test.
    //! \param[in] reserved slots already reserved by incoming Agents.
    //! Zero ignores reserved amounts.
    //! \return true if at least one Resource can be added.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool canAddAny(Resources const& resourcesToTryAdd,
                             uint32_t reserved = 0u);

    // -------------------------------------------------------------------------
    //! \brief Transfer all Resources to another container.
    //! Each amount is limited by the recipient capacity.
    //! \param[in] resourcesTarget the recipient.
    // -------------------------------------------------------------------------
    void transferTo(Resources& resourcesTarget);

    // -------------------------------------------------------------------------
    //! \return the amount for a Resource type. Returns 0 if missing.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getAmount(ResourceType const& type) const;

    // -------------------------------------------------------------------------
    //! \brief Find or create a Resource and set its capacity.
    //! If present, updates capacity and clamps amount to the new limit.
    //! \param[in] type Resource type.
    //! \param[in] capacity new capacity.
    // -------------------------------------------------------------------------
    void setCapacity(ResourceType const& type, uint32_t const capacity);

    // -------------------------------------------------------------------------
    //! \brief Set capacities for all Resources in another container.
    // -------------------------------------------------------------------------
    void setCapacities(Resources const& resourcesCapacities);

    // -------------------------------------------------------------------------
    //! \brief Replace amounts, keeping capacities.
    //!
    //! A save stores amounts, not capacities. Capacities come from the ruleset.
    //! Replacing the whole container avoids zero capacity after load, which would
    //! block all Agents and Rules that add Resources.
    // -------------------------------------------------------------------------
    void setAmounts(Resources const& amounts);

    // -------------------------------------------------------------------------
    //! \return the capacity for a Resource type. Returns 0 if missing.
    // -------------------------------------------------------------------------
    [[nodiscard]] uint32_t getCapacity(ResourceType const& type) const;

    // -------------------------------------------------------------------------
    //! \return true if all Resources are empty.
    // -------------------------------------------------------------------------
    [[nodiscard]] bool isEmpty() const;

    // -------------------------------------------------------------------------
    //! \return true if a Resource of this type exists.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline bool hasResource(ResourceType const& type) const
    {
        return findResource(type) != nullptr;
    }

    // -------------------------------------------------------------------------
    //! \brief All Resources, in order of first appearance.
    //! Used by the demo inspector and by save/load.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::vector<Resource> const& getAll() const
    {
        return m_bin;
    }

    // -------------------------------------------------------------------------
    //! \brief Write all Resources as "type: amount/capacity". Used in unit test output.
    // -------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os,
                                    Resources const& resources);

private:

    //! \brief The Resource list. Linear search is fast for a small vector.
    std::vector<Resource> m_bin;
};

} // namespace ogb

#endif
