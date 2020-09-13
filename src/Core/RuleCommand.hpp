#ifndef RULE_COMMAND_HPP
#define RULE_COMMAND_HPP

#include "Core/Rule.hpp"
#include "Core/Agent.hpp"

#if 0
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
    RuleCommandAdd(RuleValue const& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {}

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource has not reached the
    //! capacity.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext const& context) const override;

    //--------------------------------------------------------------------------
    //! \brief Increase the amount of resource of the target.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

private:

    RuleValue m_target;
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
    RuleCommandRemove(RuleValue const& target, uint32_t amount)
        : m_target(target), m_amount(amount)
    {}

    //--------------------------------------------------------------------------
    //! \brief Can be applied if the amount of resource is enough.
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext const& context) const override;

    //--------------------------------------------------------------------------
    //! \brief Decrease the amount of resource of the target.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

public:

    RuleValue target;
    uint32_t  amount;
};
#endif

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

    //--------------------------------------------------------------------------
    //! \brief Always true
    //--------------------------------------------------------------------------
    virtual bool validate(RuleContext const& context) const override;

    //--------------------------------------------------------------------------
    //! \brief Add a new agent in the city
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) override;

public:

    std::string   target;
    Resources     resources;
};

#endif
