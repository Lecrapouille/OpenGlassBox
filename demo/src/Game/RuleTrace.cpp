//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Game/RuleTrace.hpp"

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Map.hpp"
#include "OpenGlassBox/Unit.hpp"

namespace ogb {
namespace game {


// ----------------------------------------------------------------------------
RuleTrace::~RuleTrace()
{
    if (IRule::listener() == this)
    {
        IRule::setListener(nullptr);
    }
}

// ----------------------------------------------------------------------------
void RuleTrace::setRecording(bool recording)
{
    m_recording = recording;
    IRule::setListener(recording ? this : nullptr);
}

// ----------------------------------------------------------------------------
void RuleTrace::clear()
{
    m_events.clear();
    m_head = 0u;
    m_total = 0u;
}

// ----------------------------------------------------------------------------
RuleEvent const& RuleTrace::at(size_t index) const
{
    return m_events[(m_head + index) % m_events.size()];
}

// ----------------------------------------------------------------------------
void RuleTrace::onRuleExecuted(IRule::Trace const& trace)
{
    if (trace.success && m_failures_only)
        return;

    RuleContext const& context = *trace.context;

    RuleEvent event;
    event.tick = m_tick;
    event.rule = trace.rule->type();
    event.success = trace.success;
    event.u = context.u;
    event.v = context.v;

    if (context.city != nullptr)
    {
        event.city = context.city->name();
    }

    if (context.unit != nullptr)
    {
        event.entity = "Unit " + context.unit->type();
    }
    else if (context.map != nullptr)
    {
        event.entity = "Map " + context.map->type();
    }
    else
    {
        event.entity = "?";
    }

    if (!trace.success &&
        (trace.blockingCommand < trace.rule->commands().size()))
    {
        event.blockedBy = trace.rule->commands()[trace.blockingCommand]->type();
    }

    ++m_total;

    if (m_events.size() < CAPACITY)
    {
        m_events.push_back(std::move(event));
    }
    else
    {
        m_events[m_head] = std::move(event);
        m_head = (m_head + 1u) % CAPACITY;
    }
}
} // namespace game
} // namespace ogb
