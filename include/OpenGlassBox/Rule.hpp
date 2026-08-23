//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Rule.hpp
//! \brief Rule base classes and map, unit and area rule implementations.


#ifndef OPEN_GLASSBOX_RULE_HPP
#  define OPEN_GLASSBOX_RULE_HPP

#  include "OpenGlassBox/Types.hpp"
#  include "OpenGlassBox/SimulationClock.hpp"
#  include <cstdlib>

namespace ogb {

class City;
class Unit;
class Map;
class Area;
class Resources;

//==============================================================================
//! \brief Structure holding all information needed to execute simulation rules.
//==============================================================================
struct RuleContext
{
    //! \brief Non null pointer on the City.
    City* city = nullptr;
    //! \brief Non null pointer on the Unit when the rule runs on a Unit.
    Unit* unit = nullptr;
    //! \brief Non null pointer on the Map when the rule runs on a Map. Only
    //! used to tell the debugger which entity a trace belongs to.
    Map* map = nullptr;
    //! \brief Non null pointer on the Area when the rule runs on an Area.
    Area* area = nullptr;
    //! \brief Calendar of the simulation. Never null when a rule is actually
    //! running: the hour-between command reads it.
    SimulationClock const* clock = nullptr;
    //! \brief Local resources (of Map or Unit).
    Resources* locals = nullptr;
    //! \brief Global resources.
    Resources* globals = nullptr;
    //! \brief Cell of the world grid the rule acts on. Signed because the grid
    //! is unbounded in the four directions.
    int32_t u = 0, v = 0;
    //! \brief Radius action on Map resources.
    uint32_t radius = 0u;
};

//==============================================================================
//! \brief Base class interfacing command defined from simulation scripts
//==============================================================================
class IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief Needed because of virtual methods
    //--------------------------------------------------------------------------
    virtual ~IRuleCommand() = default;

    //--------------------------------------------------------------------------
    //! \brief Return true if this command can be applied in the current context.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief Apply the command on the current context.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) = 0;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    virtual std::string type() = 0;
};

//==============================================================================
//! \brief
//==============================================================================
class IRuleValue
{
public:

    //--------------------------------------------------------------------------
    //! \brief Needed because of virtual methods
    //--------------------------------------------------------------------------
    virtual ~IRuleValue() = default;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual uint32_t get(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual uint32_t capacity(RuleContext& context) = 0;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void add(RuleContext& context, uint32_t toAdd) = 0;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void remove(RuleContext& context, uint32_t toRemove) = 0;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    virtual std::string const& type() const = 0;
};

//==============================================================================
//! \brief
//==============================================================================
class IRule
{
public:

    //--------------------------------------------------------------------------
    //! \brief Value of Trace::blockingCommand when the rule succeeded.
    //--------------------------------------------------------------------------
    static constexpr size_t NO_BLOCKING_COMMAND = size_t(-1);

    //--------------------------------------------------------------------------
    //! \brief What happened during a single attempt to run a rule.
    //--------------------------------------------------------------------------
    struct Trace
    {
        //! \brief The rule that was attempted.
        IRule const* rule = nullptr;
        //! \brief The context it was attempted in. Only valid for the duration
        //! of the notification.
        RuleContext const* context = nullptr;
        //! \brief Whether every command validated and was applied.
        bool success = false;
        //! \brief Index in rule->commands() of the command that refused to
        //! validate, or NO_BLOCKING_COMMAND on success. This is what tells why
        //! a rule that looks correct never fires.
        size_t blockingCommand = NO_BLOCKING_COMMAND;
    };

    //--------------------------------------------------------------------------
    //! \brief Observer of every rule execution. Meant for the debugger of the
    //! demo, which turns the traces into a filterable log.
    //!
    //! Only one listener at a time, installed globally: rules are shared by
    //! every City and holding a pointer per rule would cost memory for a
    //! feature that is off in a release build. Nothing is built and no virtual
    //! call is made while no listener is installed.
    //--------------------------------------------------------------------------
    class Listener
    {
    public:

        virtual ~Listener() = default;
        virtual void onRuleExecuted(Trace const& trace) = 0;
    };

    //--------------------------------------------------------------------------
    //! \brief Install the observer of rule executions. Pass nullptr to detach.
    //--------------------------------------------------------------------------
    static void setListener(Listener* listener) { s_listener = listener; }

    //--------------------------------------------------------------------------
    //! \brief The currently installed observer, or nullptr.
    //--------------------------------------------------------------------------
    static Listener* listener() { return s_listener; }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    IRule(std::string const& name, uint32_t rate, uint32_t rateMinutes,
          std::vector<IRuleCommand*> const& commands)
        : m_type(name), m_rate(rate), m_rateMinutes(rateMinutes),
          m_commands(commands)
    {}

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual ~IRule() = default;

