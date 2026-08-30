//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Rule.hpp
//! \brief Base Rule classes. Rules for Layer, Building and Zone.

#ifndef OPEN_GLASSBOX_RULE_HPP
#define OPEN_GLASSBOX_RULE_HPP

#include "OpenGlassBox/SimulationClock.hpp"
#include "OpenGlassBox/Types.hpp"
#include "OpenGlassBox/Vector.hpp"

#include <algorithm>
#include <cstddef>

namespace ogb
{

class City;
class Building;
class Layer;
class Zone;
class Resources;

//==============================================================================
//! \brief What a Rule can see and change while it runs.
//!
//! A Rule has no state. The engine passes all context each run.
//! Set \c building for a Building Rule, \c layer for a Layer Rule, \c zone for a Zone Rule.
//! \c city is always set.
//!
//! The entity keeps one context and reuses it each tick.
//! A Layer Rule runs on many cells. One context per cell costs too much.
//==============================================================================
struct RuleContext
{
    //! \brief The City this Rule runs for. Never null during a run.
    City* city = nullptr;
    //! \brief The Building this Rule belongs to, or null if none.
    Building* building = nullptr;
    //! \brief The Layer this Rule belongs to, or null.
    //! Used so the debugger knows where a trace came from.
    //! Commands reach the Layer through the City.
    Layer* layer = nullptr;
    //! \brief The Zone this Rule belongs to, or null.
    Zone* zone = nullptr;
    //! \brief The calendar. Never null during a run.
    //! Commands like \c hour \c between need it.
    //! A Rule without a clock never runs when it asks for time.
    SimulationClock const* clock = nullptr;
    //! \brief What \c local reads and writes.
    //! Resources of the Building or of the cell the Rule runs on.
    Resources* locals = nullptr;
    //! \brief What \c global reads and writes.
    //! City-wide resources, such as Money.
    Resources* globals = nullptr;
    //! \brief The cell the Rule acts on.
    //! Coordinates are signed. The grid has no fixed bounds.
    Cell cell;
    //! \brief How far a Layer command may reach from \c cell.
    //! Taxicab distance. Comes from the Building footprint.
    uint32_t radius = 0u;
};

//==============================================================================
//! \brief One line in a Rule, as written in the script.
//!
//! A Rule is made of commands: test a resource, test the hour,
//! add to a Layer, send an Agent, spawn a Building, and more.
//! Each command runs in two steps:
//! first check if it can run, then run it.
//! That makes a Rule all-or-nothing.
//!
//! The parser creates commands. The Ruleset owns them.
//! Every entity shares the same commands.
//! Commands have no state. They read from RuleContext.
//==============================================================================
class IRuleCommand
{
public:

    virtual ~IRuleCommand() = default;

    //--------------------------------------------------------------------------
    //! \brief Can this command run now?
    //!
    //! The engine asks every command before any runs.
    //! Do not change anything here.
    //!
    //! \param[in,out] context where the Rule runs.
    //! \return false stops the whole Rule. Nothing runs.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief Run the command.
    //! Called only after every command passed validate.
    //! \param[in,out] context where the Rule runs.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \return the command as text, close to the script.
    //! Used by the rule log and the demo inspector.
    //! Built on demand. Not stored.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual std::string getDescription() const = 0;
};

//==============================================================================
//! \brief Where a command reads or writes a number.
//!
//! The script writes \c local \c People, \c global \c Money, or \c layer \c Water.
//! Each becomes an IRuleValue.
//! One interface lets one command, such as \c add, work for all three.
//! The command knows the amount. The value knows the place.
//!
//! See RuleValue.hpp for the three types.
//==============================================================================
class IRuleValue
{
public:

    virtual ~IRuleValue() = default;

