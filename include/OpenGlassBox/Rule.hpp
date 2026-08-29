//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Rule.hpp
//! \brief Rule base classes and layer, unit and zone rule implementations.

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
class Unit;
class Layer;
class Zone;
class Resources;

//==============================================================================
//! \brief Everything a rule is allowed to look at while it runs: where it is
//! running, on whose behalf, and what it may read and write.
//!
//! A rule holds no state of its own and is shared by every entity of its kind,
//! so all of the "where" has to be handed to it. Which pointers are set says
//! what kind of rule is running: a rule of a building fills in \c unit, a rule
//! of a layer fills in \c layer, a rule of a zone fills in \c zone. \c city is
//! always set.
//!
//! The context is held by the entity running the rule and reused from tick to
//! tick, since a rule of a layer fires over thousands of cells and building one
//! per cell would cost more than the rule itself.
//==============================================================================
struct RuleContext
{
    //! \brief The city the rule is running on behalf of. Never null while a
    //! rule is running.
    City* city = nullptr;
    //! \brief The building the rule belongs to, or null when it does not belong
    //! to one.
    Unit* unit = nullptr;
    //! \brief The layer the rule belongs to, or null. Only used to tell the
    //! debugger which entity a trace came from: the commands reach the layer
    //! through the city.
    Layer* layer = nullptr;
    //! \brief The zone the rule belongs to, or null.
    Zone* zone = nullptr;
    //! \brief The calendar. Never null while a rule is running, since
    //! \c hour \c between has nothing to read otherwise, and a rule asking for
    //! the time without one simply never fires.
    SimulationClock const* clock = nullptr;
    //! \brief What \c local reads and writes: the resources of the building or
    //! of the cell the rule is running on.
    Resources* locals = nullptr;
    //! \brief What \c global reads and writes: the resources of the city as a
    //! whole, such as money.
    Resources* globals = nullptr;
    //! \brief The cell the rule acts on. Its coordinates are signed because
    //! the grid is unbounded in the four directions.
    Cell cell;
    //! \brief How far around that cell a command reaching into a layer may
    //! reach, as a taxicab distance. Comes from the footprint of the building.
    uint32_t radius = 0u;
};

//==============================================================================
//! \brief One line inside a rule, as the script wrote it.
//!
//! Everything a rule does is a command: testing a resource, testing the hour,
//! adding to a layer, sending an agent out, putting a building up. A command is
//! asked twice, and the split is what makes a rule all-or-nothing: first
//! whether it could run, then to actually run.
//!
//! Commands are created by the parser, owned by the ruleset, and shared by
//! every entity running the rule. They therefore hold no state of their own:
//! what they need comes from the RuleContext.
//==============================================================================
class IRuleCommand
{
public:

    virtual ~IRuleCommand() = default;

    //--------------------------------------------------------------------------
    //! \brief Could this command run right now?
    //!
    //! Asked for every command of the rule before any of them is applied, so a
    //! command must not change anything here.
    //!
    //! \param[in,out] context where the rule is running.
    //! \return false to stop the whole rule, which then fires nothing at all.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief Do it. Only called once every command of the rule has agreed, so
    //! it may assume its preconditions hold.
    //! \param[in,out] context where the rule is running.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief \return the command in words, close to what the script wrote, for
    //! the rule log and the inspector of the demo. Built on demand rather than
    //! stored, so it is not free.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual std::string getDescription() const = 0;
};

//==============================================================================
//! \brief Somewhere a command may read a number from and write one back to.
//!
//! A script writes \c local \c People, \c global \c Money or \c layer \c Water,
//! and each of those becomes one of these. Having them behind one interface is
//! what lets a single command such as \c add serve all three: the command knows
//! how much, the value knows where.
//!
//! See RuleValue.hpp for the three implementations.
//==============================================================================
class IRuleValue
{
public:

    virtual ~IRuleValue() = default;

