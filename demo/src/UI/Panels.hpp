//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Panels.hpp
//! \brief Dockable debug panels: layers, inspector, charts, traffic and time.


#ifndef OPEN_GLASSBOX_DEMO_PANELS_HPP
#  define OPEN_GLASSBOX_DEMO_PANELS_HPP

#  include "Host/OpenGL.hpp"
#  include "Game/DebugState.hpp"
#  include "Game/TimeSeries.hpp"
#  include "OpenGlassBox/Simulation.hpp"

#  include <map>
#  include <memory>
#  include <string>
#  include <vector>

namespace ogb {
namespace game { class RuleTrace; }
namespace editor { class Editor; }

namespace ui {
class CityViewer;

// ****************************************************************************
//! \brief One-click heatmap picker: click a map name to show it as the main
//! layer. Lives next to the Maps tool of the toolbar, since choosing which map
//! to look at and choosing which map to paint is the same decision.
// ****************************************************************************
class LayersPanel
{
public:

    // ------------------------------------------------------------------------
    //! \brief One map per row, the controls of every row aligned in columns.
    //! \param[in] width: width in pixels of the column of rows.
    // ------------------------------------------------------------------------
    void drawColumn(Simulation& simulation, game::DebugState& state, float width);
};

// ****************************************************************************
//! \brief Details whatever is selected: resources as progress bars, rules with
//! their period and the number of ticks left before the next attempt.
//!
//! A second tab breaks the whole ruleset down, because what a rule does is a
//! property of the script rather than of the building under the cursor, and
//! reading it one building at a time says nothing about the city.
// ****************************************************************************
class InspectorPanel
{
public:

    void draw(Simulation& simulation, game::DebugState& state, game::RuleTrace const& trace);

private:

    void drawUnit(Simulation& simulation, game::DebugState& state,
                  game::RuleTrace const& trace);
    void drawAgent(Simulation& simulation, game::DebugState& state);
    void drawNode(game::DebugState& state);
    void drawWay(game::DebugState& state);
    void drawCell(Simulation& simulation, game::DebugState& state);
    void drawArea(Simulation& simulation, game::DebugState& state);
    void drawRuleset(Simulation& simulation);

private:

    //! \brief Substring the rule name or one of its commands must contain.
    char m_filter[64] = "";
};

// ****************************************************************************
//! \brief Editable source of the open ruleset (.ogs). Apply reparses and
//! keeps the city when every type still in use is still defined.
//!
//! It also reports the checksum a save has to match. A save records the
//! fingerprint of the ruleset it was written against, which is what stops a
//! city from being rebuilt with types that mean something else; while a
//! ruleset is being written that same fingerprint is in the way, hence the
//! button that reads it and the option that waives it.
// ****************************************************************************
class ScriptPanel
{
public:

    // ------------------------------------------------------------------------
    //! \brief The fingerprints, computed by the host on demand rather than
    //! every frame: hashing a file is not free.
    // ------------------------------------------------------------------------
    struct Checksum
    {
        //! \brief Whether the values below were ever computed.
        bool known = false;
        //! \brief SHA-256 of the ruleset as it sits on disk.
        std::string onDisk;
        //! \brief SHA-256 of the text in this editor, which drifts from the
        //! one above as soon as a character is typed and until Apply.
        std::string edited;
        //! \brief What the open save recorded, empty when no save is open.
        std::string save;
    };

    // ------------------------------------------------------------------------
    //! \brief What the player asked for during this frame.
    // ------------------------------------------------------------------------
    struct Actions
    {
        bool apply = false;
        bool computeChecksum = false;
        //! \brief Rewrite the open save so that it records the checksum of the
        //! ruleset as it is now.
        bool restampSave = false;
    };

    void draw(std::string& text, std::string const& status,
              Checksum const& checksum, bool& ignoreMismatch,
              Actions& actions);
};

// ****************************************************************************
//! \brief The filterable log of the rule executions.
//!
//! The engine validates every command of a rule and silently gives up on the
//! first refusal, so without this panel there is no way to know why a
//! simulation stays still. Each line names the command that blocked.
// ****************************************************************************
class RuleLogPanel
{
public:

    void draw(Simulation& simulation, game::DebugState& state, game::RuleTrace& trace);

private:

    //! \brief Case insensitive substring the entity or the rule must contain.
    char m_filter[64] = "";
    bool m_show_success = true;
    bool m_show_failure = true;
    bool m_auto_scroll = true;
};

// ****************************************************************************
//! \brief Plots the history of the aggregates of the simulation with ImPlot.
// ****************************************************************************
class ChartsPanel
{
public:

    // ------------------------------------------------------------------------
    //! \brief Append one sample of every tracked quantity. Called once per
    //! simulation tick, not once per frame, so that the abscissa is the tick.
    // ------------------------------------------------------------------------
    void sample(Simulation& simulation);

    void draw(Simulation& simulation, game::DebugState& state);

    void clear();

private:

    game::TimeSeries& series(std::string const& group, std::string const& name);

private:

    //! \brief Histories by group ("Maps", "Agents", "Globals", "Traffic") then
    //! by quantity name.
    std::map<std::string, std::map<std::string, game::TimeSeries>> m_series;
    //! \brief Interval in ticks between two samples, to keep long runs cheap.
    int m_sample_period = 5;
    uint64_t m_last_sample_tick = 0u;
    bool m_first_sample = true;
};

// ****************************************************************************
//! \brief The in-game clock and how fast it advances: stepping while paused,
//! time scale and tick rate. Play and Pause live on the map toolbar, next to
//! the tools they interact with.
// ****************************************************************************
class TimeControlPanel
{
public:

    void draw(Simulation& simulation);

    // ------------------------------------------------------------------------
    //! \brief Number of ticks the user asked to run while paused. Consumed by
    //! the host, which is the only one allowed to advance the simulation.
    // ------------------------------------------------------------------------
    uint32_t takePendingSteps();

private:

    void drawTimeOfDay(Simulation& simulation);

    uint32_t m_pending_steps = 0u;
    int m_step_size = 1;
    //! \brief Time of day being typed in. Mirrors the clock until the player
    //! touches it, so that the running clock does not fight the input.
    int m_hour = 8;
    int m_minute = 0;
    bool m_editing_time = false;
};

// ****************************************************************************
//! \brief Ranks the most saturated Ways and reports whether the network is
//! settling down.
// ****************************************************************************
class TrafficPanel
{
public:

    void draw(Simulation& simulation, game::DebugState& state);

    // ------------------------------------------------------------------------
    //! \brief Total travel time of the network, the sum over the Ways of the
    //! flow times the travel time. This is the quantity the Wardrop equilibrium
    //! minimizes, so watching it settle tells whether the routing converged.
    // ------------------------------------------------------------------------
    static float totalTravelTime(Simulation& simulation);
};
} // namespace ui
} // namespace ogb

#endif
