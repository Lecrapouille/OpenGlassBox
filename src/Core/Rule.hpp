#ifndef RULE_HPP
#define RULE_HPP

#include "Core/Resources.hpp"

class City;
class Unit;
class Resources;

//==============================================================================
//! \brief
//==============================================================================
struct RuleContext
{
    //! \brief
    City* city;
    //! \brief
    Unit* unit;
    //! \brief Local resources
    Resources* locals;
    //! \brief Global resources
    Resources* globals;
    //! \brief Position on the grid
    uint32_t u, v;
    //! \brief Radius action of Map resources
    uint32_t radius;
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
    virtual bool validate(RuleContext const& context) const = 0;

    //--------------------------------------------------------------------------
    //! \brief Apply the command on the current context.
    //--------------------------------------------------------------------------
    virtual void execute(RuleContext& context) = 0;
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
    virtual uint32_t capacity(RuleContext& context)  = 0;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void add(RuleContext& context, uint32_t toAdd) = 0;

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void remove(RuleContext& context, uint32_t toRemove) = 0;
};

//==============================================================================
//! \brief
//==============================================================================
class IRule
{
public:

    IRule(std::string const& name);
    virtual ~IRule() = default;
    virtual bool execute(RuleContext& context);
    virtual void setOption(std::string const& option, std::string const& value);

private:

    std::string                m_id;
    uint32_t                   m_rate = 1u;
    std::vector<IRuleCommand*> m_commands;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleMap: public IRule
{
public:

    RuleMap(std::string const& name, bool randomTiles = false, uint32_t randomTilesPercent = 10u)
        : IRule(name),
          m_randomTiles(randomTiles),
          m_randomTilesPercent(std::min(100u, randomTilesPercent))
    {}

    //--------------------------------------------------------------------------
    //! \brief Use randomized values ?
    //--------------------------------------------------------------------------
    bool IsRandom() const
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

#endif
