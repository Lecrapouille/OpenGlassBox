//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimulationClock.hpp
//! \brief Game calendar, derived from the tick counter.

#ifndef OPEN_GLASSBOX_SIMULATION_CLOCK_HPP
#define OPEN_GLASSBOX_SIMULATION_CLOCK_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief Game calendar: day, hour and minute, worked out from the tick
//! counter.
//!
//! Ticks are raw simulation steps. This class turns them into a time of day, so
//! a rule can say \c hour \c between \c 8 \c 18 instead of counting ticks.
//!
//! TimeConfig::ticksPerMinute sets the pace. With the default of twenty ticks
//! per minute and twenty ticks per second of game time, one game second lasts
//! one game minute and one day lasts twenty-four game minutes.
//!
//! Days start at zero and never wrap.
//!
//! Example:
//! \code
//! simulation.setTimeOfDay(0u, 8u, 0u); // start at eight in the morning
//!
//! if (simulation.getClock().isHourBetween(22u, 6u))
//!     std::cout << "night of day " << simulation.getClock().getDay() << '\n';
//! \endcode
//==============================================================================
class SimulationClock
{
public:

    //--------------------------------------------------------------------------
    //! \brief A clock at the very start of the first day, at the default pace.
    //--------------------------------------------------------------------------
    SimulationClock() = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in] ticksPerMinute ticks in one game minute. Zero is read
    //! as one: a minute has to take some time.
    //--------------------------------------------------------------------------
    explicit SimulationClock(uint32_t ticksPerMinute)
        : m_ticksPerMinute(ticksPerMinute == 0u ? 1u : ticksPerMinute)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Move the calendar on by one tick. Called once per tick by
    //! World::update().
    //--------------------------------------------------------------------------
    void tick()
    {
        ++m_ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief Restore the tick counter. For loading a save.
    //! \param[in] ticks the counter to restore.
    //--------------------------------------------------------------------------
    void setTicks(uint64_t ticks)
    {
        m_ticks = ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the date. Used at startup and when loading a save.
    //! \param[in] day whole days gone by, counted from zero.
    //! \param[in] hour hour of the day, in [0..23].
    //! \param[in] minute minute of the hour, in [0..59].
    //--------------------------------------------------------------------------
    void setTimeOfDay(uint32_t day, uint32_t hour, uint32_t minute)
    {
        uint64_t const minutes =
            (uint64_t(day) * 24u + uint64_t(hour % 24u)) * 60u +
            uint64_t(minute % 60u);
        m_ticks = minutes * uint64_t(m_ticksPerMinute);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the ticks gone by since the beginning. The only state the
    //! clock holds: everything else is worked out from it.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint64_t getTicks() const
    {
        return m_ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the ticks in one game minute. Never zero.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTicksPerMinute() const
    {
        return m_ticksPerMinute;
    }

    //--------------------------------------------------------------------------
    //! \brief Change how fast the day goes by.
    //!
    //! \note The tick counter is left alone, so the current date changes: at
    //! twice the ticks per minute, the same counter is half as much game time.
    //!
    //! \param[in] value ticks in one game minute. Zero is read as one.
    //--------------------------------------------------------------------------
    void setTicksPerMinute(uint32_t value)
    {
        m_ticksPerMinute = (value == 0u) ? 1u : value;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the minute of the current hour, in [0..59].
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getMinuteOfHour() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) % 60u);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the hour of the current day, in [0..23]. This is what the
    //! opening hours of a building are read against.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getHourOfDay() const
    {
        return uint32_t(((m_ticks / m_ticksPerMinute) / 60u) % 24u);
    }

    //--------------------------------------------------------------------------
    //! \brief \return the whole days gone by, counted from zero.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getDay() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) / (60u * 24u));
    }

    //--------------------------------------------------------------------------
    //! \brief Is it inside a window of the day? This is what \c hour \c between
    //! asks.
    //!
    //! \param[in] from first hour of the window, included.
    //! \param[in] to first hour past the window, excluded. When it is the
    //! smaller of the two, the window wraps around midnight, so
    //! \c isHourBetween(22, 6) means night. When the two are equal the window
    //! holds no hour at all and the answer is always false.
    //! \return true while the current hour falls inside the window.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isHourBetween(uint32_t from, uint32_t to) const
    {
        uint32_t const hour = getHourOfDay();
        if (from == to)
            return false;
        if (from < to)
            return (hour >= from) && (hour < to);
        return (hour >= from) || (hour < to);
    }

private:

    //! \brief Ticks gone by since the beginning. Wide enough that it will not
    //! wrap in any game anybody plays.
    uint64_t m_ticks = 0u;
    //! \brief Ticks in one game minute. Never zero.
    uint32_t m_ticksPerMinute = 20u;
};

} // namespace ogb

#endif
