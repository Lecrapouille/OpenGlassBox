//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RuleCommand.hpp
//! \brief Commands executed by rules: add, remove, spawn, test, upgrade and
//! more.

#ifndef OPEN_GLASSBOX_RULE_COMMAND_HPP
#define OPEN_GLASSBOX_RULE_COMMAND_HPP

#include "OpenGlassBox/Rule.hpp"

namespace ogb
{

//==============================================================================
//! \brief
//!
//! Example:
//! \code
//! layer Grass add 1
//! \endcode
//==============================================================================
class RuleCommandAdd: public IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    RuleCommandAdd(IRuleValue& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource has not reached the
    //! capacity.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Increase the amount of resource of the target.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

private:

    IRuleValue& m_target;
    uint32_t m_amount;
};

//==============================================================================
//! \brief
//!
//! Example:
//! \code
//! local People remove 1
//! \endcode
//==============================================================================
class RuleCommandRemove: public IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    RuleCommandRemove(IRuleValue& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource is enough.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Decrease the amount of resource of the target.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

private:

    IRuleValue& m_target;
    uint32_t m_amount;
};

//==============================================================================
//! \brief
//!
//! Example:
//! \code
//! layer Water greater 300
//! \endcode
//==============================================================================
class RuleCommandTest: public IRuleCommand
{
public:

    enum class Comparison
    {
        EQUALS,
        GREATER,
        LESS
    };

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    RuleCommandTest(IRuleValue& target, Comparison comparison, uint32_t amount)
        : m_target(target), m_amount(amount), m_comparison(comparison)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource is enough.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Decrease the amount of resource of the target.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

private:

    IRuleValue& m_target;
    uint32_t m_amount;
    Comparison m_comparison;
};

//==============================================================================
//! \brief Send an Agent out of the current Unit, carrying a load, towards any
//! Unit that accepts it.
//!
//! Example:
//! \code
//! agent Worker to Work add [ People 1 ]
//! \endcode
//==============================================================================
class RuleCommandAgent: public IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief \param[in] type the kind of agent to send, copied.
    //! \param[in] target what the agent looks for, such as "Work".
    //! \param[in] resources the load it carries, copied.
    //--------------------------------------------------------------------------
    RuleCommandAgent(AgentType const& type,
                     Name const& target,
                     Resources const& resources)
        : m_type(type), m_target(target), m_resources(resources)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

    //--------------------------------------------------------------------------
    //! \brief \return the kind of agent the command sends out.
    //--------------------------------------------------------------------------
    [[nodiscard]] AgentType const& getAgentType() const
    {
        return m_type;
    }

    //--------------------------------------------------------------------------
    //! \brief \return what the agent looks for, such as "Work".
    //--------------------------------------------------------------------------
    [[nodiscard]] Name const& getTarget() const
    {
        return m_target;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the load the agent carries.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resources const& getResources() const
    {
        return m_resources;
    }

private:

    //! \brief The kind of agent to send out.
    AgentType m_type;
    //! \brief What the agent looks for.
    Name m_target;
    //! \brief The load it carries.
    Resources m_resources;
};

//==============================================================================
//! \brief Predicate on the calendar: the rule only fires inside a window of
//! the day, which is what gives a city a working day.
//!
//! The window is half open, [from, to), and wraps around midnight when \c from
//! is the greater of the two, so \c hour \c between \c 22 \c 6 means night.
//!
//! Example:
//! \code
//! hour between 8 18
//! \endcode
//==============================================================================
class RuleCommandHour: public IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief \param[in] from first hour of the window, in [0..23].
    //! \param[in] to first hour past the window, in [0..23]. Equal to \c from
    //! means a window of no hour at all, and the rule never fires.
    //--------------------------------------------------------------------------
    RuleCommandHour(uint32_t from, uint32_t to) : m_from(from), m_to(to) {}

    //--------------------------------------------------------------------------
    //! \brief \return true while the clock of the context is inside the window.
    //! False when the context carries no clock: a rule that asks for the time
    //! cannot fire without one.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Does nothing: reading the clock changes nothing.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief \return the window in words, for the rule log of the demo.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

    //--------------------------------------------------------------------------
    //! \brief First hour of the window. Read by OpeningHours to tell whether a
    //! building is open, without parsing the script again.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getFrom() const
    {
        return m_from;
    }

    //--------------------------------------------------------------------------
    //! \brief First hour past the window. See getFrom().
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTo() const
    {
        return m_to;
    }

private:

    //! \brief First hour of the window, in [0..23].
    uint32_t m_from;

    //! \brief First hour past the window, in [0..23].
    uint32_t m_to;
};

//==============================================================================
//! \brief Predicate on how many Units of a type sit inside the current Zone.
//!
//! Example:
//! \code
//! count Home less 12
//! \endcode
//==============================================================================
class RuleCommandCount: public IRuleCommand
{
public:

    RuleCommandCount(Name const& unitType,
                     RuleCommandTest::Comparison comparison,
                     uint32_t amount)
        : m_unitType(unitType), m_comparison(comparison), m_amount(amount)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    Name m_unitType;
    RuleCommandTest::Comparison m_comparison;
    uint32_t m_amount;
};

//==============================================================================
//! \brief Create a Unit of the given type inside the current Zone.
//!
//! Example:
//! \code
//! spawn Home at nearestSegment
//! spawn Home at freeCell
//! \endcode
//==============================================================================
class RuleCommandSpawn: public IRuleCommand
{
public:

    enum class Placement
    {
        NearestSegment,
        FreeCell
    };

    RuleCommandSpawn(UnitType const& unitType, Placement placement)
        : m_unitType(unitType), m_placement(placement)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    UnitType const& m_unitType;
    Placement m_placement;
};

//==============================================================================
//! \brief Replace a Unit of one type by a Unit of another type, keeping its
//! position and its attachment to the road.
//!
//! Example:
//! \code
//! upgrade Home to Shop
//! \endcode
//==============================================================================
class RuleCommandUpgrade: public IRuleCommand
{
public:

    RuleCommandUpgrade(UnitType const& fromType, UnitType const& toType)
        : m_fromType(fromType), m_toType(toType)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    UnitType const& m_fromType;
    UnitType const& m_toType;
};

//==============================================================================
//! \brief Destroy one Unit of the given type inside the current Zone.
//!
//! Example:
//! \code
//! destroy Home
//! \endcode
//==============================================================================
class RuleCommandDestroy: public IRuleCommand
{
public:

    explicit RuleCommandDestroy(Name const& unitType) : m_unitType(unitType) {}

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    Name m_unitType;
};

} // namespace ogb

#endif
