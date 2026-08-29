//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RuleCommand.hpp
//! \brief Commands that Rules run: add, remove, spawn, test, upgrade and more.

#ifndef OPEN_GLASSBOX_RULE_COMMAND_HPP
#define OPEN_GLASSBOX_RULE_COMMAND_HPP

#include "OpenGlassBox/Rule.hpp"

namespace ogb
{

//==============================================================================
//! \brief Add an amount to a resource.
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
    //! \param[in] target where to add.
    //! \param[in] amount how much to add.
    //--------------------------------------------------------------------------
    RuleCommandAdd(IRuleValue& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Can run if the resource is below capacity.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Add the amount to the target.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief Return text for the rule log.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

private:

    IRuleValue& m_target;
    uint32_t m_amount;
};

//==============================================================================
//! \brief Remove an amount from a resource.
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
    //! \param[in] target where to remove.
    //! \param[in] amount how much to remove.
    //--------------------------------------------------------------------------
    RuleCommandRemove(IRuleValue& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Can run if enough resource is present.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Remove the amount from the target.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief Return text for the rule log.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

private:

    IRuleValue& m_target;
    uint32_t m_amount;
};

//==============================================================================
//! \brief Test a resource against a value.
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
    //! \param[in] target resource to test.
    //! \param[in] comparison equals, greater, or less.
    //! \param[in] amount value to compare against.
    //--------------------------------------------------------------------------
    RuleCommandTest(IRuleValue& target, Comparison comparison, uint32_t amount)
        : m_target(target), m_amount(amount), m_comparison(comparison)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Can run if the comparison is true.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Does nothing. A test only checks a condition.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief Return text for the rule log.
    // -------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

private:

    IRuleValue& m_target;
    uint32_t m_amount;
    Comparison m_comparison;
};

//==============================================================================
//! \brief Send an Agent from the current Building with a load.
//! The Agent looks for a Building that accepts it.
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
    //! \param[in] type Agent type to send, copied.
    //! \param[in] target name the Agent looks for, such as "Work".
    //! \param[in] resources load the Agent carries, copied.
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
    //! \return the Agent type this command sends.
    //--------------------------------------------------------------------------
    [[nodiscard]] AgentType const& getAgentType() const
    {
        return m_type;
    }

    //--------------------------------------------------------------------------
    //! \return the name the Agent looks for, such as "Work".
    //--------------------------------------------------------------------------
    [[nodiscard]] Name const& getTarget() const
    {
        return m_target;
    }

    //--------------------------------------------------------------------------
    //! \return the load the Agent carries.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resources const& getResources() const
    {
        return m_resources;
    }

private:

    //! \brief Agent type to send.
    AgentType m_type;
    //! \brief Name the Agent looks for.
    Name m_target;
    //! \brief Load the Agent carries.
    Resources m_resources;
};

//==============================================================================
//! \brief Check the hour. The Rule runs only inside a time window.
//! This defines the working day of a City.
//!
//! The window is half-open: [from, to).
//! If \c from is greater than \c to, the window wraps past midnight.
//! \c hour \c between \c 22 \c 6 means night hours.
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
    //! \param[in] from first hour in the window, 0 to 23.
    //! \param[in] to first hour after the window, 0 to 23.
    //! If \c from equals \c to, the window is empty and the Rule never runs.
    //--------------------------------------------------------------------------
    RuleCommandHour(uint32_t from, uint32_t to) : m_from(from), m_to(to) {}

    //--------------------------------------------------------------------------
    //! \return true when the clock is inside the window.
    //! Returns false when context has no clock.
    //--------------------------------------------------------------------------
    bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Does nothing. Reading the clock changes nothing.
    //--------------------------------------------------------------------------
    void execute(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \return the window as text for the demo rule log.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::string getDescription() const override;

    //--------------------------------------------------------------------------
    //! \brief First hour in the window.
    //! OpeningHours reads this to know if a Building is open.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getFrom() const
    {
        return m_from;
    }

    //--------------------------------------------------------------------------
    //! \brief First hour after the window. See getFrom().
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getTo() const
    {
        return m_to;
    }

private:

    //! \brief First hour in the window, 0 to 23.
    uint32_t m_from;

    //! \brief First hour after the window, 0 to 23.
    uint32_t m_to;
};

//==============================================================================
//! \brief Test how many Buildings of one type are in the current Zone.
//!
//! Example:
//! \code
//! count Home less 12
//! \endcode
//==============================================================================
class RuleCommandCount: public IRuleCommand
{
public:

    RuleCommandCount(Name const& buildingType,
                     RuleCommandTest::Comparison comparison,
                     uint32_t amount)
        : m_buildingType(buildingType), m_comparison(comparison), m_amount(amount)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    Name m_buildingType;
    RuleCommandTest::Comparison m_comparison;
    uint32_t m_amount;
};

//==============================================================================
//! \brief Create a Building of the given type in the current Zone.
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

    RuleCommandSpawn(BuildingType const& buildingType, Placement placement)
        : m_buildingType(buildingType), m_placement(placement)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    BuildingType const& m_buildingType;
    Placement m_placement;
};

//==============================================================================
//! \brief Replace one Building type with another.
//! Keeps position and road attachment.
//!
//! Example:
//! \code
//! upgrade Home to Shop
//! \endcode
//==============================================================================
class RuleCommandUpgrade: public IRuleCommand
{
public:

    RuleCommandUpgrade(BuildingType const& fromType, BuildingType const& toType)
        : m_fromType(fromType), m_toType(toType)
    {
    }

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    BuildingType const& m_fromType;
    BuildingType const& m_toType;
};

//==============================================================================
//! \brief Destroy one Building of the given type in the current Zone.
//!
//! Example:
//! \code
//! destroy Home
//! \endcode
//==============================================================================
class RuleCommandDestroy: public IRuleCommand
{
public:

    explicit RuleCommandDestroy(Name const& buildingType) : m_buildingType(buildingType) {}

    bool validate(RuleContext& context) override;
    void execute(RuleContext& context) override;
    [[nodiscard]] std::string getDescription() const override;

private:

    Name m_buildingType;
};

} // namespace ogb

#endif
