//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Resource.hpp
//! \brief One stock of one thing: what it is, how much of it, and how much
//! fits.

#ifndef OPEN_GLASSBOX_RESOURCE_HPP
#define OPEN_GLASSBOX_RESOURCE_HPP

#include "OpenGlassBox/Name.hpp"

namespace ogb
{

//==============================================================================
//! \brief What a Resource is made of, as a name: "Water", "People", "Goods",
//! "Money", "Pollution".
//!
//! A name rather than an enumeration, because the list is whatever the script
//! declares. Two resources are the same resource when their names match, which
//! is also how a rule refers to one and how an Agent finds a building that has
//! room for what it carries.
//!
//! Interned, so that those matches cost an integer comparison: see Name. A
//! literal still works wherever one is expected.
//!
//! Example:
//! \code
//! resources
//!     resource Water
//!     resource People
//! end
//! \endcode
//==============================================================================
using ResourceType = Name;

//==============================================================================
//! \brief The currency of the simulation: an amount of one thing, and the
//! largest amount of it that fits.
//!
//! Everything a city does is moving these around. A house holds People, a
//! factory turns them into Goods, a truck carries the Goods to a shop, a cell
//! of the Pollution layer holds what the factory gave off. What a Resource is for
//! is entirely up to the script: the engine only ever adds, removes, transfers
//! and compares.
//!
//! Two invariants hold at all times, and they are what makes a rule safe to
//! write: the amount never exceeds the capacity, and it never goes below zero.
//! Both are enforced by clamping rather than by refusing, so add() and remove()
//! cannot fail. A rule that must not lose anything tests the room first, which
//! is what RuleCommandAdd::validate does.
//!
//! Example:
//! \code
//! ogb::Resource goods("Goods");
//! goods.capacity(10u);
//! goods.add(4u);          // 4 of 10
//!
//! ogb::Resource lorry("Goods");
//! lorry.capacity(3u);
//! goods.transferTo(lorry); // the lorry takes 3, one of the four stays
//! \endcode
//==============================================================================
class Resource
{
public:

    // -------------------------------------------------------------------------
    //! \brief An empty stock with the largest capacity allowed, which is what a
    //! resource nothing has capped means: a Layer cell or a building that was
    //! never given a ceiling holds as much as it is given.
    //! \param[in] type name of the thing held.
    // -------------------------------------------------------------------------
    explicit Resource(ResourceType const& type);

    // -------------------------------------------------------------------------
    //! \brief Add to the stock, up to the capacity. What does not fit is lost.
    //! \param[in] toAdd how much to add.
    // -------------------------------------------------------------------------
    void add(uint32_t const toAdd);

    // -------------------------------------------------------------------------
    //! \brief Take from the stock, down to zero. Taking more than is there
    //! empties it rather than wrapping around.
    //! \param[in] toRemove how much to take.
    // -------------------------------------------------------------------------
    void remove(uint32_t const toRemove);

    // -------------------------------------------------------------------------
    //! \brief Move as much of this stock as fits into another one.
    //!
    //! What the recipient cannot take stays here, which is how a delivery to an
    //! almost full building leaves the Agent with something to carry on with.
    //!
    //! \param[in,out] target the recipient. Nothing is moved when it is a stock
    //! of something else.
    // -------------------------------------------------------------------------
    void transferTo(Resource& target);

    // -------------------------------------------------------------------------
    //! \brief Set the ceiling. An amount already above the new ceiling is cut
    //! down to it.
    //! \param[in] capacity the new ceiling.
    // -------------------------------------------------------------------------
    void setCapacity(uint32_t const capacity);

    // -------------------------------------------------------------------------
    //! \brief \return the name of the thing held, such as "Water".
    // -------------------------------------------------------------------------
    [[nodiscard]] inline ResourceType const& getTypeName() const
    {
        return m_type;
    }

    // -------------------------------------------------------------------------
    //! \brief \return the largest amount that fits.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getCapacity() const
    {
        return m_capacity;
    }

    // -------------------------------------------------------------------------
    //! \brief \return how much is held now.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getAmount() const
    {
        return m_amount;
    }

    // -------------------------------------------------------------------------
    //! \brief \return true when the stock is not empty.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline bool hasAmount() const
    {
        return m_amount > 0u;
    }

    // -------------------------------------------------------------------------
    //! \brief Write the stock as "type: amount/capacity", which is what a
    //! failing unit test prints.
    // -------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Resource const& resource);

public:

    //! \brief The largest capacity a stock may be given, and the one it starts
    //! with: the largest a uint32_t holds. add() watches for the wrap around
    //! rather than leaving room below it.
    static const uint32_t MAX_CAPACITY;

protected:

    //! \brief Name of the thing held.
    ResourceType m_type;
    //! \brief Largest amount that fits.
    uint32_t m_capacity = Resource::MAX_CAPACITY;
    //! \brief How much is held now, never above m_capacity.
    uint32_t m_amount = 0u;
};

} // namespace ogb

#endif