    //--------------------------------------------------------------------------
    //! \brief \param[in,out] context where the rule is running.
    //! \return how much there is right now. For a layer read over a footprint,
    //! the sum over the cells covered.
    //--------------------------------------------------------------------------
    virtual uint32_t get(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief \param[in,out] context where the rule is running.
    //! \return how much there could be at most, which is what a test against a
    //! proportion needs. For a layer, the cap of a cell times the number of
    //! cells covered.
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual uint32_t getCapacity(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief Add, up to the capacity. What overflows is dropped.
    //! \param[in,out] context where the rule is running.
    //! \param[in] toAdd how much to add.
    //--------------------------------------------------------------------------
    virtual void add(RuleContext& context, uint32_t toAdd) = 0;

    //--------------------------------------------------------------------------
    //! \brief Take away, down to zero. What is missing is not taken.
    //! \param[in,out] context where the rule is running.
    //! \param[in] toRemove how much to take.
    //--------------------------------------------------------------------------
    virtual void remove(RuleContext& context, uint32_t toRemove) = 0;

    //--------------------------------------------------------------------------
    //! \brief \return the name of the kind of resource, such as "People".
    //--------------------------------------------------------------------------
    [[nodiscard]] virtual Name const& getTypeName() const = 0;
};

//==============================================================================
//! \brief A rule: a name, how often it fires, and the commands it is made of.
//!
//! This is the whole of the scripting language at runtime. An entity runs its
//! rules once every so many ticks; a rule asks each of its commands whether it
//! could run, and only if every one of them agrees does it apply them all. A
//! rule therefore never half fires, which is what lets a script say "take a
//! person out of the house and put them in a car" without ever losing one.
//!
//! Rules are owned by the ruleset and shared by every entity of the kind that
//! lists them: a thousand houses run the same RuleUnit. That is why they hold
//! no state and are handed a RuleContext instead.
//!
//! Example:
//! \code
//! // Rules are declared by the script rather than by hand, but running one is
//! // no more than this.
//! RuleContext context;
//! context.city = &city;
//! context.unit = &home;
//! context.clock = &simulation.getClock();
//! context.locals = &home.getResources();
//! context.globals = &city.getGlobals();
//!
//! for (RuleUnit* rule : home.getRules())
//! {
//!     // A script may name a rule that does not exist, hence the test.
//!     if ((rule != nullptr) &&
//!         (home.getTicks() % rule->getPeriodTicks(ticksPerMinute) == 0u))
//!     {
//!         rule->execute(context);
//!     }
//! }
//! \endcode
//!
//! The matching script, where every line of the body is one command:
//! \code
//! unitRule SendPeopleToWork
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
    //! \brief What Trace::blockingCommand holds when the rule fired: nothing
    //! blocked it.
    //--------------------------------------------------------------------------
    static constexpr size_t NO_BLOCKING_COMMAND = size_t(-1);

    //--------------------------------------------------------------------------
    //! \brief What happened during a single attempt to run a rule, which is
    //! what the rule log of the demo is made of.
    //--------------------------------------------------------------------------
    struct Trace
    {
        //! \brief The rule that was attempted. Never null.
        IRule const* rule = nullptr;
        //! \brief Where it was attempted. Valid for the duration of the
        //! notification only: it is the working context of a live entity, and
        //! it will have moved on by the next tick.
        RuleContext const* context = nullptr;
        //! \brief True when every command agreed and all of them were applied.
        bool success = false;
        //! \brief Which command refused, as an index into getCommands(), or
        //! NO_BLOCKING_COMMAND when the rule fired. This is the answer to "why
        //! does this rule, which looks right, never do anything?"
        size_t blockingCommand = NO_BLOCKING_COMMAND;
    };

    //--------------------------------------------------------------------------
    //! \brief Observer of every attempt to run a rule, anywhere. Meant for the
    //! debugger of the demo, which turns the traces into a filterable log.
    //!
    //! One observer at a time, and a global one: rules are shared by every
    //! city, so a pointer per rule would cost memory for something that is off
    //! most of the time. While none is installed, nothing is built and no
    //! virtual call is made, so the cost of the feature when unused is one null
    //! pointer test per attempt.
    //--------------------------------------------------------------------------
    class Listener
    {
    public:

        virtual ~Listener() = default;

        //----------------------------------------------------------------------
        //! \brief A rule was attempted.
        //! \param[in] trace what happened. Do not keep the pointers it holds.
        //----------------------------------------------------------------------
        virtual void onRuleExecuted(Trace const& trace) = 0;
    };

    //--------------------------------------------------------------------------
    //! \brief Install the observer of rule attempts.
    //! \param[in] listener the observer, or nullptr to detach. Not owned, and
    //! has to outlive the simulation.
    //--------------------------------------------------------------------------
    static void setListener(Listener* listener)
    {
        s_listener = listener;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the observer in place, or nullptr when nobody is
    //! watching.
    //--------------------------------------------------------------------------
    [[nodiscard]] static Listener* getListener()
    {
        return s_listener;
    }

    //--------------------------------------------------------------------------
    //! \brief \param[in] name name of the rule, as the script wrote it.
    //! \param[in] rate how many ticks between two runs, or zero when the script
    //! gave a duration instead.
    //! \param[in] rateMinutes how many game minutes between two runs, or zero
    //! when the script counted ticks. See getPeriodTicks().
    //! \param[in] commands the body of the rule, copied. The commands
    //! themselves are not owned: the ruleset owns them.
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
    //! \brief Attempt the rule: ask every command, and apply them all only if
    //! every one of them agreed.
    //!
    //! That is what makes a rule atomic. A rule saying "take a person out of
    //! the house and put them in a car" either does both or does neither, so
    //! nobody is ever lost between the two.
    //!
    //! \note The commands are walked from the last to the first, both when
    //! asking and when applying. It makes no difference to a rule whose
    //! commands are independent, which is what a ruleset normally writes, but a
    //! rule whose commands touch the same resource sees them in the reverse of
    //! the order the script wrote.
    //!
    //! \param[in,out] context where to run it.
    //! \return true when the rule fired.
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
    //! \brief Fill in the body of a rule that was declared empty.
    //!
    //! The parser declares every rule by name before it reads any of their
    //! bodies, so that a layer may list a rule written further down the file,
    //! or a rule may fall back on one declared after it. A rule is therefore
    //! born empty and filled in on the second pass.
    //!
    //! \param[in] rate how many ticks between two runs.
    //! \param[in] rateMinutes how many game minutes between two runs.
    //! \param[in] commands the body of the rule.
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
    //! \brief \return the name of the rule, as the script wrote it. What the
    //! inspector shows and what a fallback refers to.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::string const& getName() const
    {
        return m_type;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the period as the script wrote it, in ticks. Meaningless
    //! when the script gave a duration instead: ask getPeriodTicks().
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getRate() const
    {
        return m_rate;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the period as the script wrote it, in game minutes, or
    //! zero when the script counted ticks.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getRateMinutes() const
    {
        return m_rateMinutes;
    }

    //--------------------------------------------------------------------------
    //! \brief How many ticks between two runs of the rule.
    //!
    //! A script may write \c rate \c 7, which counts ticks, or
    //! \c rate \c 30 \c minutes, which counts game time and is the form a
    //! reader can reason about. The second is turned into ticks here rather
    //! than while parsing, so that changing TimeConfig::ticksPerMinute
    //! rescales the whole ruleset instead of leaving it behind.
    //!
    //! \param[in] ticksPerMinute how many ticks make a game minute, from the
    //! settings. Zero is read as one.
    //! \return the period, never zero: a rule fires at least every tick.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getPeriodTicks(uint32_t ticksPerMinute) const
    {
        if (m_rateMinutes == 0u)
            return (m_rate == 0u) ? 1u : m_rate;

        uint32_t const perMinute = (ticksPerMinute == 0u) ? 1u : ticksPerMinute;
        return m_rateMinutes * perMinute;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the body of the rule, in the order the script wrote it.
    //! Not owned: the ruleset owns the commands. Indexed by
    //! Trace::blockingCommand.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::vector<IRuleCommand*> const& getCommands() const
    {
        return m_commands;
    }

protected:

    //--------------------------------------------------------------------------
    //! \brief Tell the observer how an attempt went. A null pointer test when
    //! nobody is watching, which is the usual case.
    //! \param[in] context where the rule was attempted.
    //! \param[in] success whether it fired.
    //! \param[in] blockingCommand which command refused, or
    //! NO_BLOCKING_COMMAND.
    //--------------------------------------------------------------------------
    void notify(RuleContext const& context,
                bool success,
                size_t blockingCommand) const
    {
        if (s_listener == nullptr)
            return;

        Trace const trace{ this, &context, success, blockingCommand };
        s_listener->onRuleExecuted(trace);
    }

private:

    //! \brief The one observer of rule attempts, shared by every rule of every
    //! city. Not owned.
    static Listener* s_listener;
    //! \brief Name of the rule, as the script wrote it.
    std::string m_type;
    //! \brief Period in ticks, when the script counted ticks.
    uint32_t m_rate = 1u;
    //! \brief Period in game minutes, when the script gave a duration.
    uint32_t m_rateMinutes = 0u;
    //! \brief The body of the rule. Not owned: the ruleset owns the commands.
    std::vector<IRuleCommand*> m_commands;
};

//==============================================================================
//! \brief A rule belonging to a layer, run over the cells of a city.
//!
//! What makes a forest spread, pollution creep, or land value follow its
//! neighbours. Such a rule fires once per cell, which is a great many times per
//! tick, and that is what \c randomTiles is for: it lets the layer visit only a
//! share of its cells on each run and still reach the same behaviour on
//! average, at a fraction of the cost.
//!
//! Example script, where the share is asked for on the command that reads the
//! layer:
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
    //! \brief \param[in] type the rule as the script declared it. Read here and
    //! not kept.
    //--------------------------------------------------------------------------
    explicit RuleLayer(RuleLayerType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands),
          m_randomTiles(type.randomTiles),
          m_randomTilesPercent(std::min(100u, type.randomTilesPercent))
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a rule that was declared empty on the second parser pass.
    //! \param[in] type the rule as the script declared it.
    //--------------------------------------------------------------------------
    void configureFrom(RuleLayerType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
        m_randomTiles = type.randomTiles;
        m_randomTilesPercent = std::min(100u, type.randomTilesPercent);
    }

    //--------------------------------------------------------------------------
    //! \brief \return true when the rule visits a random share of the cells
    //! rather than every one of them.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isRandom() const
    {
        return m_randomTiles;
    }

    //--------------------------------------------------------------------------
    //! \brief Take the share of a number the rule asked for, which is how many
    //! cells out of a total the layer will visit.
    //! \param[in] value the total.
    //! \return that share of it, rounded down. The share never exceeds a
    //! hundred percent.
    //--------------------------------------------------------------------------
    template <class T>
    T takePercent(T value) const
    {
        return value * T(m_randomTilesPercent) / T(100);
    }

private:

    //! \brief Whether to visit a random share of the cells rather than all.
    bool m_randomTiles;
    //! \brief Which share, in percent, clamped to a hundred.
    uint32_t m_randomTilesPercent;
};

//==============================================================================
//! \brief A rule belonging to a building, run once every so many ticks.
//!
//! What makes a house send people to work, a factory turn goods out, a shop
//! take deliveries. The one thing it adds to a plain rule is a fallback: a rule
//! that did not fire may name another to try instead, which is how a script
//! says "go to work, or else stay at home and be bored".
//!
//! Example script, where StayAtHome is a rule declared elsewhere:
//! \code
//! unitRule SendPeopleToWork
//!     rate 45 minutes
//!     hour between 8 18
//!     local People greater 0
//!     local People remove 1
//!     agent Worker to Work add [ People 1 ]
//!     onFail StayAtHome
//! end
//! \endcode
//==============================================================================
class RuleUnit: public IRule
{
public:

    //--------------------------------------------------------------------------
    //! \brief \param[in] type the rule as the script declared it. Read here and
    //! not kept, except for the fallback, which is kept by address.
    //--------------------------------------------------------------------------
    explicit RuleUnit(RuleUnitType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands),
          m_onFail(type.onFail)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a rule that was declared empty on the second parser pass.
    //! \param[in] type the rule as the script declared it.
    //--------------------------------------------------------------------------
    void configureFrom(RuleUnitType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
        m_onFail = type.onFail;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the rule tried instead when this one does not fire, or
    //! nullptr when there is none. Not owned: the ruleset owns it.
    //--------------------------------------------------------------------------
    [[nodiscard]] RuleUnit* getOnFail() const
    {
        return m_onFail;
    }

    //--------------------------------------------------------------------------
    //! \brief Attempt the rule and, failing that, its fallback.
    //!
    //! \note A chain of fallbacks is followed as far as it goes, and a script
    //! that makes two rules fall back on each other loops for ever. The parser
    //! does not check for that.
    //!
    //! \param[in,out] context where to run it.
    //! \return true when this rule or one of its fallbacks fired.
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

    //! \brief The rule tried instead when this one does not fire, or nullptr.
    //! Not owned.
    RuleUnit* m_onFail;
};

//==============================================================================
//! \brief A rule belonging to a zone, which puts buildings up and pulls them
//! down inside its footprint.
//!
//! What turns a painted rectangle into a neighbourhood. Unlike the rules of a
//! building, these run once for the whole zone rather than once per cell, and
//! their commands count and create rather than read and write resources.
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
    //! \brief \param[in] type the rule as the script declared it. Read here and
    //! not kept.
    //--------------------------------------------------------------------------
    explicit RuleZone(RuleZoneType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Fill in a rule that was declared empty on the second parser pass.
    //! \param[in] type the rule as the script declared it.
    //--------------------------------------------------------------------------
    void configureFrom(RuleZoneType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
    }
};

} // namespace ogb

#endif
