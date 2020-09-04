#ifndef RULE_HPP
#define RULE_HPP

#  include "Unit.hpp"
#  include <string>
#  include <vector>

class RuleCommand;

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class Rule
{
public:

    struct Context
    {
        //Box box;
        Unit        m_unit;
        Resources m_localResources;
        Resources m_globalResources;
        uint32_t    m_mapPositionX;
        uint32_t    m_mapPositionY;
        uint32_t    m_mapPositionRadius;
    };

public:

    virtual Rule() = default;

    virtual bool execute(Rule::Context& context);
    virtual void setOption(std::string const& optionId, std::string const& val);

public:

    std::string               m_id;
    uint32_t                  m_rate = 1u;
    std::vector<RuleCommand*> m_commands;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct RuleMap : public Rule
{
protected:

    bool     m_randomTiles = false;
    uint32_t m_randomTilesPercent = 10u;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct RuleUnit : public Rule
{
public:

    virtual bool execute(Rule::Context context) override
    {
        return (Rule::execute(context))
                ? true
                : m_onFailure.execute(context);
    }

public:

    RuleUnit m_onFailure;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class RuleCommand
{
public:

    virtual bool validate(Rule::Context context)
    {
        return true;
    }

    virtual void execute(Rule::Context context)
    {}
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class RuleCommandAgent : RuleCommand
{
public:

    virtual bool validate(Rule::Context& context) override
    {
        return true;
    }

    virtual void execute(Rule::Context& context) override
    {
        //context.box.AddAgent(agentType, context.unit.position,
        //                     context.unit, resources, searchTarget);
    }

public:

  string searchTarget;
  SimAgentType agentType;
  SimResourcesCollection resources;
};

#endif