    //--------------------------------------------------------------------------
    //! \param[in,out] context where the Rule runs.
    //! \return the current amount.
    //! For a Layer over a footprint, the sum over covered cells.
    //--------------------------------------------------------------------------
    virtual uint32_t get(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \param[in,out] context where the Rule runs.
    //! \return the maximum amount.
    //! Used for proportion tests.
    //! For a Layer, cell cap times number of covered cells.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual uint32_t getCapacity(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief Add up to capacity. Extra amount is dropped.
    //! \param[in,out] context where the Rule runs.
    //! \param[in] toAdd how much to add.
    //--------------------------------------------------------------------------
    virtual void add(RuleContext& context, uint32_t toAdd) = 0;

    //--------------------------------------------------------------------------
    //! \brief Remove down to zero. Missing amount is not taken.
    //! \param[in,out] context where the Rule runs.
    //! \param[in] toRemove how much to remove.
    //--------------------------------------------------------------------------
    virtual void remove(RuleContext& context, uint32_t toRemove) = 0;

    //--------------------------------------------------------------------------
    //! \return the resource name, such as "People".
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual Name const& getTypeName() const = 0;
};

//==============================================================================
//! \brief A Rule: a name, how often it runs, and its commands.
//!
//! This is the scripting language at runtime.
//! Each entity runs its Rules every N ticks.
//! The engine asks each command if it can run.
//! If all agree, it runs them all.
//! A Rule never runs halfway.
//! Example: "remove 1 People and add 1 to a car" does both or neither.
//!
//! The Ruleset owns Rules. Every entity of one kind shares them.
//! Rules have no state. They use RuleContext instead.
//!
//! Example:
//! \code
//! // Rules come from the script, not from hand-written code.
//! // Running one looks like this:
//! RuleContext context;
//! context.city = &city;
//! context.building = &home;
//! context.clock = &simulation.getClock();
//! context.locals = &home.getResources();
//! context.globals = &city.getGlobals();
//!
//! for (RuleBuilding* rule : home.getRules())
//! {
//!     // The script may name a Rule that does not exist.
//!     if ((rule != nullptr) &&
//!         (home.getTicks() % rule->getPeriodTicks(ticksPerMinute) == 0u))
//!     {
//!         rule->execute(context);
//!     }
//! }
//! \endcode
//!
//! Matching script. Each body line is one command:
//! \code
//! buildingRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 18
//!     local People greater 0
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//! end
//! \endcode
//==============================================================================
class IRule
{
public:

    //--------------------------------------------------------------------------
    //! \brief Value for Trace::blockingCommand when the Rule ran.
    //! Nothing blocked it.
    //--------------------------------------------------------------------------
    static constexpr size_t NO_BLOCKING_COMMAND = size_t(-1);

    //--------------------------------------------------------------------------
    //! \brief What happened during one attempt to run a Rule.
    //! The demo rule log uses this.
    //--------------------------------------------------------------------------
    struct Trace
    {
        //! \brief The Rule that was attempted. Never null.
        IRule const* rule = nullptr;
        //! \brief Where it was attempted.
        //! Valid only during the notification.
        //! It points at a live entity context. It moves on by the next tick.
        RuleContext const* context = nullptr;
        //! \brief Index of the command that failed, in getCommands().
        //! NO_BLOCKING_COMMAND when the Rule ran.
        //! Answers: "why does this Rule never do anything?"
        size_t blockingCommand = NO_BLOCKING_COMMAND;
        //! \brief True when every command passed and all ran. Declared last so
        //! that it sits in the gap the struct ends on rather than opening one.
        bool success = false;
    };

    //--------------------------------------------------------------------------
    //! \brief Watches every attempt to run a Rule.
    //! Used by the demo debugger. It turns traces into a filterable log.
    //!
    //! Only one listener at a time, shared globally.
    //! Rules are shared by every City.
    //! A pointer per Rule would waste memory when the feature is off.
    //! When no listener is set, the cost is one null pointer test per attempt.
    //--------------------------------------------------------------------------
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //----------------------------------------------------------------------
        //! \brief A Rule was attempted.
        //! \param[in] trace what happened. Do not keep the pointers inside.
        //----------------------------------------------------------------------
        virtual void onRuleExecuted(Trace const& trace) = 0;
    };

    //--------------------------------------------------------------------------
    //! \brief Set the Rule attempt listener.
    //! \param[in] listener the listener, or nullptr to remove it.
    //! Not owned. Must outlive the Simulation.
    //--------------------------------------------------------------------------
    static void setListener(Listener* listener)
    {
        s_listener = listener;
    }

    //--------------------------------------------------------------------------
    //! \return the current listener, or nullptr if none.
    //--------------------------------------------------------------------------
    [[nodiscard]] static Listener* getListener()
    {
        return s_listener;
    }

    //--------------------------------------------------------------------------
    //! \param[in] name Rule name from the script.
    //! \param[in] rate ticks between runs, or zero if the script used a duration.
    //! \param[in] rateMinutes game minutes between runs, or zero if the script
    //! counted ticks. See getPeriodTicks().
    //! \param[in] commands Rule body, copied.
    //! The commands are not owned here. The Ruleset owns them.
    //--------------------------------------------------------------------------
    IRule(Name const& name,
          uint32_t rate,
          uint32_t rateMinutes,
          std::vector<IRuleCommand*> const& commands)
        : m_type(name.str()),
          m_rate(rate),
          m_rateMinutes(rateMinutes),
          m_commands(commands)
    {
    }

    virtual ~IRule() = default;

