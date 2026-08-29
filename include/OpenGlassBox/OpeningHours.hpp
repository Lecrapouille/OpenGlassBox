//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file OpeningHours.hpp
//! \brief Hours of the day when a set of Rules is active.

#ifndef OPEN_GLASSBOX_OPENING_HOURS_HPP
#define OPEN_GLASSBOX_OPENING_HOURS_HPP

#include <cstdint>

namespace ogb
{

class IRule;

//==============================================================================
//! \brief When a Building is open, derived from \c hour \c between Rule conditions.
//!
//! A Rule with no hour condition may run at any hour.
//! If all Rules have no hour condition, the Building is always open.
//! If any Rule has hour conditions, the Building is closed outside those hours.
//!
//! Hours are stored as a 24-bit mask, not a single range.
//! Two Rules may have gaps between them (e.g. 8-10 and 14-18, closed at noon).
//!
//! Example:
//! \code
//! ogb::OpeningHours hours;
//! for (ogb::RuleBuilding const* rule: building.rules())
//!     hours.add(*rule);
//!
//! uint32_t const now = clock.hourOfDay();
//! if (hours.isOpen(now))
//!     std::cout << "open\n";
//! else
//!     std::cout << "closed until " << hours.nextOpening(now) << "h\n";
//! \endcode
//!
//! The matching script. The two Rules define the timetable:
//! \code
//! buildingRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 10
//!     agent Worker to Work add [ People 1 ]
//! end
//!
//! buildingRule ShopForGoods
//!     rate 3 hours
//!     hour between 14 18
//!     agent Shopper to Shop add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class OpeningHours
{
public:

    //! \brief Hours in one day. Also the number of bits in the mask.
    static constexpr uint32_t HOURS_PER_DAY = 24u;

    //! \brief Value returned by getNextOpeningHour() when no hour is open.
    static constexpr uint32_t NEVER = HOURS_PER_DAY;

    //--------------------------------------------------------------------------
    //! \brief Add hour conditions from one Rule.
    //!
    //! A Rule with no \c hour \c between may run at any hour. One such Rule
    //! keeps the Building open 24 hours.
    //! Several hour conditions on one Rule must all match (narrower window).
    //!
    //! \param[in] rule the Rule to read. Its commands are read only.
    //! Call again after the ruleset is reparsed.
    //--------------------------------------------------------------------------
    void add(IRule const& rule);

    //--------------------------------------------------------------------------
    //! \brief Check if any added Rule may run at this hour.
    //! \param[in] hourOfDay hour in [0..23]. Values above 23 wrap.
    //! \return true if at least one Rule is active. Also true if no Rule was added.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isOpen(uint32_t hourOfDay) const;

    //--------------------------------------------------------------------------
    //! \brief Check if any hour restriction was found.
    //! \return false if every Rule may run at any hour.
    //! Then isOpen() is always true and there is nothing to display.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isRestricted() const;

    //--------------------------------------------------------------------------
    //! \brief Find the next open hour.
    //! \param[in] hourOfDay start hour in [0..23].
    //! \return \c hourOfDay if already open, else the next open hour today,
    //! or NEVER if no hour is ever open.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getNextOpeningHour(uint32_t hourOfDay) const;

    //--------------------------------------------------------------------------
    //! \brief Find the last open hour before closing.
    //! \param[in] hourOfDay start hour in [0..23].
    //! \return last open hour in the current stretch, or NEVER if closed or
    //! always open.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getClosingHour(uint32_t hourOfDay) const;

private:

    //! \brief Bit h is set if a Rule with hours is active at hour h.
    uint32_t m_hours = 0u;

    //! \brief True if a Rule with no hour condition was added.
    bool m_anyHour = false;

    //! \brief True if at least one Rule was added. Empty means always open.
    bool m_empty = true;
};

} // namespace ogb

#endif
