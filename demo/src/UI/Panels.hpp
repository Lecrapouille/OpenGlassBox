//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Panels.hpp
//! \brief Dockable debug panels: layers, inspector, charts, budget, traffic and
//! time.

#ifndef OPEN_GLASSBOX_DEMO_PANELS_HPP
#define OPEN_GLASSBOX_DEMO_PANELS_HPP

#include "Game/DebugState.hpp"
#include "Game/TimeSeries.hpp"
#include "Host/OpenGL.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ogb
{
namespace game
{
class RuleTrace;
}
namespace editor
{
class Editor;
}

namespace ui
{
class CityViewer;

// ****************************************************************************
//! \brief Whether a building has something to do at a given hour of the day,
//! in the words the inspector and the tooltips of the city view both use.
//!
//! A shop that sells nothing at three in the morning is not a broken shop, and
//! the only way to tell the two apart used to be to read the ruleset.
// ****************************************************************************
struct OpeningStatus
{
    //! \brief Whether the building keeps office hours at all. False for one
    //! whose rules may run at any hour: there is nothing to display then, and
    //! calling it open would suggest it could ever be shut.
    bool known = false;

    //! \brief Whether one of its rules may run at that hour.
    bool open = false;

    //! \brief "open until 18h" or "closed until 8h".
    std::string text;

    //! \brief Why it reads that way, for a tooltip. A building whose rules do
    //! not all keep hours is never shut, which is worth a sentence.
    std::string detail;
};

// ----------------------------------------------------------------------------
//! \brief Read the timetable a building gets from the \c hour \c between
//! conditions of its rules.
//! \param[in] building the building to read the rules of.
//! \param[in] hourOfDay the hour to answer for, in [0..23].
//! \return what to display, if anything. See OpeningStatus.
// ----------------------------------------------------------------------------
OpeningStatus openingStatus(Building const& building, uint32_t hourOfDay);

// ----------------------------------------------------------------------------
//! \brief Spell a number of ticks out as game time. A tick means nothing to a
//! reader who does not know how many of them make a minute, and how many that
//! is depends on TimeConfig::ticksPerMinute.
// ----------------------------------------------------------------------------
std::string gameTimeText(uint32_t ticks, uint32_t ticksPerMinute);

// ----------------------------------------------------------------------------
//! \brief The commands a rule runs, one per line, in the order the script
//! wrote them. The name a script gives a rule says nothing about what it does.
// ----------------------------------------------------------------------------
std::string commandsText(IRule const& rule);

// ****************************************************************************
//! \brief One-click heatmap picker: click a layer name to show it as the main
//! layer.
//!
//! It used to hang off the Layers tool of the toolbar, on the grounds that
//! choosing which layer to look at and which one to paint is the same
//! decision. But the toolbar sits above the canvas, so the list pushed the
//! city down every time the tool was armed, and reading a heatmap is worth
//! doing without a brush in hand. It is a dockable panel of its own instead.
// ****************************************************************************
class LayersPanel
{
public:

    // ------------------------------------------------------------------------
    //! \brief Draw the panel. One layer per row, the controls of every row
    //! aligned in columns.
    //! \param[in,out] open: cleared when the player closes the window.
    // ------------------------------------------------------------------------
    void draw(Simulation& simulation, game::DebugState& state, bool& open);
};

// ****************************************************************************
//! \brief Details whatever is selected: resources as progress bars, rules with
//! their period and the number of ticks left before the next attempt.
//!
//! What a rule does at large is a property of the script rather than of the
//! building under the cursor, so the breakdown of the whole ruleset sits in
//! the Script panel, under the text it comes from.
// ****************************************************************************
class InspectorPanel
{
public:

    void draw(Simulation& simulation,
              game::DebugState& state,
              game::RuleTrace const& trace) const;

private:

    void drawBuilding(Simulation& simulation,
                  game::DebugState& state,
                  game::RuleTrace const& trace) const;
    void drawAgent(Simulation& simulation, game::DebugState const& state) const;
    void drawNode(game::DebugState& state) const;
    void drawSegment(game::DebugState& state) const;
    void drawCell(Simulation& simulation, game::DebugState& state) const;
    void drawZone(Simulation& simulation, game::DebugState& state) const;
};

// ****************************************************************************
//! \brief Everything about the open ruleset (.ogs): which one it is, its text,
//! what it parses into, and whether the saves beside it still open.
//!
//! Apply reparses and keeps the city when every type still in use is still
//! defined. A save records the fingerprint of the ruleset it was written
//! against, which is what stops a city from being rebuilt with types that mean
//! something else; while a ruleset is being written that same fingerprint is
//! in the way, hence the option that waives it.
// ****************************************************************************
class ScriptPanel
{
public:

    // ------------------------------------------------------------------------
    //! \brief The fingerprints. Refreshed by the host when a file is read or
    //! written rather than every frame: hashing a file is not free.
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
        //! \brief Saves beside the ruleset whose fingerprint no longer matches
        //! it, and which therefore refuse to open.
        std::vector<std::string> staleSaves;
    };

    // ------------------------------------------------------------------------
    //! \brief The rulesets the player can switch to without leaving the panel.
    // ------------------------------------------------------------------------
    struct Files
    {
        //! \brief Path of the ruleset the editor holds.
        std::string current;
        //! \brief Every \c .ogs sitting in the same directory, this one
        //! included. Refreshed by the host when a file is read or written:
        //! walking a directory every frame is not free.
        std::vector<std::string> rulesets;
    };

    // ------------------------------------------------------------------------
    //! \brief The two switches that decide what the demo does behind the
    //! player's back. Owned by the host, which also offers them in its menu.
    // ------------------------------------------------------------------------
    struct Options
    {
        //! \brief Reparse the ruleset when the file changes under the editor,
        //! so that a script written in another editor is seen at once.
        bool autoReload = true;
        //! \brief Open a save whose fingerprint no longer matches the ruleset.
        bool ignoreMismatch = false;
    };

    // ------------------------------------------------------------------------
    //! \brief What the player asked for during this frame.
    // ------------------------------------------------------------------------
    struct Actions
    {
        bool apply = false;
        //! \brief Record the fingerprint of the ruleset as it is now into
        //! every stale save beside it.
        bool restampSaves = false;
        //! \brief Ruleset picked from the list, to be opened in place of the
        //! current one. Empty when nothing was picked.
        std::string openRuleset;
    };

    void draw(Simulation& simulation,
              std::string& text,
              std::string const& status,
              Checksum const& checksum,
              Files const& files,
              Options& options,
              Actions& actions);

private:

    // ------------------------------------------------------------------------
    //! \brief Every rule the open ruleset defines, broken down: which kind of
    //! entity runs it, how often, and what it does. It sits under the text it
    //! was parsed from, since that is what it answers questions about.
    // ------------------------------------------------------------------------
    void drawRuleset(Simulation& simulation);

private:

    //! \brief Substring the rule name or one of its commands must contain.
    std::string m_filter;
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

    void draw(Simulation& simulation,
              game::DebugState& state,
              game::RuleTrace& trace);

private:

    //! \brief Case insensitive substring the entity or the rule must contain.
    std::string m_filter;
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

    //! \brief Histories by group ("Layers", "Agents", "Globals", "Traffic") then
    //! by quantity name.
    std::map<std::string, std::map<std::string, game::TimeSeries>> m_series;
    //! \brief Interval in ticks between two samples, to keep long runs cheap.
    int m_sample_period = 5;
    uint64_t m_last_sample_tick = 0u;
    bool m_first_sample = true;
    //! \brief Whether raw samples are drawn. Off by itself, only the trend
    //! remains.
    bool m_show_raw = false;
    //! \brief Whether the smoothed curve is drawn next to the raw one.
    bool m_show_smoothed = true;
};

// ****************************************************************************
//! \brief The in-game clock and how fast it advances, one section per
//! question: which day it is and how to jump to another hour, how to step
//! through the ticks while paused, how fast a tick follows the next, and how
//! many of them a second is worth.
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
//! \brief The dials the player turns: what share of its asking price each
//! service is granted, and what rate each kind of building is taxed at.
//!
//! What a city owns and what it grants are two different things, and a city
//! needs to be able to tell them apart: a rich city that grants its police a
//! tenth of what it asks for has a thin patrol and money left in the bank. The
//! treasury is a stock the rules move; a dial is a setting only the player
//! moves, which is why it needs a panel at all. A rule can add and remove,
//! never set, so no rule could hold a setting at its value.
//!
//! The list of dials is not written here. A ruleset is the game, so hard coding
//! the services would tie the demo to one of them. The panel reads the resources
//! the script declared and keeps them by their name: a name ending in "Budget"
//! is a share of an asking price, from nothing to everything, and a name
//! starting with "Tax" is a rate. The convention is documented in
//! doc/script.md.
// ****************************************************************************
class BudgetPanel
{
public:

    void draw(Simulation& simulation);

    // ------------------------------------------------------------------------
    //! \brief Whether a resource is a share granted to a service.
    // ------------------------------------------------------------------------
    static bool isBudget(std::string const& name);

    // ------------------------------------------------------------------------
    //! \brief Whether a resource is a rate of taxation.
    // ------------------------------------------------------------------------
    static bool isTax(std::string const& name);

private:

    void drawCity(City& city, Simulation& simulation);

    //! \brief The treasury at the start of the game day being played, per city
    //! name, so that the panel can report what the day cost. The engine does
    //! not attribute a payment to a service, so this is the whole balance and
    //! not a breakdown.
    struct Ledger
    {
        uint32_t moneyAtDawn = 0u;
        uint32_t lastDay = 0u;
        int64_t previousDay = 0;
        bool seen = false;
    };

    std::map<std::string, Ledger> m_ledgers;
};

// ****************************************************************************
//! \brief Ranks the most saturated Segments and reports whether the network is
//! settling down.
// ****************************************************************************
class TrafficPanel
{
public:

    void draw(Simulation& simulation, game::DebugState& state);

    // ------------------------------------------------------------------------
    //! \brief Total travel time of the network, the sum over the Segments of the
    //! flow times the travel time. This is the quantity the Wardrop equilibrium
    //! minimizes, so watching it settle tells whether the routing converged.
    // ------------------------------------------------------------------------
    static float totalTravelTime(Simulation& simulation);
};
} // namespace ui
} // namespace ogb

#endif
