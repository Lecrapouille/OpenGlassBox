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

//! \brief How every kind of definition is stored: by name, in a map of owning
//! pointers. Pointers rather than values because a name is declared before its
//! body is read, and because everything else refers to these by address:
//! growing the map must not move them.
template<class T>
using Catalog = std::map<std::string, std::unique_ptr<T>, std::less<>>;

//==============================================================================
//! \brief Everything a script defines: the recipes the entities are built from,
//! and the rules and commands they run.
//!
//! This owns the ruleset, and it is the reason a script has to outlive the
//! cities loaded from it. A building keeps a const reference to its recipe, and
//! a rule keeps raw pointers to its commands. Nothing here may be destroyed
//! while a city stands: clear() is only safe once the world is gone.
//!
//! Each kind of definition has three ways in, and the one you pick says what
//! you expect. \c getXxx throws, for code that knows the ruleset declares the
//! name. \c findXxx answers nullptr, which is what the parser uses: an unknown
//! name is an error it reports with a line number rather than an exception.
//! \c addXxx refuses to overwrite, so a name declared twice is an error rather
//! than a silent replacement.
//==============================================================================
class ScriptDefinitions
{
public:

    //--------------------------------------------------------------------------
    //! \brief Look up a recipe by name.
    //! \param[in] id the name to look for.
    //! \return the recipe.
    //! \throw std::out_of_range when the name was never declared.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource const& getResource(std::string const& id) const;
    [[nodiscard]] PathType const& getPathType(std::string const& id) const;
    [[nodiscard]] SegmentType const& getSegmentType(std::string const& id) const;
    [[nodiscard]] AgentType const& getAgentType(std::string const& id) const;
    [[nodiscard]] UnitType const& getUnitType(std::string const& id) const;
    [[nodiscard]] LayerType const& getLayerType(std::string const& id) const;
    [[nodiscard]] RuleLayer const& getRuleLayer(std::string const& id) const;
    [[nodiscard]] RuleUnit const& getRuleUnit(std::string const& id) const;
    [[nodiscard]] RuleZone const& getRuleZone(std::string const& id) const;
    [[nodiscard]] ZoneType const& getZoneType(std::string const& id) const;

    //--------------------------------------------------------------------------
    //! \brief Look a recipe up by name, without throwing.
    //!
    //! What the parser uses: an unknown name is an error it wants to report
    //! with a line and a column, not an exception.
    //!
    //! \param[in] id the name to look for.
    //! \return the recipe, or nullptr when the name was never declared.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource* findResource(std::string const& id);
    [[nodiscard]] PathType* findPathType(std::string const& id);
    [[nodiscard]] SegmentType* findSegmentType(std::string const& id);
    [[nodiscard]] AgentType* findAgentType(std::string const& id);
    [[nodiscard]] UnitType* findUnitType(std::string const& id);
    [[nodiscard]] LayerType* findLayerType(std::string const& id);
    [[nodiscard]] RuleLayer* findRuleLayer(std::string const& id);
    [[nodiscard]] RuleUnit* findRuleUnit(std::string const& id);
    [[nodiscard]] RuleZone* findRuleZone(std::string const& id);
    [[nodiscard]] ZoneType* findZoneType(std::string const& id);

