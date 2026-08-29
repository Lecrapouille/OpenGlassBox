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
namespace ogb
{

template<class T>
static T const&
lookup(Catalog<T> const& container, std::string const& id, char const* what)
{
    auto const it = container.find(id);
    if (it == container.end())
    {
        throw std::out_of_range(std::string("Unknown ") + what + " '" + id +
                                "'");
    }

    return *it->second;
}

// -----------------------------------------------------------------------------
template<class T>
static T* search(Catalog<T>& container, std::string const& id)
{
    auto const it = container.find(id);
    return (it == container.end()) ? nullptr : it->second.get();
}

// -----------------------------------------------------------------------------
template<class T>
static T const* search(Catalog<T> const& container, std::string const& id)
{
    auto const it = container.find(id);
    return (it == container.end()) ? nullptr : it->second.get();
}

// -----------------------------------------------------------------------------
//! \brief Create a named entry, or return nullptr when the name is taken.
// -----------------------------------------------------------------------------
template<class T, class... Args>
static T* create(Catalog<T>& container, std::string const& id, Args&&... args)
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

SegmentType const& ScriptDefinitions::getSegmentType(std::string const& id) const
{
    return lookup(m_segmentTypes, id, "segment type");
}

AgentType const& ScriptDefinitions::getAgentType(std::string const& id) const
{
    return lookup(m_agentTypes, id, "agent type");
}

UnitType const& ScriptDefinitions::getUnitType(std::string const& id) const
{
    return lookup(m_unitTypes, id, "unit type");
}

LayerType const& ScriptDefinitions::getLayerType(std::string const& id) const
{
    return lookup(m_layerTypes, id, "layer type");
}

RuleLayer const& ScriptDefinitions::getRuleLayer(std::string const& id) const
{
    return lookup(m_ruleLayers, id, "layer rule");
}

RuleUnit const& ScriptDefinitions::getRuleUnit(std::string const& id) const
{
    return lookup(m_ruleUnits, id, "unit rule");
}

RuleZone const& ScriptDefinitions::getRuleZone(std::string const& id) const
{
    return lookup(m_ruleZones, id, "zone rule");
}

ZoneType const& ScriptDefinitions::getZoneType(std::string const& id) const
{
    return lookup(m_zoneTypes, id, "zone type");
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

SegmentType* ScriptDefinitions::findSegmentType(std::string const& id)
{
    return search(m_segmentTypes, id);
}

AgentType* ScriptDefinitions::findAgentType(std::string const& id)
{
    return search(m_agentTypes, id);
}

UnitType* ScriptDefinitions::findUnitType(std::string const& id)
{
    return search(m_unitTypes, id);
}

LayerType* ScriptDefinitions::findLayerType(std::string const& id)
{
    return search(m_layerTypes, id);
}

RuleLayer* ScriptDefinitions::findRuleLayer(std::string const& id)
{
    return search(m_ruleLayers, id);
}

RuleUnit* ScriptDefinitions::findRuleUnit(std::string const& id)
{
    return search(m_ruleUnits, id);
}

RuleZone* ScriptDefinitions::findRuleZone(std::string const& id)
{
    return search(m_ruleZones, id);
}

ZoneType* ScriptDefinitions::findZoneType(std::string const& id)
{
    return search(m_zoneTypes, id);
}

// -----------------------------------------------------------------------------
Resource const* ScriptDefinitions::findResource(std::string const& id) const
{
    return search(m_resources, id);
}

PathType const* ScriptDefinitions::findPathType(std::string const& id) const
{
    return search(m_pathTypes, id);
}

SegmentType const* ScriptDefinitions::findSegmentType(std::string const& id) const
{
    return search(m_segmentTypes, id);
}

AgentType const* ScriptDefinitions::findAgentType(std::string const& id) const
{
    return search(m_agentTypes, id);
}

UnitType const* ScriptDefinitions::findUnitType(std::string const& id) const
{
    return search(m_unitTypes, id);
}

LayerType const* ScriptDefinitions::findLayerType(std::string const& id) const
{
    return search(m_layerTypes, id);
}

RuleLayer const* ScriptDefinitions::findRuleLayer(std::string const& id) const
{
    return search(m_ruleLayers, id);
}

RuleUnit const* ScriptDefinitions::findRuleUnit(std::string const& id) const
{
    return search(m_ruleUnits, id);
}

RuleZone const* ScriptDefinitions::findRuleZone(std::string const& id) const
{
    return search(m_ruleZones, id);
}

ZoneType const* ScriptDefinitions::findZoneType(std::string const& id) const
{
    return search(m_zoneTypes, id);
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

SegmentType* ScriptDefinitions::addSegmentType(std::string const& id)
{
    return create(m_segmentTypes, id, id);
}

AgentType* ScriptDefinitions::addAgentType(std::string const& id)
{
    return create(m_agentTypes, id, id);
}

UnitType* ScriptDefinitions::addUnitType(std::string const& id)
{
    return create(m_unitTypes, id, id);
}

LayerType* ScriptDefinitions::addLayerType(std::string const& id)
{
    return create(m_layerTypes, id, id);
}

// -----------------------------------------------------------------------------
RuleLayer* ScriptDefinitions::addRuleLayer(std::string const& id)
{
    // Born empty: the second pass gives it its rate and its commands, once
    // every name it may refer to has been declared.
    return create(m_ruleLayers, id, RuleLayerType(id));
}

// -----------------------------------------------------------------------------
RuleUnit* ScriptDefinitions::addRuleUnit(std::string const& id)
{
    return create(m_ruleUnits, id, RuleUnitType(id));
}

RuleZone* ScriptDefinitions::addRuleZone(std::string const& id)
{
    return create(m_ruleZones, id, RuleZoneType(id));
}

ZoneType* ScriptDefinitions::addZoneType(std::string const& id)
{
    return create(m_zoneTypes, id, id);
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
    m_layerTypes.clear();
    m_unitTypes.clear();
    m_ruleLayers.clear();
    m_ruleUnits.clear();
    m_ruleZones.clear();
    m_zoneTypes.clear();
    m_commands.clear();
    m_values.clear();
    m_agentTypes.clear();
    m_segmentTypes.clear();
    m_pathTypes.clear();
    m_resources.clear();
}

// -----------------------------------------------------------------------------
bool ScriptDefinitions::isEmpty() const
{
    return m_resources.empty() && m_pathTypes.empty() && m_segmentTypes.empty() &&
           m_agentTypes.empty() && m_unitTypes.empty() && m_layerTypes.empty() &&
           m_ruleLayers.empty() && m_ruleUnits.empty() && m_ruleZones.empty() &&
           m_zoneTypes.empty();
}

} // namespace ogb
