//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_RULE_COMMAND_HPP
#  define OPEN_GLASSBOX_RULE_COMMAND_HPP

#  include "OpenGlassBox/Rule.hpp"

//==============================================================================
//! \brief
//!
//! Example:
//! \code
//! map Grass add 1
//! \endcode
//==============================================================================
class RuleCommandAdd : public IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    RuleCommandAdd(IRuleValue& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {}

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource has not reached the
    //! capacity.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Increase the amount of resource of the target.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    virtual std::string type() override;

private:

    IRuleValue& m_target;
    uint32_t  m_amount;
};

//==============================================================================
//! \brief
//!
//! Example:
//! \code
//! local People remove 1
//! \endcode
//==============================================================================
class RuleCommandRemove : public IRuleCommand
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    RuleCommandRemove(IRuleValue& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {}

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource is enough.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Decrease the amount of resource of the target.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    virtual std::string type() override;

private:

    IRuleValue& m_target;
    uint32_t  m_amount;
};

//==============================================================================
//! \brief
//!
//! Example:
//! \code
//! map Water greater 300
//! \endcode
//==============================================================================
class RuleCommandTest: public IRuleCommand
{
public:

    enum class Comparison { EQUALS, GREATER, LESS };

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    RuleCommandTest(IRuleValue& target, Comparison comparison, uint32_t amount)
        : m_target(target), m_amount(amount), m_comparison(comparison)
    {}

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource is enough.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Decrease the amount of resource of the target.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    virtual std::string type() override;

private:

    IRuleValue& m_target;
    uint32_t  m_amount;
    Comparison m_comparison;
};

//==============================================================================
//! \brief Class holding Agent information from a simulation script
//!
//! Example:
//! \code
//! agent People color 0xFFFF00 speed 10
//! \endcode
//==============================================================================
class RuleCommandAgent : public IRuleCommand, public AgentType
{
public:

    RuleCommandAgent(AgentType const& type, std::string const& target, Resources const& resources)
        : AgentType(type), m_target(target), m_resources(resources)
    {}

    virtual bool validate(RuleContext& context) override;
    virtual void execute(RuleContext& context) override;
    virtual std::string type() override;

public:

    std::string   m_target;
    Resources     m_resources;
};

//==============================================================================
//! \brief Predicate on the calendar.
//!
//! Example:
//! \code
//! hour between 8 18
//! \endcode
//==============================================================================
class RuleCommandHour : public IRuleCommand
{
public:

    RuleCommandHour(uint32_t from, uint32_t to)
        : m_from(from), m_to(to)
    {}

    virtual bool validate(RuleContext& context) override;
    virtual void execute(RuleContext& context) override;
    virtual std::string type() override;

private:

    uint32_t m_from;
    uint32_t m_to;
};

//==============================================================================
//! \brief Predicate on how many Units of a type sit inside the current Area.
//!
//! Example:
//! \code
//! count Home less 12
//! \endcode
//==============================================================================
class RuleCommandCount : public IRuleCommand
{
public:

    RuleCommandCount(std::string unitType, RuleCommandTest::Comparison comparison,
                     uint32_t amount)
        : m_unitType(std::move(unitType)), m_comparison(comparison), m_amount(amount)
    {}

    virtual bool validate(RuleContext& context) override;
    virtual void execute(RuleContext& context) override;
    virtual std::string type() override;

private:

    std::string m_unitType;
    RuleCommandTest::Comparison m_comparison;
    uint32_t m_amount;
};

//==============================================================================
//! \brief Create a Unit of the given type inside the current Area.
//!
//! Example:
//! \code
//! spawn Home at nearestWay
//! spawn Home at freeCell
//! \endcode
//==============================================================================
class RuleCommandSpawn : public IRuleCommand
{
public:

    enum class Placement { NearestWay, FreeCell };

    RuleCommandSpawn(UnitType const& unitType, Placement placement)
        : m_unitType(unitType), m_placement(placement)
    {}

    virtual bool validate(RuleContext& context) override;
    virtual void execute(RuleContext& context) override;
    virtual std::string type() override;

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
class RuleCommandUpgrade : public IRuleCommand
{
public:

    RuleCommandUpgrade(UnitType const& fromType, UnitType const& toType)
        : m_fromType(fromType), m_toType(toType)
    {}

    virtual bool validate(RuleContext& context) override;
    virtual void execute(RuleContext& context) override;
    virtual std::string type() override;

private:

    UnitType const& m_fromType;
    UnitType const& m_toType;
};

//==============================================================================
//! \brief Destroy one Unit of the given type inside the current Area.
//!
//! Example:
//! \code
//! destroy Home
//! \endcode
//==============================================================================
class RuleCommandDestroy : public IRuleCommand
{
public:

    explicit RuleCommandDestroy(std::string unitType)
        : m_unitType(std::move(unitType))
    {}

    virtual bool validate(RuleContext& context) override;
    virtual void execute(RuleContext& context) override;
    virtual std::string type() override;

private:

    std::string m_unitType;
};

#endif
