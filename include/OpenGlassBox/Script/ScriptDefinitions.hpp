//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file ScriptDefinitions.hpp
//! \brief Owned catalogue of types and rules from a parsed script.

#ifndef OPEN_GLASSBOX_SCRIPT_DEFINITIONS_HPP
#define OPEN_GLASSBOX_SCRIPT_DEFINITIONS_HPP

#include "OpenGlassBox/RuleCommand.hpp"
#include "OpenGlassBox/RuleValue.hpp"

#include <map>
#include <memory>
#include <stdexcept>

namespace ogb
{

//! \brief Store definitions by name in a map of owning pointers. Pointers are
//! used because a name is declared before its body is read, and other code
//! refers to these by address. Growing the map must not move them.
template<class T>
using Catalog = std::map<std::string, std::unique_ptr<T>, std::less<>>;

//==============================================================================
//! \brief Everything a script defines: recipes for entities, rules, and
//! commands.
//!
//! This owns the ruleset. A script must outlive cities loaded from it. A
//! building keeps a const reference to its recipe. A rule keeps raw pointers to
//! its commands. Nothing here may be destroyed while a city exists. Call
//! clear() only after the world is gone.
//!
//! Each definition type has three accessors:
//! - \c getXxx throws when the name must exist;
//! - \c findXxx returns nullptr for the parser to report a line number;
//! - \c addXxx refuses duplicate names instead of overwriting silently.
//==============================================================================
class ScriptDefinitions
{
public:

    //--------------------------------------------------------------------------
    //! \brief Look up a definition by name.
    //! \param[in] id the name to find.
    //! \return the definition.
    //! \throw std::out_of_range when the name was never declared.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource const& getResource(std::string const& id) const;
    [[nodiscard]] PathType const& getPathType(std::string const& id) const;
    [[nodiscard]] SegmentType const& getSegmentType(std::string const& id) const;
    [[nodiscard]] AgentType const& getAgentType(std::string const& id) const;
    [[nodiscard]] BuildingType const& getBuildingType(std::string const& id) const;
    [[nodiscard]] LayerType const& getLayerType(std::string const& id) const;
    [[nodiscard]] RuleLayer const& getRuleLayer(std::string const& id) const;
    [[nodiscard]] RuleBuilding const& getRuleBuilding(std::string const& id) const;
    [[nodiscard]] RuleZone const& getRuleZone(std::string const& id) const;
    [[nodiscard]] ZoneType const& getZoneType(std::string const& id) const;

    //--------------------------------------------------------------------------
    //! \brief Look up a definition by name without throwing.
    //!
    //! Used by the parser: an unknown name becomes an error with line and
    //! column, not an exception.
    //!
    //! \param[in] id the name to find.
    //! \return the definition, or nullptr when the name was never declared.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource* findResource(std::string const& id);
    [[nodiscard]] PathType* findPathType(std::string const& id);
    [[nodiscard]] SegmentType* findSegmentType(std::string const& id);
    [[nodiscard]] AgentType* findAgentType(std::string const& id);
    [[nodiscard]] BuildingType* findBuildingType(std::string const& id);
    [[nodiscard]] LayerType* findLayerType(std::string const& id);
    [[nodiscard]] RuleLayer* findRuleLayer(std::string const& id);
    [[nodiscard]] RuleBuilding* findRuleBuilding(std::string const& id);
    [[nodiscard]] RuleZone* findRuleZone(std::string const& id);
    [[nodiscard]] ZoneType* findZoneType(std::string const& id);

    //--------------------------------------------------------------------------
    //! \brief Look up a definition on a read-only catalogue. Used by save
    //! loading to check that the ruleset declares every name in the file.
    //!
    //! \param[in] id the name to find.
    //! \return the definition, or nullptr when the name was never declared.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource const* findResource(std::string const& id) const;
    [[nodiscard]] PathType const* findPathType(std::string const& id) const;
    [[nodiscard]] SegmentType const* findSegmentType(std::string const& id) const;
    [[nodiscard]] AgentType const* findAgentType(std::string const& id) const;
    [[nodiscard]] BuildingType const* findBuildingType(std::string const& id) const;
    [[nodiscard]] LayerType const* findLayerType(std::string const& id) const;
    [[nodiscard]] RuleLayer const* findRuleLayer(std::string const& id) const;
    [[nodiscard]] RuleBuilding const* findRuleBuilding(std::string const& id) const;
    [[nodiscard]] RuleZone const* findRuleZone(std::string const& id) const;
    [[nodiscard]] ZoneType const* findZoneType(std::string const& id) const;

