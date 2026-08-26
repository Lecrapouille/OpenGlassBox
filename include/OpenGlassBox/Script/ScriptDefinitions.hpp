//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file ScriptDefinitions.hpp
//! \brief Owned catalogue of types and rules produced by a parsed script.

#ifndef OPEN_GLASSBOX_SCRIPT_DEFINITIONS_HPP
#define OPEN_GLASSBOX_SCRIPT_DEFINITIONS_HPP

#include "OpenGlassBox/RuleCommand.hpp"
#include "OpenGlassBox/RuleValue.hpp"

#include <map>
#include <memory>
#include <stdexcept>

namespace ogb
{

//==============================================================================
//! \brief Everything a script defines: the recipes the entities are built from,
//! and the rules and commands they run.
//!
//! This is the owner of the ruleset, and the reason a script has to outlive the
//! towns loaded from it. A building keeps a const reference to its recipe, and
//! a rule keeps raw pointers to its commands, so nothing here may be destroyed
//! while a City is standing: clear() is only safe once the world has been
//! thrown away.
//!
//! Every kind of thing is stored the same way, by name, in a map of owning
//! pointers. Pointers rather than values because a name is declared before its
//! body is read, and because everything else refers to these by address:
//! growing a map must not move them.
//!
//! Three ways in are offered for each kind, and which one to use says what the
//! caller expects. \c get throws, and is for code that knows the ruleset
//! declares the name. \c find answers nullptr, and is what the parser uses, an
//! unknown name being an error it would rather report with a line number than
//! an exception. \c add refuses to overwrite, so that a name declared twice is
//! an error rather than a silent replacement.
//==============================================================================
class ScriptDefinitions
{
public:

    //--------------------------------------------------------------------------
    //! \brief Look a recipe up by name.
    //! \param[in] id the name the script declared.
    //! \return the recipe, which lives as long as the ruleset does.
    //! \throw std::out_of_range when the name was never declared.
    //--------------------------------------------------------------------------
    Resource const& getResource(std::string const& id) const;
    PathType const& getPathType(std::string const& id) const;
    WayType const& getWayType(std::string const& id) const;
    AgentType const& getAgentType(std::string const& id) const;
    UnitType const& getUnitType(std::string const& id) const;
    MapType const& getMapType(std::string const& id) const;
    RuleMap const& getRuleMap(std::string const& id) const;
    RuleUnit const& getRuleUnit(std::string const& id) const;
    RuleArea const& getRuleArea(std::string const& id) const;
    AreaType const& getAreaType(std::string const& id) const;

    //--------------------------------------------------------------------------
    //! \brief Look a recipe up by name, without throwing.
    //!
    //! What the parser uses: an unknown name is an error it wants to report
    //! with a line and a column, not an exception.
    //!
    //! \param[in] id the name to look for.
    //! \return the recipe, or nullptr when the name was never declared.
    //--------------------------------------------------------------------------
    Resource* findResource(std::string const& id);
    PathType* findPathType(std::string const& id);
    WayType* findWayType(std::string const& id);
    AgentType* findAgentType(std::string const& id);
    UnitType* findUnitType(std::string const& id);
    MapType* findMapType(std::string const& id);
    RuleMap* findRuleMap(std::string const& id);
    RuleUnit* findRuleUnit(std::string const& id);
    RuleArea* findRuleArea(std::string const& id);
    AreaType* findAreaType(std::string const& id);

    //--------------------------------------------------------------------------
    //! \brief Declare a recipe, empty, for the parser to fill in.
    //!
    //! A name already taken is refused rather than overwritten. Silently
    //! replacing it, which is what used to happen, made a name typed twice
    //! almost impossible to notice.
    //!
    //! \param[in] id the name to declare.
    //! \return the new recipe, or nullptr when the name is already taken.
    //--------------------------------------------------------------------------
    Resource* addResource(std::string const& id);
    PathType* addPathType(std::string const& id);
    WayType* addWayType(std::string const& id);
    AgentType* addAgentType(std::string const& id);
    UnitType* addUnitType(std::string const& id);
    MapType* addMapType(std::string const& id);
    RuleMap* addRuleMap(std::string const& id);
    RuleUnit* addRuleUnit(std::string const& id);
    RuleArea* addRuleArea(std::string const& id);
    AreaType* addAreaType(std::string const& id);

    //--------------------------------------------------------------------------
    //! \brief Take ownership of a command the parser built, and hand back a
    //! plain pointer to put inside a rule.
    //!
    //! Commands are not named and cannot be looked up: they only exist inside
    //! the rule that uses them. They are kept here all the same, so that they
    //! live exactly as long as the ruleset and a rule may hold a raw pointer.
    //!
    //! \param[in] command the command, whose ownership is taken.
    //! \return a pointer to it, valid until clear().
    //--------------------------------------------------------------------------
    IRuleCommand* own(std::unique_ptr<IRuleCommand> command);