    //--------------------------------------------------------------------------
    //! \brief Try to run the Rule.
    //! Ask every command. Run all only if every one passes.
    //!
    //! This makes a Rule atomic.
    //! "Remove 1 People and put them in a car" does both or neither.
    //!
    //! \note Commands run from last to first, in validate and execute.
    //! Independent commands are not affected.
    //! Commands that touch the same resource run in reverse script order.
    //!
    //! \param[in,out] context where to run.
    //! \return true when the Rule ran.
    //--------------------------------------------------------------------------
    virtual bool execute(RuleContext& context)
    {
        size_t i = m_commands.size();
        while (i--)
        {
            if (!m_commands[i]->validate(context))
            {
                notify(context, false, i);
                return false;
            }
        }

        i = m_commands.size();
        while (i--)
        {
            m_commands[i]->execute(context);
        }

        notify(context, true, NO_BLOCKING_COMMAND);
        return true;
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a Rule that was declared empty.
    //!
    //! The parser declares every Rule by name before it reads bodies.
    //! A Layer can list a Rule written later in the file.
    //! A Rule can use a fallback declared after it.
    //! Rules start empty and are filled on the second pass.
    //!
    //! \param[in] rate ticks between runs.
    //! \param[in] rateMinutes game minutes between runs.
    //! \param[in] commands Rule body.
    //--------------------------------------------------------------------------
    void reset(uint32_t rate,
               uint32_t rateMinutes,
               std::vector<IRuleCommand*> commands)
    {
        m_rate = rate;
        m_rateMinutes = rateMinutes;
        m_commands = std::move(commands);
    }

    //--------------------------------------------------------------------------
    //! \return the Rule name from the script.
    //! Shown in the inspector. Used by fallbacks.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::string const& getName() const
    {
        return m_type;
    }

    //--------------------------------------------------------------------------
    //! \return the period in ticks, as written in the script.
    //! Not used when the script gave a duration. Use getPeriodTicks().
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getRate() const
    {
        return m_rate;
    }

    //--------------------------------------------------------------------------
    //! \return the period in game minutes, as written in the script.
    //! Zero when the script counted ticks.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getRateMinutes() const
    {
        return m_rateMinutes;
    }

    //--------------------------------------------------------------------------
    //! \brief Ticks between two runs of the Rule.
    //!
    //! The script may write \c rate \c 7 (ticks)
    //! or \c rate \c 30 \c minutes (game time).
    //! Minutes are converted to ticks here, not at parse time.
    //! Changing TimeConfig::ticksPerMinute rescales the whole Ruleset.
    //!
    //! \param[in] ticksPerMinute ticks per game minute, from settings.
    //! Zero is treated as one.
    //! \return the period. Never zero. A Rule runs at least every tick.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getPeriodTicks(uint32_t ticksPerMinute) const
    {
        if (m_rateMinutes == 0u)
            return (m_rate == 0u) ? 1u : m_rate;

        uint32_t const perMinute = (ticksPerMinute == 0u) ? 1u : ticksPerMinute;
        return m_rateMinutes * perMinute;
    }

    //--------------------------------------------------------------------------
    //! \return the Rule body in script order.
    //! Not owned. The Ruleset owns the commands.
    //! Indexed by Trace::blockingCommand.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::vector<IRuleCommand*> const& getCommands() const
    {
        return m_commands;
    }

protected:

    //--------------------------------------------------------------------------
    //! \brief Tell the listener how the attempt went.
    //! One null pointer test when no listener is set.
    //! \param[in] context where the Rule was attempted.
    //! \param[in] success whether the Rule ran.
    //! \param[in] blockingCommand failed command index, or NO_BLOCKING_COMMAND.
    //--------------------------------------------------------------------------
    void notify(RuleContext const& context,
                bool success,
                size_t blockingCommand) const
    {
        if (s_listener == nullptr)
            return;

        Trace const trace{ this, &context, blockingCommand, success };
        s_listener->onRuleExecuted(trace);
    }

private:

    //! \brief The one Rule attempt listener, shared by every Rule in every City.
    //! Not owned.
    static Listener* s_listener;
    //! \brief Rule name from the script.
    std::string m_type;
    //! \brief Period in ticks when the script counted ticks.
    uint32_t m_rate = 1u;
    //! \brief Period in game minutes when the script used a duration.
    uint32_t m_rateMinutes = 0u;
    //! \brief Rule body. Not owned. The Ruleset owns the commands.
    std::vector<IRuleCommand*> m_commands;
};

//==============================================================================
//! \brief A Rule for a Layer. It runs over City cells.
//!
//! Examples: forest spread, pollution creep, land value from neighbours.
//! It runs once per cell, many times per tick.
//! \c randomTiles lets the Layer visit only part of its cells each run.
//! Over time the behaviour matches visiting all cells, at lower cost.
//!
//! Example script. The share is set on the command that reads the Layer:
//! \code
//! layerRule CreateGrass
//!     rate 20 minutes
//!     layer Water remove 10 randomTilesPercent 90
//!     layer Grass add 1
//! end
//! \endcode
//==============================================================================
class RuleLayer: public IRule
{
public:

