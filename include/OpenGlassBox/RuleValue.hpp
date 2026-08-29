//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RuleValue.hpp
//! \brief What a rule command reads and writes: a local, a global or a layer
//! quantity.

#ifndef OPEN_GLASSBOX_RULE_VALUE_HPP
#define OPEN_GLASSBOX_RULE_VALUE_HPP

#include "OpenGlassBox/Rule.hpp"

namespace ogb
{

class Layer;
class City;

//==============================================================================
//! \brief The treasury of the Simulation: one stock shared by every City.
//!
//! What a script writes \c global. Money is the obvious one: a factory pays
//! into it and a shop is only allowed to sell while there is something in it,
//! which is how a whole country can run out of cash.
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
    //! \brief \param[in] resource which stock of the treasury is meant. Only
    //! its name is used; the amount and the capacity are those of the treasury.
    //--------------------------------------------------------------------------
    explicit RuleValueGlobal(Resource const& resource) : m_resource(resource) {}

    //! \brief \return how much of it the treasury holds.
    uint32_t get(RuleContext& context) override;

    //! \brief \return how much of it the treasury may hold.
    [[nodiscard]] uint32_t getCapacity(RuleContext& context) override;

    //! \brief Pay into the treasury, up to its capacity.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Take from the treasury, down to nothing.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \brief \return the name of the stock, for the rule log of the demo.
    [[nodiscard]] Name const& getTypeName() const override;

private:

    //! \brief Names the stock. Not the stock itself: that one belongs to the
    //! Simulation and is reached through the context.
    Resource m_resource;
};

//==============================================================================
//! \brief What the entity running the rule holds: the resources of a building,
//! or of the cell of a Layer.
//!
//! What a script writes \c local. This is where nearly everything happens: a
//! house holds People, a factory turns them into Goods, and both are local
//! values of the building running the rule.
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
    //! \brief \param[in] resource which stock of the entity is meant. Only its
    //! name is used.
    //--------------------------------------------------------------------------
    explicit RuleValueLocal(Resource const& resource) : m_resource(resource) {}

    //! \brief \return how much of it the entity holds. Zero when it holds none.
    uint32_t get(RuleContext& context) override;

    //! \brief \return how much of it the entity may hold.
    [[nodiscard]] uint32_t getCapacity(RuleContext& context) override;

    //! \brief Add to the entity, up to its capacity.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Take from the entity, down to nothing.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \brief \return the name of the stock, for the rule log of the demo.
    [[nodiscard]] Name const& getTypeName() const override;

private:

    //! \brief Names the stock. The stock itself belongs to the building or to
    //! the Layer cell and is reached through the context.
    Resource m_resource;
};

//==============================================================================
//! \brief One cell of a heatmap of the City, and its neighbours within the
//! reach of the building running the rule.
//!
//! What a script writes \c layer. Unlike the two others this one is spatial: a
//! factory with \c layerRadius \c 3 pollutes a diamond of cells around itself,
//! and the amount asked for is spread over them. This is what ties the rules to
//! the ground.
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
    //! \brief \param[in] layerId name of the Layer, as the script declared it.
    //! It is looked up in the City the rule runs in, so a script may name a
    //! Layer that a given City does not have.
    //--------------------------------------------------------------------------
    explicit RuleValueLayer(Name const& layerId) : m_layerId(layerId) {}

    //! \brief \return what the cells within reach hold, added up.
    uint32_t get(RuleContext& context) override;

    //! \brief \return what the cells within reach may hold, added up.
    [[nodiscard]] uint32_t getCapacity(RuleContext& context) override;

    //! \brief Spread that amount over the cells within reach.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Take that amount from the cells within reach.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \brief \return the name of the Layer, for the rule log of the demo.
    [[nodiscard]] Name const& getTypeName() const override;

private:

    //--------------------------------------------------------------------------
    //! \brief The layer this value reads, looked up by name once per City.
    //!
    //! A layer rule runs on every cell of the region, so a lookup by name would
    //! be paid hundreds of thousands of times per tick in a large city.
    //!
    //! \param[in] context the context of the rule, holding the City.
    //! \return the Layer. Throws when the City has no Layer of that name.
    //--------------------------------------------------------------------------
    Layer& layer(RuleContext& context);

private:

    //! \brief Name of the Layer, as the script wrote it.
    Name m_layerId;
    //! \brief Which City the cached lookup was made against. A rule outlives a
    //! City, being owned by the ruleset, so the cache has to be invalidated
    //! when another city runs it.
    City const* m_city = nullptr;
    //! \brief The cached layer, valid as long as m_city is the city running.
    Layer* m_layer = nullptr;
};

} // namespace ogb

#endif
