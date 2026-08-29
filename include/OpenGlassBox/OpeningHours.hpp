//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file OpeningHours.hpp
//! \brief The hours of the day a set of rules is awake.

#ifndef OPEN_GLASSBOX_OPENING_HOURS_HPP
#define OPEN_GLASSBOX_OPENING_HOURS_HPP

#include <cstdint>

namespace ogb
{

class IRule;

//==============================================================================
//! \brief When a building has something to do, read from the \c hour \c between
//! conditions of its rules.
//!
//! A rule with no such condition may fire at any hour of the day, and a
//! building whose rules are all like that never closes. As soon as one rule
//! keeps office hours, the building is shut outside of them: a shop that sells
//! nothing at three in the morning is not a broken shop, and saying so in the
//! inspector saves the reader the trip through the ruleset.
//!
//! The hours are held as a bitmask of the twenty-four hours of a day rather
//! than as a range, because the windows of two rules need not touch: a house
//! that sends workers out from 8 to 10 and shoppers from 14 to 18 has nobody
//! going anywhere at noon.
//!
//! Example:
//! \code
//! ogb::OpeningHours hours;
//! for (ogb::RuleUnit const* rule: unit.rules())
//!     hours.add(*rule);
//!
//! uint32_t const now = clock.hourOfDay();
//! if (hours.isOpen(now))
//!     std::cout << "open\n";
//! else
//!     std::cout << "closed until " << hours.nextOpening(now) << "h\n";
//! \endcode
//!
//! The matching script, where the two rules are what draws the timetable:
//! \code
//! unitRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 10
//!     agent Worker to Work add [ People 1 ]
//! end
//!
//! unitRule ShopForGoods
//!     rate 3 hours
//!     hour between 14 18
//!     agent Shopper to Shop add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class OpeningHours
{
public:

    //! \brief Hours in a day, and therefore bits used by the mask.
    static constexpr uint32_t HOURS_PER_DAY = 24u;

    //! \brief Returned by nextOpening() when no hour of the day opens.
    static constexpr uint32_t NEVER = HOURS_PER_DAY;

    //--------------------------------------------------------------------------
    //! \brief Take the calendar conditions of one rule into account.
    //!
    //! A rule holding no \c hour \c between condition may run at any hour, and
    //! one such rule is enough to keep the building open around the clock. A
    //! rule holding several of them is awake when all of them agree, which is
    //! how a window is narrowed down.
    //!
    //! \param[in] rule the rule to read the conditions of. Its commands are
    //! only read, and nothing is kept: call this again after the ruleset has
    //! been reparsed.
    //--------------------------------------------------------------------------
    void add(IRule const& rule);

    //--------------------------------------------------------------------------
    //! \brief Whether one of the rules seen so far may run at that hour.
    //! \param[in] hourOfDay hour of the day, in [0..23]. Larger values wrap.
    //! \return true when at least one rule is awake then, and true as well when
    //! no rule has been added at all: what has no timetable is never shut.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isOpen(uint32_t hourOfDay) const;

    //--------------------------------------------------------------------------
    //! \brief Whether any timetable was found at all.
    //! \return false when every rule seen so far may run at any hour, in which
    //! case isOpen() is true everywhere and there is nothing to display.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isRestricted() const;

    //--------------------------------------------------------------------------
    //! \brief The next hour of the day the doors open.
    //! \param[in] hourOfDay hour to start looking from, in [0..23].
    //! \return \c hourOfDay itself when it is already open, the first open hour
    //! of the coming day otherwise, or NEVER when no hour of the day opens.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getNextOpeningHour(uint32_t hourOfDay) const;

    //--------------------------------------------------------------------------
    //! \brief The last hour of the day still open before the doors shut.
    //! \param[in] hourOfDay hour to start looking from, in [0..23].
    //! \return the last open hour of the current stretch, or NEVER when
    //! \c hourOfDay is closed or when nothing ever closes.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getClosingHour(uint32_t hourOfDay) const;

private:

    //! \brief Bit h set when a rule with a timetable is awake at hour h.
    uint32_t m_hours = 0u;

    //! \brief A rule with no timetable was seen: it may run at any hour.
    bool m_anyHour = false;

    //! \brief A rule was seen at all. An empty set is open, not shut.
    bool m_empty = true;
};

} // namespace ogb

#endif
