//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file RuleValue.hpp
//! \brief What a rule command reads and writes: a local, a global or a map
//! quantity.

#ifndef OPEN_GLASSBOX_RULE_VALUE_HPP
#define OPEN_GLASSBOX_RULE_VALUE_HPP

#include "OpenGlassBox/Rule.hpp"

namespace ogb
{

class Map;
class World;

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
    uint32_t capacity(RuleContext& context) override;

    //! \brief Pay into the treasury, up to its capacity.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Take from the treasury, down to nothing.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \brief \return the name of the stock, for the rule log of the demo.
    std::string const& type() const override;

private:

    //! \brief Names the stock. Not the stock itself: that one belongs to the
    //! Simulation and is reached through the context.
    Resource m_resource;
};

//==============================================================================
//! \brief What the entity running the rule holds: the resources of a building,
//! or of the cell of a Map.
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
    uint32_t capacity(RuleContext& context) override;

    //! \brief Add to the entity, up to its capacity.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Take from the entity, down to nothing.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \brief \return the name of the stock, for the rule log of the demo.
    std::string const& type() const override;

private:

    //! \brief Names the stock. The stock itself belongs to the building or to
    //! the Map cell and is reached through the context.
    Resource m_resource;
};

//==============================================================================
//! \brief One cell of a heatmap of the City, and its neighbours within the
//! reach of the building running the rule.
//!
//! What a script writes \c map. Unlike the two others this one is spatial: a
//! factory with \c mapRadius \c 3 pollutes a diamond of cells around itself,
//! and the amount asked for is spread over them. This is what ties the rules to
//! the ground.
//!
//! Example:
//! \code
//! map Pollution add 1
//! map Water greater 300
//! \endcode
//==============================================================================
class RuleValueMap: public IRuleValue
{
public:

    //--------------------------------------------------------------------------
    //! \brief \param[in] mapId name of the Map, as the script declared it. It
    //! is looked up in the City the rule runs in, so a script may name a Map
    //! that a given City does not have.
    //--------------------------------------------------------------------------
    explicit RuleValueMap(std::string const& mapId) : m_mapId(mapId) {}

    //! \brief \return what the cells within reach hold, added up.
    uint32_t get(RuleContext& context) override;

    //! \brief \return what the cells within reach may hold, added up.
    uint32_t capacity(RuleContext& context) override;

    //! \brief Spread that amount over the cells within reach.
    void add(RuleContext& context, uint32_t toAdd) override;

    //! \brief Take that amount from the cells within reach.
    void remove(RuleContext& context, uint32_t toRemove) override;

    //! \brief \return the name of the Map, for the rule log of the demo.
    std::string const& type() const override;

private:

    //--------------------------------------------------------------------------
    //! \brief The Map this value reads, looked up by name once per World.
    //!
    //! A map rule runs on every cell of the region, so a lookup by name would
    //! be paid hundreds of thousands of times per tick in a large city.
    //!
    //! \param[in] context the context of the rule, holding the City.
    //! \return the Map. Throws when the City has no Map of that name.
    //--------------------------------------------------------------------------
    Map& map(RuleContext& context);

private:

    //! \brief Name of the Map, as the script wrote it.
    std::string m_mapId;
    //! \brief Which World the cached lookup was made against. A rule outlives a
    //! City, being owned by the ruleset, so the cache has to be invalidated
    //! when another world is simulated.
    World const* m_world = nullptr;
    //! \brief The cached Map, valid as long as m_world is the world running.
    Map* m_map = nullptr;
};

} // namespace ogb

#endif
