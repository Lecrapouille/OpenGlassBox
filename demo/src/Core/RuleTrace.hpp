//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_DEMO_RULE_TRACE_HPP
#  define OPEN_GLASSBOX_DEMO_RULE_TRACE_HPP

#  include "OpenGlassBox/Rule.hpp"

#  include <cstdint>
#  include <string>
#  include <vector>

namespace ogb {
namespace core {


// ****************************************************************************
//! \brief One recorded attempt to run a rule, flattened into plain strings so
//! that the log survives the destruction of the entities it refers to.
// ****************************************************************************
struct RuleEvent
{
    uint64_t tick = 0u;
    std::string city;
    //! \brief "Unit Home" or "Map Water", the entity the rule ran on.
    std::string entity;
    std::string rule;
    //! \brief Description of the command that refused to validate, empty on
    //! success. This is the answer to "why does nothing happen".
    std::string blockedBy;
    uint32_t u = 0u;
    uint32_t v = 0u;
    bool success = false;
};

// ****************************************************************************
//! \brief Records the rule executions of the engine into a bounded ring buffer.
//!
//! Attaching this listener has a real cost, since a rule fires on every cell of
//! every map at every tick, so recording is opt-in and off by default.
// ****************************************************************************
class RuleTrace: public IRule::Listener
{
public:

    //! \brief Number of events kept. Older events are overwritten.
    static constexpr size_t CAPACITY = 4096u;

    // ------------------------------------------------------------------------
    //! \brief Detach from the engine.
    // ------------------------------------------------------------------------
    ~RuleTrace() override;

    // ------------------------------------------------------------------------
    //! \brief Start or stop recording. Attaching and detaching the listener is
    //! what makes the engine pay nothing while recording is off.
    // ------------------------------------------------------------------------
    void setRecording(bool recording);
    bool recording() const { return m_recording; }

    // ------------------------------------------------------------------------
    //! \brief Tick stamped on the events recorded from now on. Set by the host
    //! before each simulation step.
    // ------------------------------------------------------------------------
    void setTick(uint64_t tick) { m_tick = tick; }

    // ------------------------------------------------------------------------
    //! \brief Only record the failures. Successes are by far the most numerous
    //! and the least interesting when hunting a rule that never fires.
    // ------------------------------------------------------------------------
    void setFailuresOnly(bool failuresOnly) { m_failures_only = failuresOnly; }
    bool failuresOnly() const { return m_failures_only; }

    // ------------------------------------------------------------------------
    //! \brief Drop every recorded event.
    // ------------------------------------------------------------------------
    void clear();

    // ------------------------------------------------------------------------
    //! \brief Number of events currently held.
    // ------------------------------------------------------------------------
    size_t size() const { return m_events.size(); }

    // ------------------------------------------------------------------------
    //! \brief Access the nth event, zero being the oldest.
    // ------------------------------------------------------------------------
    RuleEvent const& at(size_t index) const;

    // ------------------------------------------------------------------------
    //! \brief Total number of events recorded since the last clear, including
    //! those already overwritten.
    // ------------------------------------------------------------------------
    uint64_t totalRecorded() const { return m_total; }

private:

    // ------------------------------------------------------------------------
    //! \brief Derived from IRule::Listener.
    // ------------------------------------------------------------------------
    void onRuleExecuted(IRule::Trace const& trace) override;

private:

    std::vector<RuleEvent> m_events;
    //! \brief Index of the oldest event inside m_events once it is full.
    size_t m_head = 0u;
    uint64_t m_tick = 0u;
    uint64_t m_total = 0u;
    bool m_recording = false;
    bool m_failures_only = true;
};
} // namespace core
} // namespace ogb

#endif
