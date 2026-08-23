//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimulationClock.hpp
//! \brief In-game clock derived from the simulation tick counter.

#ifndef OPEN_GLASSBOX_SIMULATION_CLOCK_HPP
#define OPEN_GLASSBOX_SIMULATION_CLOCK_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief The calendar of the game: what hour of what day it is, worked out
//! from the number of ticks elapsed.
//!
//! A tick is the unit the simulation counts in, and it means nothing on its
//! own. This class is what turns it into a time of day, so that a rule can say
//! \c hour \c between \c 8 \c 18 instead of counting ticks, and so that a
//! building can be open or shut.
//!
//! The conversion belongs to the clock rather than being hard-wired into the
//! rules: \c ticksPerMinute is a runtime setting, so a ruleset wanting a
//! shorter day changes the settings instead of every rule. At the default of
//! twenty ticks per minute and twenty ticks per second of game time, one second
//! of game time is one game minute, and a day goes by in twenty-four minutes of
//! game time at normal speed.
//!
//! Days are counted from zero and never wrap: there is no week and no month,
//! only "how many days since the town was founded".
//!
//! Example:
//! \code
//! SimulationClock& clock = simulation.clock();
//! clock.setTimeOfDay(0u, 8u, 0u); // Open at eight in the morning.
//!
//! if (clock.hourBetween(22u, 6u))
//!     std::cout << "night of day " << clock.day() << '\n';
//! \endcode
//!
//! The matching script, which is what a rule reads it through:
//! \code
//! unitRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 18
//!     local People greater 0
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class SimulationClock
{
public:

    //--------------------------------------------------------------------------
    //! \brief A clock at the very start of the first day, at the default rate.
    //--------------------------------------------------------------------------
    SimulationClock() = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] ticksPerMinute how many ticks make one game minute.
    //! Zero is read as one: a minute has to take some time.
    //--------------------------------------------------------------------------
    explicit SimulationClock(uint32_t ticksPerMinute)
        : m_ticksPerMinute(ticksPerMinute == 0u ? 1u : ticksPerMinute)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Move the calendar on by one tick. Called once per tick by the
    //! World.
    //--------------------------------------------------------------------------
    void tick()
    {
        ++m_ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the tick count outright, which only a save being loaded has
    //! any business doing.
    //! \param[in] ticks the count to restore.
    //--------------------------------------------------------------------------
    void setTicks(uint64_t ticks)
    {
        m_ticks = ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief Move the calendar to a date and a time of day.
    //!
    //! A game opening at midnight makes the player wait eight game hours before
    //! anybody leaves for work, so both the save format and the demo need a way
    //! of saying when the town wakes up.
    //!
    //! \param[in] day which day, counted from zero. Not wrapped.
    //! \param[in] hour which hour, taken modulo twenty-four.
    //! \param[in] minute which minute, taken modulo sixty.
    //--------------------------------------------------------------------------
    void setTimeOfDay(uint32_t day, uint32_t hour, uint32_t minute)
    {
        uint64_t const minutes =
            (uint64_t(day) * 24u + uint64_t(hour % 24u)) * 60u +
            uint64_t(minute % 60u);
        m_ticks = minutes * uint64_t(m_ticksPerMinute);
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many ticks have gone by since the beginning. The only
    //! state the clock holds: everything else is worked out from it.
    //--------------------------------------------------------------------------
    uint64_t ticks() const
    {
        return m_ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many ticks make one game minute. Never zero.
    //--------------------------------------------------------------------------
    uint32_t ticksPerMinute() const
    {
        return m_ticksPerMinute;
    }

    //--------------------------------------------------------------------------
    //! \brief Change how fast the day goes by.
    //!
    //! \note The tick count is left alone, so the current date changes: at
    //! twice the ticks per minute, the same count of ticks is half as much game
    //! time.
    //!
    //! \param[in] value how many ticks make one game minute. Zero is read as
    //! one.
    //--------------------------------------------------------------------------
    void setTicksPerMinute(uint32_t value)
    {
        m_ticksPerMinute = (value == 0u) ? 1u : value;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the minute of the current hour, in [0..59].
    //--------------------------------------------------------------------------
    uint32_t minuteOfHour() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) % 60u);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the hour of the current day, in [0..23]. What the opening
    //! hours of a building are read against.
    //--------------------------------------------------------------------------
    uint32_t hourOfDay() const
    {
        return uint32_t(((m_ticks / m_ticksPerMinute) / 60u) % 24u);
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many whole days have gone by, counted from zero.
    //--------------------------------------------------------------------------
    uint32_t day() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) / (60u * 24u));
    }

    //--------------------------------------------------------------------------
    //! \brief Is it inside a window of the day? What \c hour \c between asks.
    //!
    //! \param[in] from first hour of the window, included.
    //! \param[in] to first hour past the window, excluded. When it is the
    //! smaller of the two the window wraps around midnight, so
    //! \c hourBetween(22, 6) means night. When the two are equal the window
    //! holds no hour at all and the answer is always false.
    //! \return true while the current hour falls inside the window.
    //--------------------------------------------------------------------------
    bool hourBetween(uint32_t from, uint32_t to) const
    {
        uint32_t const hour = hourOfDay();
        if (from == to)
            return false;
        if (from < to)
            return (hour >= from) && (hour < to);
        return (hour >= from) || (hour < to);
    }

private:

    //! \brief How many ticks have gone by since the beginning. Wide enough that
    //! it will not wrap around in any game anybody plays.
    uint64_t m_ticks = 0u;
    //! \brief How many ticks make one game minute. Never zero.
    uint32_t m_ticksPerMinute = 20u;
};

} // namespace ogb

#endif
