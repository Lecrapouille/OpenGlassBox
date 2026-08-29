//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RuleValue.hpp
//! \brief What a Rule command reads and writes: local, global or Layer value.

#ifndef OPEN_GLASSBOX_RULE_VALUE_HPP
#define OPEN_GLASSBOX_RULE_VALUE_HPP

#include "OpenGlassBox/Rule.hpp"

namespace ogb
{

class Layer;
class City;

//==============================================================================
//! \brief Simulation treasury: one stock shared by every City.
//!
//! The script writes \c global.
//! Money is the usual example.
//! A factory pays into it. A shop sells only while it has Money.
//! A whole country can run out of cash.
//!
//! Example:
//! \code
//! global Money add 1
//! global Money greater 0
//! \endcode
//==============================================================================
class RuleValueGlobal: public IRuleValue
{
public:

    //--------------------------------------------------------------------------
    //! \param[in] resource which treasury stock to use.
    //! Only the name is used. Amount and capacity come from the treasury.
    //--------------------------------------------------------------------------
    explicit RuleValueGlobal(Resource const& resource) : m_resource(resource) {}

    //! \return how much the treasury holds.
    uint32_t get(RuleContext& context) override;

    //! \return maximum amount the treasury can hold.
    [[nodiscard]] uint32_t getCapacity(RuleContext& context) override;

    //! \brief Add to the treasury, up to capacity.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Remove from the treasury, down to zero.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \return stock name for the demo rule log.
    [[nodiscard]] Name const& getTypeName() const override;

private:

    //! \brief Names the stock. The stock lives in the Simulation.
    //! Reached through the context.
    Resource m_resource;
};

//==============================================================================
//! \brief Resources of the entity running the Rule.
//! A Building or a Layer cell.
//!
//! The script writes \c local.
//! A house holds People. A factory makes Goods.
//! Both are local values of the Building running the Rule.
//!
//! Example:
//! \code
//! local People greater 0
//! local People remove 1
//! local Goods add 1
//! \endcode
//==============================================================================
class RuleValueLocal: public IRuleValue
{
public:

    //--------------------------------------------------------------------------
    //! \param[in] resource which entity stock to use.
    //! Only the name is used.
    //--------------------------------------------------------------------------
    explicit RuleValueLocal(Resource const& resource) : m_resource(resource) {}

    //! \return how much the entity holds. Zero if none.
    uint32_t get(RuleContext& context) override;

    //! \return maximum amount the entity can hold.
    [[nodiscard]] uint32_t getCapacity(RuleContext& context) override;

    //! \brief Add to the entity, up to capacity.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Remove from the entity, down to zero.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \return stock name for the demo rule log.
    [[nodiscard]] Name const& getTypeName() const override;

private:

    //! \brief Names the stock. The stock belongs to the Building or Layer cell.
    //! Reached through the context.
    Resource m_resource;
};

//==============================================================================
//! \brief One City cell in a heatmap, and neighbours in the Building reach.
//!
//! The script writes \c layer.
//! This value is spatial.
//! A factory with \c layerRadius \c 3 affects a diamond of cells around it.
//! The amount is spread over those cells.
//! This ties Rules to the ground.
//!
//! Example:
//! \code
//! layer Pollution add 1
//! layer Water greater 300
//! \endcode
//==============================================================================
class RuleValueLayer: public IRuleValue
{
public:

    //--------------------------------------------------------------------------
    //! \param[in] layerId Layer name from the script.
    //! Looked up in the City where the Rule runs.
    //! The script may name a Layer this City does not have.
    //--------------------------------------------------------------------------
    explicit RuleValueLayer(Name const& layerId) : m_layerId(layerId) {}

    //! \return sum over cells in reach.
    uint32_t get(RuleContext& context) override;

    //! \return maximum sum over cells in reach.
    [[nodiscard]] uint32_t getCapacity(RuleContext& context) override;

    //! \brief Spread the amount over cells in reach.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Remove the amount from cells in reach.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \return Layer name for the demo rule log.
    [[nodiscard]] Name const& getTypeName() const override;

private:

    //--------------------------------------------------------------------------
    //! \brief Find the Layer by name, cached per City.
    //!
    //! A Layer Rule runs on every cell in the region.
    //! A name lookup each time would cost too much in a large City.
    //!
    //! \param[in] context Rule context with the City.
    //! \return the Layer. Throws if the City has no Layer with that name.
    //--------------------------------------------------------------------------
    Layer& layer(RuleContext& context);

private:

    //! \brief Layer name from the script.
    Name m_layerId;
    //! \brief City used for the cached lookup.
    //! A Rule outlives a City. The Ruleset owns the Rule.
    //! The cache must reset when another City runs it.
    City const* m_city = nullptr;
    //! \brief Cached Layer. Valid while m_city is the running City.
    Layer* m_layer = nullptr;
};

} // namespace ogb

#endif