    //--------------------------------------------------------------------------
    //! \brief Validate every command then, and only then, apply them all, so
    //! that a rule is either fully applied or not applied at all.
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
    //! \brief Set the body of the rule after construction.
    //!
    //! The parser declares every rule by name before it parses any of them, so
    //! that a map may list a rule written further down the file. A rule is
    //! therefore born empty and filled in on the second pass.
    //--------------------------------------------------------------------------
    void reset(uint32_t rate, uint32_t rateMinutes,
               std::vector<IRuleCommand*> commands)
    {
        m_rate = rate;
        m_rateMinutes = rateMinutes;
        m_commands = std::move(commands);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    std::string const& type() const { return m_type; }

    //--------------------------------------------------------------------------
    //! \brief Period of the rule as the script wrote it, in ticks. Meaningless
    //! when the script gave a duration: ask periodTicks() instead.
    //--------------------------------------------------------------------------
    uint32_t rate() const { return m_rate; }

    //--------------------------------------------------------------------------
    //! \brief Period of the rule as a duration of game time, in minutes. Zero
    //! when the script counted ticks.
    //--------------------------------------------------------------------------
    uint32_t rateMinutes() const { return m_rateMinutes; }

    //--------------------------------------------------------------------------
    //! \brief How many ticks separate two runs of the rule.
    //!
    //! A script may write "rate 7", which counts ticks, or "rate 30 minutes",
    //! which counts game time and is what a reader can reason about. The second
    //! form is turned into ticks here rather than at parsing time, so that
    //! changing SimulationConfig::ticksPerMinute rescales the whole ruleset
    //! instead of leaving it behind.
    //--------------------------------------------------------------------------
    uint32_t periodTicks(uint32_t ticksPerMinute) const
    {
        if (m_rateMinutes == 0u)
            return (m_rate == 0u) ? 1u : m_rate;

        uint32_t const perMinute = (ticksPerMinute == 0u) ? 1u : ticksPerMinute;
        return m_rateMinutes * perMinute;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    std::vector<IRuleCommand*> const& commands() const { return m_commands; }

protected:

    //--------------------------------------------------------------------------
    //! \brief Report the outcome of an attempt to the installed listener. Boils
    //! down to a null pointer test when the debugger is not attached.
    //--------------------------------------------------------------------------
    void notify(RuleContext const& context, bool success, size_t blockingCommand) const
    {
        if (s_listener == nullptr)
            return;

        Trace const trace { this, &context, success, blockingCommand };
        s_listener->onRuleExecuted(trace);
    }

private:

    static Listener*           s_listener;

    std::string                m_type;
    uint32_t                   m_rate = 1u;
    uint32_t                   m_rateMinutes = 0u;
    std::vector<IRuleCommand*> m_commands;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleMap: public IRule
{
public:

    RuleMap(RuleMapType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands),
          m_randomTiles(type.randomTiles),
          m_randomTilesPercent(std::min(100u, type.randomTilesPercent))
    {}

    //--------------------------------------------------------------------------
    //! \brief Fill in a rule that was declared empty. See IRule::reset.
    //--------------------------------------------------------------------------
    void reset(RuleMapType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
        m_randomTiles = type.randomTiles;
        m_randomTilesPercent = std::min(100u, type.randomTilesPercent);
    }

    //--------------------------------------------------------------------------
    //! \brief Use randomized values ?
    //--------------------------------------------------------------------------
    bool isRandom() const
    {
        return m_randomTiles;
    }

    //--------------------------------------------------------------------------
    //! \brief Compute the percent of the given value
    //--------------------------------------------------------------------------
    template<class T>
    T percent(T value) const
    {
        return value * T(m_randomTilesPercent) / T(100);
    }

private:

    bool m_randomTiles;
    uint32_t m_randomTilesPercent;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleUnit: public IRule
{
public:

    RuleUnit(RuleUnitType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands),
          m_onFail(type.onFail)
    {}

    //--------------------------------------------------------------------------
    //! \brief Fill in a rule that was declared empty. See IRule::reset.
    //--------------------------------------------------------------------------
    void reset(RuleUnitType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
        m_onFail = type.onFail;
    }

    //--------------------------------------------------------------------------
    //! \brief The rule run instead when this one does not fire, or nullptr.
    //--------------------------------------------------------------------------
    RuleUnit* onFail() const { return m_onFail; }

    virtual bool execute(RuleContext& context) override
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

    RuleUnit* m_onFail;
};

//==============================================================================
//! \brief Rule run by an Area: spawn, upgrade or destroy Units inside its
//! footprint.
//==============================================================================
class RuleArea: public IRule
{
public:

    RuleArea(RuleAreaType const& type)
        : IRule(type.name, type.rate, type.rateMinutes, type.commands)
    {}

    void reset(RuleAreaType const& type)
    {
        IRule::reset(type.rate, type.rateMinutes, type.commands);
    }
};

} // namespace ogb

#endif