    //--------------------------------------------------------------------------
    //! \param[in] type Rule as declared in the script. Read here, not kept.
    //--------------------------------------------------------------------------
    explicit RuleLayer(RuleLayerType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands),
          m_randomTilesPercent(std::min(100u, type.randomTilesPercent)),
          m_randomTiles(type.randomTiles)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a Rule declared empty on the second parser pass.
    //! \param[in] type Rule as declared in the script.
    //--------------------------------------------------------------------------
    void configureFrom(RuleLayerType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
        m_randomTiles = type.randomTiles;
        m_randomTilesPercent = std::min(100u, type.randomTilesPercent);
    }

    //--------------------------------------------------------------------------
    //! \return true when the Rule visits a random share of cells,
    //! not every cell.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isRandom() const
    {
        return m_randomTiles;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the share of \p value the Rule asked for.
    //! Used to pick how many cells the Layer visits.
    //! \param[in] value the total.
    //! \return that share, rounded down. Never above 100 percent.
    //--------------------------------------------------------------------------
    template <class T>
    T takePercent(T value) const
    {
        return value * T(m_randomTilesPercent) / T(100);
    }

private:

    //! \brief Share in percent, clamped to 100.
    uint32_t m_randomTilesPercent;
    //! \brief Visit a random share of cells instead of all cells. Declared last
    //! so that it sits in the gap the class ends on rather than opening one.
    bool m_randomTiles;
};

//==============================================================================
//! \brief A Rule for a Building. It runs every N ticks on the Building.
//!
//! Examples: a house sends People to work, a factory makes Goods, a shop takes
//! deliveries.
//! It adds a fallback: if this Rule does not run, try another Rule.
//! Example: "go to work, or stay home".
//!
//! Example script. StayAtHome is declared elsewhere:
//! \code
//! buildingRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 18
//!     local People greater 0
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//!     onFail StayAtHome
//! end
//! \endcode
//==============================================================================
class RuleBuilding: public IRule
{
public:

    //--------------------------------------------------------------------------
    //! \param[in] type Rule as declared in the script.
    //! Read here, not kept, except the fallback pointer.
    //--------------------------------------------------------------------------
    explicit RuleBuilding(RuleBuildingType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands),
          m_onFail(type.onFail)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a Rule declared empty on the second parser pass.
    //! \param[in] type Rule as declared in the script.
    //--------------------------------------------------------------------------
    void configureFrom(RuleBuildingType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
        m_onFail = type.onFail;
    }

    //--------------------------------------------------------------------------
    //! \return the fallback Rule when this one does not run, or nullptr.
    //! Not owned. The Ruleset owns it.
    //--------------------------------------------------------------------------
    [[nodiscard]] RuleBuilding* getOnFail() const
    {
        return m_onFail;
    }

    //--------------------------------------------------------------------------
    //! \brief Try to run the Rule, then its fallback if needed.
    //!
    //! \note Fallback chains run until one Rule succeeds.
    //! Two Rules that fallback on each other loop forever.
    //! The parser does not check for that.
    //!
    //! \param[in,out] context where to run.
    //! \return true when this Rule or a fallback ran.
    //--------------------------------------------------------------------------
    bool execute(RuleContext& context) override
    {
        if (IRule::execute(context))
        {
            return true;
        }
        else
        {
            if (m_onFail != nullptr)
                return m_onFail->execute(context);
            else
                return false;
        }
    }

private:

    //! \brief Fallback Rule when this one does not run, or nullptr. Not owned.
    RuleBuilding* m_onFail;
};

//==============================================================================
//! \brief A Rule for a Zone. It spawns and destroys Buildings in the Zone.
//!
//! It turns a painted rectangle into a neighbourhood.
//! Unlike Building Rules, these run once for the whole Zone, not per cell.
//! Their commands count and create Buildings instead of reading resources.
//!
//! Example script:
//! \code
//! zoneRule GrowHomes
//!     rate 4 hours
//!     count Home less 10
//!     spawn Home at nearestSegment
//! end
//! \endcode
//==============================================================================
class RuleZone: public IRule
{
public:

    //--------------------------------------------------------------------------
    //! \param[in] type Rule as declared in the script. Read here, not kept.
    //--------------------------------------------------------------------------
    explicit RuleZone(RuleZoneType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a Rule declared empty on the second parser pass.
    //! \param[in] type Rule as declared in the script.
    //--------------------------------------------------------------------------
    void configureFrom(RuleZoneType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
    }
};

} // namespace ogb

#endif
