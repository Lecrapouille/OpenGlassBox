//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RuleValue.hpp
//! \brief Values read or written by rules: local, global and per-map quantities.


#ifndef OPEN_GLASSBOX_RULE_VALUE_HPP
#  define OPEN_GLASSBOX_RULE_VALUE_HPP

#  include "OpenGlassBox/Rule.hpp"

namespace ogb {

//==============================================================================
//! \brief
//==============================================================================
class RuleValueGlobal : public IRuleValue
{
public:

    RuleValueGlobal(Resource const& resource)
        : m_resource(resource)
    {}

    virtual uint32_t get(RuleContext& context) override;
    virtual uint32_t capacity(RuleContext& context) override;
    virtual void add(RuleContext& context, uint32_t toAdd) override;
    virtual void remove(RuleContext& context, uint32_t toRemove) override;
    virtual std::string const& type() const override;

private:

    Resource m_resource;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleValueLocal : public IRuleValue
{
public:

    RuleValueLocal(Resource const& resource)
        : m_resource(resource)
    {}

    virtual uint32_t get(RuleContext& context) override;
    virtual uint32_t capacity(RuleContext& context) override;
    virtual void add(RuleContext& context, uint32_t toAdd) override;
    virtual void remove(RuleContext& context, uint32_t toRemove) override;
    virtual std::string const& type() const override;

private:

    Resource m_resource;
};

//==============================================================================
//! \brief
//==============================================================================
class RuleValueMap : public IRuleValue
{
public:

    RuleValueMap(std::string const& mapId)
        : m_mapId(mapId)
    {}

    virtual uint32_t get(RuleContext& context) override;
    virtual uint32_t capacity(RuleContext& context) override;
    virtual void add(RuleContext& context, uint32_t toAdd) override;
    virtual void remove(RuleContext& context, uint32_t toRemove) override;
    virtual std::string const& type() const override;

private:

    std::string m_mapId;
};

} // namespace ogb

#endif
