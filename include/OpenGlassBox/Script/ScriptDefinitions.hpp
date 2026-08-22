//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SCRIPT_DEFINITIONS_HPP
#  define OPEN_GLASSBOX_SCRIPT_DEFINITIONS_HPP

#  include "OpenGlassBox/Rule.hpp"
#  include "OpenGlassBox/RuleCommand.hpp"
#  include "OpenGlassBox/RuleValue.hpp"
#  include "OpenGlassBox/Types.hpp"
#  include <map>
#  include <memory>
#  include <stdexcept>
#  include <string>
#  include <vector>

namespace ogb {

//==============================================================================
//! \brief Everything a simulation script defines: the types the entities refer
//! to, and the rules and commands they run.
//!
//! This is the owner. The entities of the simulation hold const references into
//! it and the rules hold raw pointers to their commands, so nothing here may be
//! destroyed while a City is alive, and clear() is only safe on a world that
//! has already been thrown away.
//==============================================================================
class ScriptDefinitions
{
public:

    //--------------------------------------------------------------------------
    //! \brief Look up a type by name, throwing when it is unknown.
    //! \throw std::out_of_range if the name was never defined.
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
    //! \brief Look up a type by name, returning nullptr when it is unknown.
    //! This is what the parser uses, since an unknown name is an error it wants
    //! to report with a position rather than an exception.
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
    //! \brief Create a type, or return nullptr when the name is already taken.
    //! A duplicate is an error rather than a silent overwrite, which is what
    //! used to happen and which made a typo very hard to notice.
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
    //! \brief Take ownership of a rule command and of the value it acts on. The
    //! rules only ever hold raw pointers to them.
    //--------------------------------------------------------------------------
    IRuleCommand* own(std::unique_ptr<IRuleCommand> command);
    IRuleValue* own(std::unique_ptr<IRuleValue> value);

    //--------------------------------------------------------------------------
    //! \brief Enumerate the types, for the editor and the debugger.
    //--------------------------------------------------------------------------
    std::map<std::string, std::unique_ptr<Resource>> const& resources() const
    {
        return m_resources;
    }
    std::map<std::string, std::unique_ptr<PathType>> const& pathTypes() const
    {
        return m_pathTypes;
    }
    std::map<std::string, std::unique_ptr<WayType>> const& wayTypes() const
    {
        return m_wayTypes;
    }
    std::map<std::string, std::unique_ptr<AgentType>> const& agentTypes() const
    {
        return m_agentTypes;
    }
    std::map<std::string, std::unique_ptr<UnitType>> const& unitTypes() const
    {
        return m_unitTypes;
    }
    std::map<std::string, std::unique_ptr<MapType>> const& mapTypes() const
    {
        return m_mapTypes;
    }
    std::map<std::string, std::unique_ptr<RuleMap>> const& ruleMaps() const
    {
        return m_ruleMaps;
    }
    std::map<std::string, std::unique_ptr<RuleUnit>> const& ruleUnits() const
    {
        return m_ruleUnits;
    }
    std::map<std::string, std::unique_ptr<RuleArea>> const& ruleAreas() const
    {
        return m_ruleAreas;
    }
    std::map<std::string, std::unique_ptr<AreaType>> const& areaTypes() const
    {
        return m_areaTypes;
    }

    //--------------------------------------------------------------------------
    //! \brief Drop everything. Only call it once no City refers to it.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief Whether anything at all was defined.
    //--------------------------------------------------------------------------
    bool empty() const;

private:

    std::map<std::string, std::unique_ptr<Resource>> m_resources;
    std::map<std::string, std::unique_ptr<PathType>> m_pathTypes;
    std::map<std::string, std::unique_ptr<WayType>> m_wayTypes;
    std::map<std::string, std::unique_ptr<AgentType>> m_agentTypes;
    std::map<std::string, std::unique_ptr<UnitType>> m_unitTypes;
    std::map<std::string, std::unique_ptr<MapType>> m_mapTypes;
    std::map<std::string, std::unique_ptr<RuleMap>> m_ruleMaps;
    std::map<std::string, std::unique_ptr<RuleUnit>> m_ruleUnits;
    std::map<std::string, std::unique_ptr<RuleArea>> m_ruleAreas;
    std::map<std::string, std::unique_ptr<AreaType>> m_areaTypes;

    //! \brief Commands and values live as long as the rules pointing at them.
    std::vector<std::unique_ptr<IRuleCommand>> m_commands;
    std::vector<std::unique_ptr<IRuleValue>> m_values;
};

} // namespace ogb

#endif
