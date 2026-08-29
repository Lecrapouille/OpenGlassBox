//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/OpeningHours.hpp"
#include "OpenGlassBox/RuleCommand.hpp"

namespace ogb
{

//------------------------------------------------------------------------------
//! \brief The half open window [from, to) as a bitmask of the hours of a day,
//! wrapping around midnight the way SimulationClock::isHourBetween() does.
//------------------------------------------------------------------------------
static uint32_t windowMask(uint32_t from, uint32_t to)
{
    uint32_t const first = from % OpeningHours::HOURS_PER_DAY;
    uint32_t const last = to % OpeningHours::HOURS_PER_DAY;
    if (first == last)
        return 0u;

    uint32_t mask = 0u;
    for (uint32_t hour = first; hour != last;
         hour = (hour + 1u) % OpeningHours::HOURS_PER_DAY)
    {
        mask |= (1u << hour);
    }
    return mask;
}

//------------------------------------------------------------------------------
void OpeningHours::add(IRule const& rule)
{
    m_empty = false;

    // Every command of the rule has to validate for the rule to fire, so two
    // windows on the same rule narrow each other down, while the windows of two
    // rules add up. A rule asking for no hour at all is awake all day.
    uint32_t mask = ~0u;
    bool timed = false;
    for (IRuleCommand const* command: rule.getCommands())
    {
        auto const* hour = dynamic_cast<RuleCommandHour const*>(command);
        if (hour == nullptr)
            continue;

        mask &= windowMask(hour->getFrom(), hour->getTo());
        timed = true;
    }

    if (!timed)
    {
        m_anyHour = true;
        return;
    }

    m_hours |= mask;
}

//------------------------------------------------------------------------------
bool OpeningHours::isOpen(uint32_t hourOfDay) const
{
    if (m_empty || m_anyHour)
        return true;

    return ((m_hours >> (hourOfDay % HOURS_PER_DAY)) & 1u) != 0u;
}

//------------------------------------------------------------------------------
bool OpeningHours::isRestricted() const
{
    return !m_empty && !m_anyHour;
}

//------------------------------------------------------------------------------
uint32_t OpeningHours::getNextOpeningHour(uint32_t hourOfDay) const
{
    if (isOpen(hourOfDay))
        return hourOfDay % HOURS_PER_DAY;

    for (uint32_t i = 1u; i <= HOURS_PER_DAY; ++i)
    {
        uint32_t const hour = (hourOfDay + i) % HOURS_PER_DAY;
        if (((m_hours >> hour) & 1u) != 0u)
            return hour;
    }

    return NEVER;
}

//------------------------------------------------------------------------------
uint32_t OpeningHours::getClosingHour(uint32_t hourOfDay) const
{
    if (!isRestricted() || !isOpen(hourOfDay))
        return NEVER;

    uint32_t last = hourOfDay % HOURS_PER_DAY;
    for (uint32_t i = 1u; i < HOURS_PER_DAY; ++i)
    {
        uint32_t const hour = (hourOfDay + i) % HOURS_PER_DAY;
        if (((m_hours >> hour) & 1u) == 0u)
            break;
        last = hour;
    }

    return last;
}

} // namespace ogb
