//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Ruleset.hpp
//! \brief What a script declared, and how to load one.

#ifndef OPEN_GLASSBOX_RULESET_HPP
#define OPEN_GLASSBOX_RULESET_HPP

#include "OpenGlassBox/Script/IScriptParser.hpp"

namespace ogb
{

//==============================================================================
//! \brief What a script declared: the recipes a city is built from, and the
//! rules it runs.
//!
//! The parsing itself sits behind IScriptParser, picked from the extension of
//! the file, so another language can be plugged in without the engine knowing.
//! This class is what the rest of the engine talks to: it owns the definitions
//! and forwards the lookups.
//!
//! A Simulation owns one, and hands it out for reading only. Load a script
//! through Simulation::loadScriptFile() rather than here, so the simulation
//! knows its ruleset changed.
//!
//! Example:
//! \code
//! ogb::UnitType const& home = simulation.getRuleset().getUnitType("Home");
//! city.addUnit(home, node);
//! \endcode
//==============================================================================
class Ruleset
{
public:

    //--------------------------------------------------------------------------
    //! \brief Load a script from a file, replacing what was loaded before.
    //!
    //! On failure the previous definitions are kept, so a bad reload leaves a
    //! running simulation alone rather than emptying it.
    //!
    //! \param[in] filename the script to read.
    //! \return true on success. See getErrors() for what went wrong.
    //--------------------------------------------------------------------------
    bool loadFile(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Load a script held in memory.
    //! \param[in] source the script itself.
    //! \param[in] name what the errors should call it, there being no path.
    //! \return true on success.
    //--------------------------------------------------------------------------
    bool loadString(std::string const& source,
                    std::string const& name = "<string>");

    //--------------------------------------------------------------------------
    //! \brief \return everything found wrong by the last load, in the order it
    //! was found. Empty after a load that went well.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::vector<ParseError> const& getErrors() const
    {
        return m_errors;
    }

    //--------------------------------------------------------------------------
    //! \brief \return the errors of the last load, one per line, ready to be
    //! shown.
    //--------------------------------------------------------------------------
    std::string formatErrors() const;

    //--------------------------------------------------------------------------
    //! \brief \return the whole catalogue, for code that walks it rather than
    //! looking a name up.
    //--------------------------------------------------------------------------
    [[nodiscard]] ScriptDefinitions const& getDefinitions() const
    {
        return m_definitions;
    }

    //--------------------------------------------------------------------------
    //! \brief Look up a recipe by name.
    //! \param[in] id the name the script gave it.
    //! \return the recipe, which outlives every entity built from it.
    //! \throw std::out_of_range when the script never declared that name.
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
    [[nodiscard]] UnitType const& getUnitType(std::string const& id) const
    {
        return m_definitions.getUnitType(id);
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
    [[nodiscard]] RuleUnit const& getRuleUnit(std::string const& id) const
    {
        return m_definitions.getRuleUnit(id);
    }
    [[nodiscard]] RuleZone const& getRuleZone(std::string const& id) const
    {
        return m_definitions.getRuleZone(id);
    }

    //--------------------------------------------------------------------------
    //! \brief Look a recipe up by name, without throwing. Use this to ask
    //! whether a name exists; getXxx() is for when it has to.
    //! \param[in] id the name the script would have given it.
    //! \return the recipe, or nullptr when the script never declared that name.
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
    [[nodiscard]] UnitType const* findUnitType(std::string const& id) const
    {
        return m_definitions.findUnitType(id);
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
    //! \brief List what the script declared, by name. The editor needs these to
    //! offer a choice of road, building or zone to place.
    //--------------------------------------------------------------------------
    [[nodiscard]] Catalog<PathType> const& getPathTypes() const
    {
        return m_definitions.getPathTypes();
    }
    [[nodiscard]] Catalog<SegmentType> const& getSegmentTypes() const
    {
        return m_definitions.getSegmentTypes();
    }
    [[nodiscard]] Catalog<UnitType> const& getUnitTypes() const
    {
        return m_definitions.getUnitTypes();
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
    [[nodiscard]] Catalog<RuleUnit> const& getRuleUnits() const
    {
        return m_definitions.getRuleUnits();
    }
    [[nodiscard]] Catalog<RuleZone> const& getRuleZones() const
    {
        return m_definitions.getRuleZones();
    }

private:

    //! \brief What the last successful load produced.
    ScriptDefinitions m_definitions;
    //! \brief What went wrong during the last load.
    std::vector<ParseError> m_errors;
};

} // namespace ogb

#endif
