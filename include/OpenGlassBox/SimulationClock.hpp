//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimulationClock.hpp
//! \brief In-game clock derived from the simulation tick counter.


#ifndef OPEN_GLASSBOX_SIMULATION_CLOCK_HPP
#  define OPEN_GLASSBOX_SIMULATION_CLOCK_HPP

#  include <cstdint>

namespace ogb {

//==============================================================================
//! \brief Calendar of a Simulation, derived from the number of ticks elapsed.
//!
//! The conversion is owned by the clock rather than hard-wired in the rules:
//! \c ticksPerMinute is a runtime setting, so a script that wants a shorter
//! day just changes the config. At the default of 20 ticks per minute, one
//! second of game time is one game minute, and a day lasts 24 minutes of game
//! time at speed x1.
//==============================================================================
class SimulationClock
{
public:

    SimulationClock() = default;

    explicit SimulationClock(uint32_t ticksPerMinute)
        : m_ticksPerMinute(ticksPerMinute == 0u ? 1u : ticksPerMinute)
    {}

    //--------------------------------------------------------------------------
    //! \brief Advance the calendar by one simulation tick.
    //--------------------------------------------------------------------------
    void tick() { ++m_ticks; }
    void setTicks(uint64_t ticks) { m_ticks = ticks; }

    //--------------------------------------------------------------------------
    //! \brief Move the calendar to a date and a time of day. A simulation that
    //! opens at midnight makes the player wait height game hours before anybody
    //! leaves for work, so both the save format and the demo need to say when
    //! the city wakes up.
    //--------------------------------------------------------------------------
    void setTimeOfDay(uint32_t day, uint32_t hour, uint32_t minute)
    {
        uint64_t const minutes =
            (uint64_t(day) * 24u + uint64_t(hour % 24u)) * 60u +
            uint64_t(minute % 60u);
        m_ticks = minutes * uint64_t(m_ticksPerMinute);
    }

    //--------------------------------------------------------------------------
    //! \brief Number of ticks elapsed since the beginning of the simulation.
    //--------------------------------------------------------------------------
    uint64_t ticks() const { return m_ticks; }

    //--------------------------------------------------------------------------
    //! \brief How many ticks make one game minute. Never zero.
    //--------------------------------------------------------------------------
    uint32_t ticksPerMinute() const { return m_ticksPerMinute; }
    void setTicksPerMinute(uint32_t value)
    {
        m_ticksPerMinute = (value == 0u) ? 1u : value;
    }

    //--------------------------------------------------------------------------
    //! \brief Minute of the current hour, in [0..59].
    //--------------------------------------------------------------------------
    uint32_t minuteOfHour() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) % 60u);
    }

    //--------------------------------------------------------------------------
    //! \brief Hour of the current day, in [0..23].
    //--------------------------------------------------------------------------
    uint32_t hourOfDay() const
    {
        return uint32_t(((m_ticks / m_ticksPerMinute) / 60u) % 24u);
    }

    //--------------------------------------------------------------------------
    //! \brief Number of full days elapsed.
    //--------------------------------------------------------------------------
    uint32_t day() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) / (60u * 24u));
    }

    //--------------------------------------------------------------------------
    //! \brief Whether the current hour is in [from, to). Wraps around midnight
    //! when \c from is greater than \c to, so that "hour between 22 6" means
    //! night.
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

    uint64_t m_ticks = 0u;
    uint32_t m_ticksPerMinute = 20u;
};

} // namespace ogb

#endif
