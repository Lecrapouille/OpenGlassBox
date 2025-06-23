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

    //--------------------------------------------------------------------------
    //! \brief Always true
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext& context) override;

    //--------------------------------------------------------------------------
    //! \brief Add a new agent in the city
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

    // -------------------------------------------------------------------------
    //! \brief
    // -------------------------------------------------------------------------
    virtual std::string type() override;

public:

    std::string   m_target;
    Resources     m_resources;
};

#endif
