#ifndef RULE_HPP
#define RULE_HPP

#  include "Core/Resources.hpp"
#  include <string>
#  include <vector>

class RuleCommand;
class City;
class Unit;

struct RuleContext
{
    //! \brief
    City       *city;
    //! \brief
    Unit       *unit;
    //! \brief Local resources
    Resources  *locals;
    //! \brief Global resources
    Resources  *globals;
    //! \brief Position on the grid
    uint32_t    u, v;
    //! \brief Radius action of Map resources
    float       radius;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class Rule
{
public:

    virtual ~Rule() = default;

    virtual bool execute(RuleContext& context);
    virtual void setOption(std::string const& optionId, std::string const& val);
    uint32_t rate() const { return m_rate; }

public:

    std::string               m_id;
    uint32_t                  m_rate = 1u;
    std::vector<RuleCommand*> m_commands;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class RuleMap : public Rule
{
protected:

    bool     m_randomTiles = false;
    uint32_t m_randomTilesPercent = 10u;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class RuleUnit : public Rule
{
public:

    virtual bool execute(RuleContext& context) override
    {
        return (Rule::execute(context))
                ? true
                : false; // FIXME m_onFailure.execute(context);
    }

public:

    //RuleUnit m_onFailure;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class RuleCommand
{
public:

    virtual bool validate(RuleContext& /*context*/)
    {
        return true;
    }

    virtual void execute(RuleContext& /*context*/)
    {}
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
#if 0
class RuleCommandAgent : RuleCommand
{
public:

    virtual bool validate(RuleContext& context) override
    {
        return true;
    }

    virtual void execute(RuleContext& context) override
    {
        //context.box.AddAgent(agentType, context.unit.position,
        //                     context.unit, resources, searchTarget);
    }

public:

    std::string   m_searchTarget;
    Agent        *m_agent;
    Resources    *m_resources;
};
#endif

#endif