    //--------------------------------------------------------------------------
    //! \brief Look a recipe up by name, without throwing, on a catalogue held
    //! for reading only. What a save reader uses to tell whether the ruleset it
    //! is given declares everything the file names.
    //!
    //! \param[in] id the name to look for.
    //! \return the recipe, or nullptr when the name was never declared.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource const* findResource(std::string const& id) const;
    [[nodiscard]] PathType const* findPathType(std::string const& id) const;
    [[nodiscard]] SegmentType const* findSegmentType(std::string const& id) const;
    [[nodiscard]] AgentType const* findAgentType(std::string const& id) const;
    [[nodiscard]] UnitType const* findUnitType(std::string const& id) const;
    [[nodiscard]] LayerType const* findLayerType(std::string const& id) const;
    [[nodiscard]] RuleLayer const* findRuleLayer(std::string const& id) const;
    [[nodiscard]] RuleUnit const* findRuleUnit(std::string const& id) const;
    [[nodiscard]] RuleZone const* findRuleZone(std::string const& id) const;
    [[nodiscard]] ZoneType const* findZoneType(std::string const& id) const;

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
    SegmentType* addSegmentType(std::string const& id);
    AgentType* addAgentType(std::string const& id);
    UnitType* addUnitType(std::string const& id);
    LayerType* addLayerType(std::string const& id);
    RuleLayer* addRuleLayer(std::string const& id);
    RuleUnit* addRuleUnit(std::string const& id);
    RuleZone* addRuleZone(std::string const& id);
    ZoneType* addZoneType(std::string const& id);

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
    [[nodiscard]] Catalog<Resource> const& getResources() const
    {
        return m_resources;
    }
    [[nodiscard]] Catalog<PathType> const& getPathTypes() const
    {
        return m_pathTypes;
    }
    [[nodiscard]] Catalog<SegmentType> const& getSegmentTypes() const
    {
        return m_segmentTypes;
    }
    [[nodiscard]] Catalog<AgentType> const& getAgentTypes() const
    {
        return m_agentTypes;
    }
    [[nodiscard]] Catalog<UnitType> const& getUnitTypes() const
    {
        return m_unitTypes;
    }
    [[nodiscard]] Catalog<LayerType> const& getLayerTypes() const
    {
        return m_layerTypes;
    }
    [[nodiscard]] Catalog<RuleLayer> const& getRuleLayers() const
    {
        return m_ruleLayers;
    }
    [[nodiscard]] Catalog<RuleUnit> const& getRuleUnits() const
    {
        return m_ruleUnits;
    }
    [[nodiscard]] Catalog<RuleZone> const& getRuleZones() const
    {
        return m_ruleZones;
    }
    [[nodiscard]] Catalog<ZoneType> const& getZoneTypes() const
    {
        return m_zoneTypes;
    }

    //--------------------------------------------------------------------------
    //! \brief Drop the whole ruleset.
    //!
    //! \note Only safe once nothing refers to it: every city has to be gone
    //! first, since its buildings hold references into this. Calling it on a
    //! live world leaves dangling references everywhere.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief \return true when the script declared nothing at all, which is
    //! what an unparsed or a failed ruleset looks like.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isEmpty() const;

private:

    //! \brief The kinds of resource, by name: what everything else is counted
    //! in.
    Catalog<Resource> m_resources;
    //! \brief The kinds of network, by name: roads, rails, pipes.
    Catalog<PathType> m_pathTypes;
    //! \brief The kinds of segment, by name, with their speed and capacity.
    Catalog<SegmentType> m_segmentTypes;
    //! \brief The kinds of agent, by name, with their speed and colour.
    Catalog<AgentType> m_agentTypes;
    //! \brief The kinds of building, by name, with their rules and capacities.
    Catalog<UnitType> m_unitTypes;
    //! \brief The kinds of layer, by name, with their cap and their rules.
    Catalog<LayerType> m_layerTypes;
    //! \brief The rules a layer may run, by name.
    Catalog<RuleLayer> m_ruleLayers;
    //! \brief The rules a building may run, by name.
    Catalog<RuleUnit> m_ruleUnits;
    //! \brief The rules a zone may run, by name.
    Catalog<RuleZone> m_ruleZones;
    //! \brief The kinds of zone, by name, with their rules.
    Catalog<ZoneType> m_zoneTypes;
    //! \brief The commands of every rule. Held here, and not by the rules, so
    //! that a rule may keep a raw pointer to one and two rules may share one.
    std::vector<std::unique_ptr<IRuleCommand>> m_commands;
    //! \brief The places those commands read and write. Same reasoning as
    //! m_commands.
    std::vector<std::unique_ptr<IRuleValue>> m_values;
};

} // namespace ogb

#endif
