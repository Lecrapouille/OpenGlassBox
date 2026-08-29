//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Game/RuleTrace.hpp"

#include "OpenGlassBox/City.hpp"
#include "OpenGlassBox/Layer.hpp"
#include "OpenGlassBox/Unit.hpp"

namespace ogb {
namespace game {


// ----------------------------------------------------------------------------
RuleTrace::~RuleTrace()
{
    if (IRule::getListener() == this)
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
    event.rule = trace.rule->getName();
    event.success = trace.success;
    event.u = context.cell.u;
    event.v = context.cell.v;

    if (context.city != nullptr)
    {
        event.city = context.city->getName();
    }

    if (context.unit != nullptr)
    {
        event.entity = "Unit " + context.unit->getTypeName().str();
    }
    else if (context.layer != nullptr)
    {
        event.entity = "Layer " + context.layer->getTypeName().str();
    }
    else
    {
        event.entity = "?";
    }

    if (!trace.success &&
        (trace.blockingCommand < trace.rule->getCommands().size()))
    {
        event.blockedBy =
            trace.rule->getCommands()[trace.blockingCommand]->getDescription();
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
