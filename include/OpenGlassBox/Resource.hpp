//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Resource.hpp
//! \brief One stock of one Resource: type, amount, and capacity.

#ifndef OPEN_GLASSBOX_RESOURCE_HPP
#define OPEN_GLASSBOX_RESOURCE_HPP

#include "OpenGlassBox/Name.hpp"

namespace ogb
{

//==============================================================================
//! \brief What a Resource holds, as a name: "Water", "People", "Goods", etc.
//!
//! Uses a name, not an enum. The script declares the list.
//! Two resources match when their names match. Rules and Agents use names too.
//!
//! Interned as a Name for fast comparison. Literals still work. See Name.
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
//! \brief An amount of one Resource and its capacity.
//!
//! A city moves Resources around. A house holds People, a factory makes Goods,
//! an Agent carries Goods to a shop, a Pollution Layer cell holds emissions.
//! The script defines what each Resource means. The engine adds, removes,
//! transfers, and compares.
//!
//! Two rules always hold: amount never exceeds capacity, and never goes below zero.
//! add() and remove() clamp instead of failing. To avoid loss, check room first.
//! RuleCommandAdd::validate does this.
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
    //! \brief Empty stock with maximum capacity.
    //! Used when nothing caps the amount (Layer cell, Building with no ceiling).
    //! \param[in] type name of the Resource.
    // -------------------------------------------------------------------------
    explicit Resource(ResourceType const& type);

    // -------------------------------------------------------------------------
    //! \brief Add to the stock, up to capacity. Excess is lost.
    //! \param[in] toAdd amount to add.
    // -------------------------------------------------------------------------
    void add(uint32_t const toAdd);

    // -------------------------------------------------------------------------
    //! \brief Remove from the stock, down to zero.
    //! Removing more than available empties the stock (no wrap-around).
    //! \param[in] toRemove amount to remove.
    // -------------------------------------------------------------------------
    void remove(uint32_t const toRemove);

    // -------------------------------------------------------------------------
    //! \brief Move as much as fits into another Resource.
    //!
    //! What does not fit stays here. Used when a full Building leaves an Agent
    //! with a remainder.
    //!
    //! \param[in,out] target the recipient. No move if the type differs.
    // -------------------------------------------------------------------------
    void transferTo(Resource& target);

    // -------------------------------------------------------------------------
    //! \brief Set capacity. Amount above the new capacity is reduced.
    //! \param[in] capacity new capacity.
    // -------------------------------------------------------------------------
    void setCapacity(uint32_t const capacity);

    // -------------------------------------------------------------------------
    //! \return the Resource type name, such as "Water".
    // -------------------------------------------------------------------------
    [[nodiscard]] inline ResourceType const& getTypeName() const
    {
        return m_type;
    }

    // -------------------------------------------------------------------------
    //! \return the maximum amount that fits.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getCapacity() const
    {
        return m_capacity;
    }

    // -------------------------------------------------------------------------
    //! \return the current amount.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline uint32_t getAmount() const
    {
        return m_amount;
    }

    // -------------------------------------------------------------------------
    //! \return true when the stock is not empty.
    // -------------------------------------------------------------------------
    [[nodiscard]] inline bool hasAmount() const
    {
        return m_amount > 0u;
    }

    // -------------------------------------------------------------------------
    //! \brief Write as "type: amount/capacity". Used in unit test output.
    // -------------------------------------------------------------------------
    friend std::ostream& operator<<(std::ostream& os, Resource const& resource);

public:

    //! \brief Maximum allowed capacity. Also the default for a new Resource.
    //! add() guards against uint32_t overflow.
    static const uint32_t MAX_CAPACITY;

protected:

    //! \brief Resource type name.
    ResourceType m_type;
    //! \brief Maximum amount that fits.
    uint32_t m_capacity = Resource::MAX_CAPACITY;
    //! \brief Current amount. Never above m_capacity.
    uint32_t m_amount = 0u;
};

} // namespace ogb

#endif