    //--------------------------------------------------------------------------
    //! \brief Declare an empty definition for the parser to fill in.
    //!
    //! A name already taken returns nullptr. Duplicate names are errors, not
    //! silent overwrites.
    //!
    //! \param[in] id the name to declare.
    //! \return the new definition, or nullptr when the name is taken.
    //--------------------------------------------------------------------------
    Resource* addResource(std::string const& id);
    PathType* addPathType(std::string const& id);
    SegmentType* addSegmentType(std::string const& id);
    AgentType* addAgentType(std::string const& id);
    BuildingType* addBuildingType(std::string const& id);
    LayerType* addLayerType(std::string const& id);
    RuleLayer* addRuleLayer(std::string const& id);
    RuleBuilding* addRuleBuilding(std::string const& id);
    RuleZone* addRuleZone(std::string const& id);
    ZoneType* addZoneType(std::string const& id);

    //--------------------------------------------------------------------------
    //! \brief Take ownership of a command and return a pointer for a rule.
    //!
    //! Commands have no name and cannot be looked up. They live inside rules.
    //! They are stored here so they live as long as the ruleset and rules can
    //! hold raw pointers.
    //!
    //! \param[in] command the command to own.
    //! \return a pointer valid until clear().
    //--------------------------------------------------------------------------
    IRuleCommand* own(std::unique_ptr<IRuleCommand> command);

    //--------------------------------------------------------------------------
    //! \brief Same as own() for a value a command reads or writes.
    //! \param[in] value the value to own.
    //! \return a pointer valid until clear().
    //--------------------------------------------------------------------------
    IRuleValue* own(std::unique_ptr<IRuleValue> value);

    //--------------------------------------------------------------------------
    //! \brief List what the script declared, by name. Used by the editor for
    //! placement choices and by the debugger.
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
    [[nodiscard]] Catalog<BuildingType> const& getBuildingTypes() const
    {
        return m_buildingTypes;
    }
    [[nodiscard]] Catalog<LayerType> const& getLayerTypes() const
    {
        return m_layerTypes;
    }
    [[nodiscard]] Catalog<RuleLayer> const& getRuleLayers() const
    {
        return m_ruleLayers;
    }
    [[nodiscard]] Catalog<RuleBuilding> const& getRuleBuildings() const
    {
        return m_ruleBuildings;
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
    //! \brief Remove the whole ruleset.
    //!
    //! \note Safe only when nothing refers to it. Destroy all cities first.
    //! Calling this on a live world leaves dangling references.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \return true when the script declared nothing. This happens for
    //! an unparsed or failed ruleset.
    //--------------------------------------------------------------------------
    [[nodiscard]] bool isEmpty() const;

private:

    //! \brief Resource types by name: the resource names everything else uses.
    Catalog<Resource> m_resources;
    //! \brief Network types by name: roads, rails, pipes.
    Catalog<PathType> m_pathTypes;
    //! \brief Segment types by name, with speed and capacity.
    Catalog<SegmentType> m_segmentTypes;
    //! \brief Agent types by name, with speed and colour.
    Catalog<AgentType> m_agentTypes;
    //! \brief Building types by name, with rules and capacities.
    Catalog<BuildingType> m_buildingTypes;
    //! \brief Layer types by name, with cap and rules.
    Catalog<LayerType> m_layerTypes;
    //! \brief Layer rules by name.
    Catalog<RuleLayer> m_ruleLayers;
    //! \brief Building rules by name.
    Catalog<RuleBuilding> m_ruleBuildings;
    //! \brief Zone rules by name.
    Catalog<RuleZone> m_ruleZones;
    //! \brief Zone types by name, with rules.
    Catalog<ZoneType> m_zoneTypes;
    //! \brief Commands for all rules. Stored here so rules can share them and
    //! keep raw pointers.
    std::vector<std::unique_ptr<IRuleCommand>> m_commands;
    //! \brief Values that commands read and write. Same reason as m_commands.
    std::vector<std::unique_ptr<IRuleValue>> m_values;
};

} // namespace ogb

#endif
