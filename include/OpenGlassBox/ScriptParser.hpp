//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file ScriptParser.hpp
//! \brief Script holder that loads simulation definitions through pluggable
//! parsers.

#ifndef OPEN_GLASSBOX_SCRIPT_HPP
#define OPEN_GLASSBOX_SCRIPT_HPP

#include "OpenGlassBox/Script/IScriptParser.hpp"

namespace ogb
{

//==============================================================================
//! \brief Holds what a simulation script defined and knows how to load one.
//!
//! The parsing itself lives behind IScriptParser, picked from the extension of
//! the file, so that another language can be plugged in without the engine
//! knowing. This class is what the rest of the engine talks to: it owns the
//! definitions and forwards the lookups.
//==============================================================================
class Script
{
public:

    Script() = default;
    virtual ~Script() = default;

    //--------------------------------------------------------------------------
    //! \brief Load a simulation script, replacing whatever was loaded before.
    //!
    //! On failure the previous definitions are kept, so a bad reload leaves a
    //! running simulation alone rather than emptying it.
    //!
    //! \return true in case of success. See errors() for what went wrong.
    //--------------------------------------------------------------------------
    bool parse(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Load a script held in memory, reported under the given name.
    //--------------------------------------------------------------------------
    bool parseString(std::string const& source,
                     std::string const& name = "<string>");

    //--------------------------------------------------------------------------
    //! \brief Everything found wrong by the last load, in the order it was
    //! found. Empty after a successful load.
    //--------------------------------------------------------------------------
    std::vector<ParseError> const& errors() const
    {
        return m_errors;
    }

    //--------------------------------------------------------------------------
    //! \brief The errors of the last load, one per line, ready to be shown.
    //--------------------------------------------------------------------------
    std::string formatErrors() const;

    //--------------------------------------------------------------------------
    //! \brief What the script defined.
    //--------------------------------------------------------------------------
    ScriptDefinitions const& definitions() const
    {
        return m_definitions;
    }
    ScriptDefinitions& definitions()
    {
        return m_definitions;
    }

    //--------------------------------------------------------------------------
    //! \brief Search a type by its identifier.
    //! \throw std::out_of_range if the identifier was never defined.
    //--------------------------------------------------------------------------
    Resource const& getResource(std::string const& id) const
    {
        return m_definitions.getResource(id);
    }
    PathType const& getPathType(std::string const& id) const
    {
        return m_definitions.getPathType(id);
    }
    WayType const& getWayType(std::string const& id) const
    {
        return m_definitions.getWayType(id);
    }
    AgentType const& getAgentType(std::string const& id) const
    {
        return m_definitions.getAgentType(id);
    }
    RuleMap const& getRuleMap(std::string const& id) const
    {
        return m_definitions.getRuleMap(id);
    }
    RuleUnit const& getRuleUnit(std::string const& id) const
    {
        return m_definitions.getRuleUnit(id);
    }
    UnitType const& getUnitType(std::string const& id) const
    {
        return m_definitions.getUnitType(id);
    }
    MapType const& getMapType(std::string const& id) const
    {
        return m_definitions.getMapType(id);
    }
    AreaType const& getAreaType(std::string const& id) const
    {
        return m_definitions.getAreaType(id);
    }
    RuleArea const& getRuleArea(std::string const& id) const
    {
        return m_definitions.getRuleArea(id);
    }

    //--------------------------------------------------------------------------
    //! \brief Enumerate the types the script defined. The editor needs them to
    //! offer a choice of road, building or resource to place.
    //--------------------------------------------------------------------------
    std::map<std::string, std::unique_ptr<PathType>> const& pathTypes() const
    {
        return m_definitions.pathTypes();
    }
    std::map<std::string, std::unique_ptr<WayType>> const& wayTypes() const
    {
        return m_definitions.wayTypes();
    }
    std::map<std::string, std::unique_ptr<UnitType>> const& unitTypes() const
    {
        return m_definitions.unitTypes();
    }
    std::map<std::string, std::unique_ptr<MapType>> const& mapTypes() const
    {
        return m_definitions.mapTypes();
    }
    std::map<std::string, std::unique_ptr<AgentType>> const& agentTypes() const
    {
        return m_definitions.agentTypes();
    }
    std::map<std::string, std::unique_ptr<AreaType>> const& areaTypes() const
    {
        return m_definitions.areaTypes();
    }

private:

    ScriptDefinitions m_definitions;
    std::vector<ParseError> m_errors;
};

} // namespace ogb

#endif
