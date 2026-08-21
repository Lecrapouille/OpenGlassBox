//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Script/ScriptDefinitions.hpp"

// -----------------------------------------------------------------------------
//! \brief Look up a name, throwing a message that says what was missing rather
//! than the bare "map::at" the standard library would give.
// -----------------------------------------------------------------------------
template<class T>
static T const& lookup(std::map<std::string, std::unique_ptr<T>> const& container,
                       std::string const& id, char const* what)
{
    auto const it = container.find(id);
    if (it == container.end())
    {
        throw std::out_of_range(std::string("Unknown ") + what + " '" + id + "'");
    }

    return *it->second;
}

// -----------------------------------------------------------------------------
template<class T>
static T* search(std::map<std::string, std::unique_ptr<T>>& container,
                 std::string const& id)
{
    auto const it = container.find(id);
    return (it == container.end()) ? nullptr : it->second.get();
}

// -----------------------------------------------------------------------------
//! \brief Create a named entry, or return nullptr when the name is taken.
// -----------------------------------------------------------------------------
template<class T, class... Args>
static T* create(std::map<std::string, std::unique_ptr<T>>& container,
                 std::string const& id, Args&&... args)
{
    if (container.find(id) != container.end())
        return nullptr;

    auto owned = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    T* raw = owned.get();
    container[id] = std::move(owned);

    return raw;
}

// -----------------------------------------------------------------------------
Resource const& ScriptDefinitions::getResource(std::string const& id) const
{
    return lookup(m_resources, id, "resource");
}

PathType const& ScriptDefinitions::getPathType(std::string const& id) const
{
    return lookup(m_pathTypes, id, "path type");
}

WayType const& ScriptDefinitions::getWayType(std::string const& id) const
{
    return lookup(m_wayTypes, id, "segment type");
}

AgentType const& ScriptDefinitions::getAgentType(std::string const& id) const
{
    return lookup(m_agentTypes, id, "agent type");
}

UnitType const& ScriptDefinitions::getUnitType(std::string const& id) const
{
    return lookup(m_unitTypes, id, "unit type");
}

MapType const& ScriptDefinitions::getMapType(std::string const& id) const
{
    return lookup(m_mapTypes, id, "map type");
}

RuleMap const& ScriptDefinitions::getRuleMap(std::string const& id) const
{
    return lookup(m_ruleMaps, id, "map rule");
}

RuleUnit const& ScriptDefinitions::getRuleUnit(std::string const& id) const
{
    return lookup(m_ruleUnits, id, "unit rule");
}

RuleArea const& ScriptDefinitions::getRuleArea(std::string const& id) const
{
    return lookup(m_ruleAreas, id, "area rule");
}

AreaType const& ScriptDefinitions::getAreaType(std::string const& id) const
{
    return lookup(m_areaTypes, id, "area type");
}

// -----------------------------------------------------------------------------
Resource* ScriptDefinitions::findResource(std::string const& id)
{
    return search(m_resources, id);
}

PathType* ScriptDefinitions::findPathType(std::string const& id)
{
    return search(m_pathTypes, id);
}

WayType* ScriptDefinitions::findWayType(std::string const& id)
{
    return search(m_wayTypes, id);
}

AgentType* ScriptDefinitions::findAgentType(std::string const& id)
{
    return search(m_agentTypes, id);
}

UnitType* ScriptDefinitions::findUnitType(std::string const& id)
{
    return search(m_unitTypes, id);
}

MapType* ScriptDefinitions::findMapType(std::string const& id)
{
    return search(m_mapTypes, id);
}

RuleMap* ScriptDefinitions::findRuleMap(std::string const& id)
{
    return search(m_ruleMaps, id);
}

RuleUnit* ScriptDefinitions::findRuleUnit(std::string const& id)
{
    return search(m_ruleUnits, id);
}

RuleArea* ScriptDefinitions::findRuleArea(std::string const& id)
{
    return search(m_ruleAreas, id);
}

AreaType* ScriptDefinitions::findAreaType(std::string const& id)
{
    return search(m_areaTypes, id);
}

// -----------------------------------------------------------------------------
Resource* ScriptDefinitions::addResource(std::string const& id)
{
    return create(m_resources, id, id);
}

PathType* ScriptDefinitions::addPathType(std::string const& id)
{
    return create(m_pathTypes, id, id);
}

WayType* ScriptDefinitions::addWayType(std::string const& id)
{
    return create(m_wayTypes, id, id);
}

AgentType* ScriptDefinitions::addAgentType(std::string const& id)
{
    return create(m_agentTypes, id, id);
}

UnitType* ScriptDefinitions::addUnitType(std::string const& id)
{
    return create(m_unitTypes, id, id);
}

MapType* ScriptDefinitions::addMapType(std::string const& id)
{
    return create(m_mapTypes, id, id);
}

// -----------------------------------------------------------------------------
RuleMap* ScriptDefinitions::addRuleMap(std::string const& id)
{
    // Born empty: the second pass gives it its rate and its commands, once
    // every name it may refer to has been declared.
    return create(m_ruleMaps, id, RuleMapType(id));
}

// -----------------------------------------------------------------------------
RuleUnit* ScriptDefinitions::addRuleUnit(std::string const& id)
{
    return create(m_ruleUnits, id, RuleUnitType(id));
}

RuleArea* ScriptDefinitions::addRuleArea(std::string const& id)
{
    return create(m_ruleAreas, id, RuleAreaType(id));
}

AreaType* ScriptDefinitions::addAreaType(std::string const& id)
{
    return create(m_areaTypes, id, id);
}

// -----------------------------------------------------------------------------
IRuleCommand* ScriptDefinitions::own(std::unique_ptr<IRuleCommand> command)
{
    IRuleCommand* raw = command.get();
    m_commands.push_back(std::move(command));
    return raw;
}

// -----------------------------------------------------------------------------
IRuleValue* ScriptDefinitions::own(std::unique_ptr<IRuleValue> value)
{
    IRuleValue* raw = value.get();
    m_values.push_back(std::move(value));
    return raw;
}

// -----------------------------------------------------------------------------
void ScriptDefinitions::clear()
{
    // The rules point at the commands and the commands at the values, so unwind
    // in that order: rules first, then commands, then what they read.
    m_mapTypes.clear();
    m_unitTypes.clear();
    m_ruleMaps.clear();
    m_ruleUnits.clear();
    m_ruleAreas.clear();
    m_areaTypes.clear();
    m_commands.clear();
    m_values.clear();
    m_agentTypes.clear();
    m_wayTypes.clear();
    m_pathTypes.clear();
    m_resources.clear();
}

// -----------------------------------------------------------------------------
bool ScriptDefinitions::empty() const
{
    return m_resources.empty() && m_pathTypes.empty() && m_wayTypes.empty() &&
           m_agentTypes.empty() && m_unitTypes.empty() && m_mapTypes.empty() &&
           m_ruleMaps.empty() && m_ruleUnits.empty() && m_ruleAreas.empty() &&
           m_areaTypes.empty();
}
