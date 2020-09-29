#ifndef SCRIPT_HPP
#  define SCRIPT_HPP

#  include "Core/City.hpp"
#  include "Core/RuleValue.hpp"
#  include "Core/RuleCommand.hpp"
#  include <fstream>

class Script
{
public:

    Script(std::string const& filename);

    operator bool() const { return m_success; }

    Resource const& getResource(std::string const& id) const
    {
        return getT<Resource>(m_resources, id);
    }

    PathType const& getPathType(std::string const& id) const
    {
        return getT<PathType>(m_pathTypes, id);
    }

    WayType const& getWayType(std::string const& id) const
    {
        return getT<WayType>(m_segmentTypes, id);
    }

    AgentType const& getAgentType(std::string const& id) const
    {
        return getT<AgentType>(m_agentTypes, id);
    }

    RuleMap const& getRuleMap(std::string const& id) const
    {
        return getT<RuleMap>(m_ruleMaps, id);
    }

    RuleUnit const& getRuleUnit(std::string const& id) const
    {
        return getT<RuleUnit>(m_ruleUnits, id);
    }

    UnitType const& getUnitType(std::string const& id) const
    {
        return getT<UnitType>(m_unitTypes, id);
    }

    MapType const& getMapType(std::string const& id) const
    {
        return getT<MapType>(m_mapTypes, id);
    }

private:

    std::string const& nextToken();
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
    void parseRuleArray(std::vector<RuleMap*>& rules);
    IRuleCommand* parseCommand(std::string const& token);

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
    bool m_success;
};

#endif