    //--------------------------------------------------------------------------
    //! \brief The same, for the place a command reads and writes.
    //! \param[in] value the value, whose ownership is taken.
    //! \return a pointer to it, valid until clear().
    //--------------------------------------------------------------------------
    IRuleValue* own(std::unique_ptr<IRuleValue> value);

    //--------------------------------------------------------------------------
    //! \brief List what the script declared, by name. What the editor offers
    //! the player as a choice of road, building or zone to place, and what the
    //! debugger walks.
    //--------------------------------------------------------------------------
    std::map<std::string, std::unique_ptr<Resource>, std::less<>> const&
    resources() const
    {
        return m_resources;
    }
    std::map<std::string, std::unique_ptr<PathType>, std::less<>> const&
    pathTypes() const
    {
        return m_pathTypes;
    }
    std::map<std::string, std::unique_ptr<WayType>, std::less<>> const&
    wayTypes() const
    {
        return m_wayTypes;
    }
    std::map<std::string, std::unique_ptr<AgentType>, std::less<>> const&
    agentTypes() const
    {
        return m_agentTypes;
    }
    std::map<std::string, std::unique_ptr<UnitType>, std::less<>> const&
    unitTypes() const
    {
        return m_unitTypes;
    }
    std::map<std::string, std::unique_ptr<MapType>, std::less<>> const&
    mapTypes() const
    {
        return m_mapTypes;
    }
    std::map<std::string, std::unique_ptr<RuleMap>, std::less<>> const&
    ruleMaps() const
    {
        return m_ruleMaps;
    }
    std::map<std::string, std::unique_ptr<RuleUnit>, std::less<>> const&
    ruleUnits() const
    {
        return m_ruleUnits;
    }
    std::map<std::string, std::unique_ptr<RuleArea>, std::less<>> const&
    ruleAreas() const
    {
        return m_ruleAreas;
    }
    std::map<std::string, std::unique_ptr<AreaType>, std::less<>> const&
    areaTypes() const
    {
        return m_areaTypes;
    }

    //--------------------------------------------------------------------------
    //! \brief Drop the whole ruleset.
    //!
    //! \note Only safe once nothing refers to it: every town has to be gone
    //! first, since its buildings hold references into this. Calling it on a
    //! live world leaves dangling references everywhere.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief \return true when the script declared nothing at all, which is
    //! what an unparsed or a failed ruleset looks like.
    //--------------------------------------------------------------------------
    bool empty() const;

private:

    //! \brief The kinds of resource, by name: what everything else is counted
    //! in.
    std::map<std::string, std::unique_ptr<Resource>, std::less<>> m_resources;
    //! \brief The kinds of network, by name: roads, rails, pipes.
    std::map<std::string, std::unique_ptr<PathType>, std::less<>> m_pathTypes;
    //! \brief The kinds of segment, by name, with their speed and capacity.
    std::map<std::string, std::unique_ptr<WayType>, std::less<>> m_wayTypes;
    //! \brief The kinds of agent, by name, with their speed and colour.
    std::map<std::string, std::unique_ptr<AgentType>, std::less<>> m_agentTypes;
    //! \brief The kinds of building, by name, with their rules and capacities.
    std::map<std::string, std::unique_ptr<UnitType>, std::less<>> m_unitTypes;
    //! \brief The kinds of layer, by name, with their cap and their rules.
    std::map<std::string, std::unique_ptr<MapType>, std::less<>> m_mapTypes;
    //! \brief The rules a layer may run, by name.
    std::map<std::string, std::unique_ptr<RuleMap>, std::less<>> m_ruleMaps;
    //! \brief The rules a building may run, by name.
    std::map<std::string, std::unique_ptr<RuleUnit>, std::less<>> m_ruleUnits;
    //! \brief The rules a zone may run, by name.
    std::map<std::string, std::unique_ptr<RuleArea>, std::less<>> m_ruleAreas;
    //! \brief The kinds of zone, by name, with their rules.
    std::map<std::string, std::unique_ptr<AreaType>, std::less<>> m_areaTypes;
    //! \brief The commands of every rule. Held here, and not by the rules, so
    //! that a rule may keep a raw pointer to one and two rules may share one.
    std::vector<std::unique_ptr<IRuleCommand>> m_commands;
    //! \brief The places those commands read and write. Same reasoning as
    //! m_commands.
    std::vector<std::unique_ptr<IRuleValue>> m_values;
};

} // namespace ogb

#endif
