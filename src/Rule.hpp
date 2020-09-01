#ifndef RULE_HPP
#define RULE_HPP

#  include "Resource.hpp"
#  include "Unit.hpp"
#  include <string>
#  include <vector>

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
class Rule
{
public:

    struct Context
    {
        //Box box;
        Unit unit;
        ResourceBin localResources;
        ResourceBin globalResources;
        uint32_t mapPositionX;
        uint32_t mapPositionY;
        uint32_t mapPositionRadius;
    };

public:

    virtual Rule() = default;

    virtual bool execute(Rule::Context& context);
    virtual void setOption(std::string const& optionId, std::string const& val);

public:

    std::string id;
    uint32_t m_rate = 1u;
    std::vector<RuleCommand*> m_commands;
};

// -----------------------------------------------------------------------------
//!
// -----------------------------------------------------------------------------
struct RuleMap : public Rule
{
protected:

    bool randomTiles = false;
    int randomTilesPercent = 10;
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

#endif
