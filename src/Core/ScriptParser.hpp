//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SCRIPT_HPP
#  define OPEN_GLASSBOX_SCRIPT_HPP

#  include "Core/City.hpp"
#  include "Core/RuleValue.hpp"
#  include "Core/RuleCommand.hpp"
#  include <fstream>

//==============================================================================
//! \brief Parse a simulation script and store internally all types (Unit,
//! Resource, Map, Path) and simulation rules.
//==============================================================================
class Script
{
public:

    //--------------------------------------------------------------------------
    //! \brief Dummy constructor: do nothing
    //--------------------------------------------------------------------------
    Script() = default;

    //--------------------------------------------------------------------------
    //! \brief Parse the simulation file and fill its internal states: collection
    //! of xxxType which defines the different of object the simulation will play
    //! (Path, Agent, Unit, Maps ...), collections of Rules and Commands.
    //! \return true in case of success.
    //--------------------------------------------------------------------------
    bool parse(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Return true if parse() ended with success.
    //--------------------------------------------------------------------------
    operator bool() const { return m_success; }

    //--------------------------------------------------------------------------
    //! \brief Search the Resource given by its identifier and return the const
    //! reference to the Resource given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    Resource const& getResource(std::string const& id) const
    {
        return getT<Resource>(m_resources, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the PathType given by its identifier and return the const
    //! reference to the PathType given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    PathType const& getPathType(std::string const& id) const
    {
        return getT<PathType>(m_pathTypes, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the WayType given by its identifier and return the const
    //! reference to the WayType given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    WayType const& getWayType(std::string const& id) const
    {
        return getT<WayType>(m_segmentTypes, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the AgentType given by its identifier and return the const
    //! reference to the AgentType given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    AgentType const& getAgentType(std::string const& id) const
    {
        return getT<AgentType>(m_agentTypes, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the RuleMap given by its identifier and return the const
    //! reference to the RuleMap given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    RuleMap const& getRuleMap(std::string const& id) const
    {
        return getT<RuleMap>(m_ruleMaps, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the RuleUnit given by its identifier and return the const
    //! reference to the RuleUnit given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    RuleUnit const& getRuleUnit(std::string const& id) const
    {
        return getT<RuleUnit>(m_ruleUnits, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the UnitType given by its identifier and return the const
    //! reference to the UnitType given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    UnitType const& getUnitType(std::string const& id) const
    {
        return getT<UnitType>(m_unitTypes, id);
    }

    //--------------------------------------------------------------------------
    //! \brief Search the MapType given by its identifier and return the const
    //! reference to the MapType given. Throw an exception if not found.
    //--------------------------------------------------------------------------
    MapType const& getMapType(std::string const& id) const
    {
        return getT<MapType>(m_mapTypes, id);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Split the script file into tokens. Return the reference of the
    //! last token.
    //--------------------------------------------------------------------------
    std::string const& nextToken();

    //--------------------------------------------------------------------------
    //! \brief Entry point method for parsing the script.
    //--------------------------------------------------------------------------
    void parseScript();
    void parseResources();
    void parseResource();
    void parseResourcesArray(Resources& resources);
    void parseCapacitiesArray(Resources& resources);
    void parseRules();
    void parseMaps();
    void parseMap();
    void parsePaths();
    void parsePath();
    void parseWays();
    void parseWay();
    void parseAgents();
    void parseAgent();
    void parseUnits();
    void parseUnit();
    void parseRuleMap();
    void parseRuleUnit();
    void parseStringArray(std::vector<std::string>& vec);
    void parseRuleMapArray(std::vector<RuleMap*>& rules);
    void parseRuleUnitArray(std::vector<RuleUnit*>& rules);
    IRuleCommand* parseCommand(std::string const& token);

    //--------------------------------------------------------------------------
    //! \brief Helper for public methods getXxxType()
    //--------------------------------------------------------------------------
    template<class T>
    T const& getT(std::map<std::string, T*> const& map, std::string const& id) const
    {
        return *(map.at(id));
    }

private:

    std::map<std::string, Resource*> m_resources;
    std::map<std::string, PathType*> m_pathTypes;
    std::map<std::string, WayType*> m_segmentTypes;
    std::map<std::string, AgentType*> m_agentTypes;
    std::map<std::string, RuleMap*> m_ruleMaps;
    std::map<std::string, RuleUnit*> m_ruleUnits;
    std::map<std::string, UnitType*> m_unitTypes;
    std::map<std::string, MapType*> m_mapTypes;
    std::ifstream m_file;
    std::string m_token;
    bool m_success = false;
};

#endif
