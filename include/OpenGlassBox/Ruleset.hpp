//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Ruleset.hpp
//! \brief What a script declares, and how to load it.

#ifndef OPEN_GLASSBOX_RULESET_HPP
#define OPEN_GLASSBOX_RULESET_HPP

#include "OpenGlassBox/Script/IScriptParser.hpp"

namespace ogb
{

//==============================================================================
//! \brief What a script declares: types and Rules for a City.
//!
//! Parsing uses IScriptParser. The file extension picks the parser.
//! Another language can plug in without changing the engine.
//! The rest of the engine talks to this class.
//! It owns definitions and answers lookups.
//!
//! A Simulation owns one Ruleset and shares it read-only.
//! Load a script through Simulation::loadScriptFile(), not here.
//! The Simulation must know when the Ruleset changes.
//!
//! Example:
//! \code
//! ogb::BuildingType const& home = simulation.getRuleset().getBuildingType("Home");
//! city.addBuilding(home, node);
//! \endcode
//==============================================================================
class Ruleset
{
public:

    //--------------------------------------------------------------------------
    //! \brief Load a script from a file. Replaces the previous load.
    //!
    //! On failure, keep the previous definitions.
    //! A bad reload does not empty a running Simulation.
    //!
    //! \param[in] filename script file to read.
    //! \return true on success. See getErrors() for details.
    //--------------------------------------------------------------------------
    bool loadFile(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Load a script from a string.
    //! \param[in] source script text.
    //! \param[in] name name used in error messages when there is no file path.
    //! \return true on success.
    //--------------------------------------------------------------------------
    bool loadString(std::string const& source,
                    std::string const& name = "<string>");

    //--------------------------------------------------------------------------
    //! \return errors from the last load, in order found.
    //! Empty after a successful load.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::vector<ParseError> const& getErrors() const
    {
        return m_errors;
    }

    //--------------------------------------------------------------------------
    //! \return errors from the last load, one per line, ready to show.
    //--------------------------------------------------------------------------
    std::string formatErrors() const;

    //--------------------------------------------------------------------------
    //! \return all definitions, for code that walks the catalogue.
    //--------------------------------------------------------------------------
    [[nodiscard]] ScriptDefinitions const& getDefinitions() const
    {
        return m_definitions;
    }

    //--------------------------------------------------------------------------
    //! \brief Find a type by name.
    //! \param[in] id name from the script.
    //! \return the type. It outlives every entity built from it.
    //! \throw std::out_of_range if the script never declared that name.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource const& getResource(std::string const& id) const
    {
        return m_definitions.getResource(id);
    }
    [[nodiscard]] PathType const& getPathType(std::string const& id) const
    {
        return m_definitions.getPathType(id);
    }
    [[nodiscard]] SegmentType const& getSegmentType(std::string const& id) const
    {
        return m_definitions.getSegmentType(id);
    }
    [[nodiscard]] AgentType const& getAgentType(std::string const& id) const
    {
        return m_definitions.getAgentType(id);
    }
    [[nodiscard]] BuildingType const& getBuildingType(std::string const& id) const
    {
        return m_definitions.getBuildingType(id);
    }
    [[nodiscard]] LayerType const& getLayerType(std::string const& id) const
    {
        return m_definitions.getLayerType(id);
    }
    [[nodiscard]] ZoneType const& getZoneType(std::string const& id) const
    {
        return m_definitions.getZoneType(id);
    }
    [[nodiscard]] RuleLayer const& getRuleLayer(std::string const& id) const
    {
        return m_definitions.getRuleLayer(id);
    }
    [[nodiscard]] RuleBuilding const& getRuleBuilding(std::string const& id) const
    {
        return m_definitions.getRuleBuilding(id);
    }
    [[nodiscard]] RuleZone const& getRuleZone(std::string const& id) const
    {
        return m_definitions.getRuleZone(id);
    }

    //--------------------------------------------------------------------------
    //! \brief Find a type by name without throwing.
    //! Use this to test if a name exists.
    //! Use getXxx() when the name must exist.
    //! \param[in] id name from the script.
    //! \return the type, or nullptr if the script never declared it.
    //--------------------------------------------------------------------------
    [[nodiscard]] Resource const* findResource(std::string const& id) const
    {
        return m_definitions.findResource(id);
    }
    [[nodiscard]] PathType const* findPathType(std::string const& id) const
    {
        return m_definitions.findPathType(id);
    }
    [[nodiscard]] SegmentType const* findSegmentType(std::string const& id) const
    {
        return m_definitions.findSegmentType(id);
    }
    [[nodiscard]] AgentType const* findAgentType(std::string const& id) const
    {
        return m_definitions.findAgentType(id);
    }
    [[nodiscard]] BuildingType const* findBuildingType(std::string const& id) const
    {
        return m_definitions.findBuildingType(id);
    }
    [[nodiscard]] LayerType const* findLayerType(std::string const& id) const
    {
        return m_definitions.findLayerType(id);
    }
    [[nodiscard]] ZoneType const* findZoneType(std::string const& id) const
    {
        return m_definitions.findZoneType(id);
    }

    //--------------------------------------------------------------------------
    //! \brief List declared types by name.
    //! The editor uses these to offer roads, Buildings and Zones to place.
    //--------------------------------------------------------------------------
    [[nodiscard]] Catalog<PathType> const& getPathTypes() const
    {
        return m_definitions.getPathTypes();
    }
    [[nodiscard]] Catalog<SegmentType> const& getSegmentTypes() const
    {
        return m_definitions.getSegmentTypes();
    }
    [[nodiscard]] Catalog<BuildingType> const& getBuildingTypes() const
    {
        return m_definitions.getBuildingTypes();
    }
    [[nodiscard]] Catalog<LayerType> const& getLayerTypes() const
    {
        return m_definitions.getLayerTypes();
    }
    [[nodiscard]] Catalog<AgentType> const& getAgentTypes() const
    {
        return m_definitions.getAgentTypes();
    }
    [[nodiscard]] Catalog<ZoneType> const& getZoneTypes() const
    {
        return m_definitions.getZoneTypes();
    }
    [[nodiscard]] Catalog<RuleLayer> const& getRuleLayers() const
    {
        return m_definitions.getRuleLayers();
    }
    [[nodiscard]] Catalog<RuleBuilding> const& getRuleBuildings() const
    {
        return m_definitions.getRuleBuildings();
    }
    [[nodiscard]] Catalog<RuleZone> const& getRuleZones() const
    {
        return m_definitions.getRuleZones();
    }

private:

    //! \brief Result of the last successful load.
    ScriptDefinitions m_definitions;
    //! \brief Errors from the last load.
    std::vector<ParseError> m_errors;
};

} // namespace ogb

#endif
