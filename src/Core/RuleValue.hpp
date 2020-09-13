#ifndef RULEVALUE_HPP
#define RULEVALUE_HPP

#include "Core/Rule.hpp"

//==============================================================================
//! \brief
//==============================================================================
class RuleValueGlobal : public IRuleValue
{
public:

    RuleValueGlobal(Resource const& resource_)
        : resource(resource_)
    {}

    virtual uint32_t get(RuleContext& context) override;
    virtual uint32_t capacity(RuleContext& context) override;
    virtual void add(RuleContext& context, uint32_t toAdd) override;
    virtual void remove(RuleContext& context, uint32_t toRemove) override;

private:

    Resource resource;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleValueLocal : public IRuleValue
{
public:

    RuleValueLocal(Resource const& resource_)
        : resource(resource_)
    {}

    virtual uint32_t get(RuleContext& context) override;
    virtual uint32_t capacity(RuleContext& context) override;
    virtual void add(RuleContext& context, uint32_t toAdd) override;
    virtual void remove(RuleContext& context, uint32_t toRemove) override;

private:

    Resource resource;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleValueMap : public IRuleValue
{
public:

    RuleValueMap(std::string const& mapId_)
        : mapId(mapId_)
    {}

    virtual uint32_t get(RuleContext& context) override;
    virtual uint32_t capacity(RuleContext& context) override;
    virtual void add(RuleContext& context, uint32_t toAdd) override;
    virtual void remove(RuleContext& context, uint32_t toRemove) override;

private:

    std::string mapId;
};

#endif
