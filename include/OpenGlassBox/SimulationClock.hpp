//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file SimulationClock.hpp
//! \brief Game calendar derived from the tick counter.

#ifndef OPEN_GLASSBOX_SIMULATION_CLOCK_HPP
#define OPEN_GLASSBOX_SIMULATION_CLOCK_HPP

#include <cstdint>

namespace ogb
{

//==============================================================================
//! \brief Game calendar: day, hour, and minute from the tick counter.
//!
//! Ticks are raw simulation steps. This class converts them to time of day.
//! Rules can write \c hour \c between \c 8 \c 18 instead of counting ticks.
//!
//! TimeConfig::ticksPerMinute sets the pace. Default: 20 ticks per minute and
//! 20 ticks per second of game time. One game second equals one game minute.
//! One day lasts 24 game minutes.
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
    //! \brief Clock at the start of day zero, default pace.
    //--------------------------------------------------------------------------
    SimulationClock() = default;

    //--------------------------------------------------------------------------
    //! \param[in] ticksPerMinute ticks in one game minute. Zero is treated as 1.
    //--------------------------------------------------------------------------
    explicit SimulationClock(uint32_t ticksPerMinute)
        : m_ticksPerMinute(ticksPerMinute == 0u ? 1u : ticksPerMinute)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Advance the calendar by one tick. Called once per tick by World::update().
    //--------------------------------------------------------------------------
    void tick()
    {
        ++m_ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief Restore the tick counter when loading a save.
    //! \param[in] ticks tick count to restore.
    //--------------------------------------------------------------------------
    void setTicks(uint64_t ticks)
    {
        m_ticks = ticks;
    }

    //--------------------------------------------------------------------------
    //! \brief Set the date. Used at startup and when loading a save.
    //! \param[in] day days since start, from zero.
    //! \param[in] hour hour in [0..23].
    //! \param[in] minute minute in [0..59].
    //--------------------------------------------------------------------------
    void setTimeOfDay(uint32_t day, uint32_t hour, uint32_t minute)
    {
        uint64_t const minutes =
            (uint64_t(day) * 24u + uint64_t(hour % 24u)) * 60u +
            uint64_t(minute % 60u);
        m_ticks = minutes * uint64_t(m_ticksPerMinute);
    }

    //--------------------------------------------------------------------------
    //! \return ticks since the start. All other fields derive from this.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint64_t getTicks() const
    {
        return m_ticks;
    }

    //--------------------------------------------------------------------------
    //! \return ticks in one game minute. Never zero.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTicksPerMinute() const
    {
        return m_ticksPerMinute;
    }

    //--------------------------------------------------------------------------
    //! \brief Change how fast the day passes.
    //!
    //! \note The tick counter is unchanged. The current date changes:
    //! twice the ticks per minute means half as much game time for the same ticks.
    //!
    //! \param[in] value ticks in one game minute. Zero is treated as 1.
    //--------------------------------------------------------------------------
    void setTicksPerMinute(uint32_t value)
    {
        m_ticksPerMinute = (value == 0u) ? 1u : value;
    }

    //--------------------------------------------------------------------------
    //! \return minute of the current hour, in [0..59].
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getMinuteOfHour() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) % 60u);
    }

    //--------------------------------------------------------------------------
    //! \return hour of the current day, in [0..23].
    //! Used by Building OpeningHours.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getHourOfDay() const
    {
        return uint32_t(((m_ticks / m_ticksPerMinute) / 60u) % 24u);
    }

    //--------------------------------------------------------------------------
    //! \return days since start, from zero.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getDay() const
    {
        return uint32_t((m_ticks / m_ticksPerMinute) / (60u * 24u));
    }

    //--------------------------------------------------------------------------
    //! \brief Check if the current hour is inside a window.
    //! Used by \c hour \c between Rules.
    //!
    //! \param[in] from first hour included.
    //! \param[in] to first hour excluded. If \c to < \c from, the window wraps
    //! midnight (e.g. \c isHourBetween(22, 6) is night).
    //! If \c from == \c to, the window is empty and the result is always false.
    //! \return true if the current hour is inside the window.
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

    //! \brief Ticks since start. Large enough to avoid wrap in normal play.
    uint64_t m_ticks = 0u;
    //! \brief Ticks in one game minute. Never zero.
    uint32_t m_ticksPerMinute = 20u;
};

} // namespace ogb

#endif
