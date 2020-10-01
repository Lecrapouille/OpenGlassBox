//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/Script.hpp"
#include "Core/RuleCommand.hpp"
#include <iostream>
#include <stdexcept>
#include <cstring>

static uint32_t toUint(std::string const& word)
{
    return static_cast<uint32_t>(atoi(word.c_str()));
}

static uint32_t toColor(std::string const& word)
{
    return static_cast<uint32_t>(strtol(word.c_str(), NULL, 16));
}

static float toFloat(std::string const& word)
{
    return static_cast<float>(atoi(word.c_str()));
}

static bool toBool(std::string const& word)
{
    if (word == "true")
        return true;
    if (word == "false")
        return false;
    return !!toUint(word);
}

Script::Script(std::string const& filename)
    : m_file(filename)
{
    std::cout << "Parsing script '" << filename << "'" << std::endl;
    if (!m_file)
    {
        std::cerr << "Failed opening '" << filename << "' Reason '"
                  << strerror(errno) << "'" << std::endl;
        m_success = false;
        return ;
    }

    try
    {
        parseScript();
        m_success = true;
        std::cout << "  done" << std::endl;
    }
    catch (std::exception &e)
    {
        std::cerr << "Failed parsing script '"
                  << filename << "' at token '"
                  << m_token << "' Reason was '"
                  << e.what() << "'"
                  << std::endl;
        m_success = false;
    }
}

std::string const& Script::nextToken()
{
    if (m_file >> m_token)
    {
        // Uncomment for debug
        // std::cout << "I read '" << m_token << "'" << std::endl;
    }
    else
    {
        m_token.clear();
    }
    return m_token;
}

void Script::parseScript()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "resources")
            parseResources();
        else if (token == "rules")
            parseRules();
        else if (token == "maps")
            parseMaps();
        else if (token == "paths")
            parsePaths();
        else if (token == "segments")
            parseWays();
        else if (token == "agents")
            parseAgents();
        else if (token == "units")
            parseUnits();
        else if (token == "")
            return ;
        else
            throw std::runtime_error("parseScript()");
    }
}

void Script::parseResources()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "resource")
            parseResource();
        else
            throw std::runtime_error("parseResources()");
    }
}

void Script::parseResource()
{
    std::string const& name = nextToken();
    m_resources[name] = new Resource(name);
}

void Script::parseResourcesArray(Resources& resources)
{
    {
        std::string const& token = nextToken();
        if (token != "[")
            throw std::runtime_error("parseResourcesArray()");
    }

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "]")
            return ;

        Resource const& resource = getResource(token);
        uint32_t amount = toUint(nextToken());
        // FIXME should be setAmount
        resources.addResource(resource.type(), amount);
    }
}

void Script::parseCapacitiesArray(Resources& resources)
{
    {
        std::string const& token = nextToken();
        if (token != "[")
            throw std::runtime_error("parseCapacitiesArray()");
    }

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "]")
            return ;

        Resource const& resource = getResource(token);
        uint32_t capacity = toUint(nextToken());
        resources.setCapacity(resource.type(), capacity);
    }
}

void Script::parsePaths()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "path")
            parsePath();
        else
            throw std::runtime_error("parsePaths()");
    }
}

void Script::parsePath()
{
    PathType* path = new PathType(nextToken());
    m_pathTypes[path->name] = path;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
        {
            path->color = toColor(nextToken());
            return ;
        }
        else
        {
            throw std::runtime_error("parsePath()");
        }
    }
}

void Script::parseWays()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "segment")
            parseWay();
        else
            throw std::runtime_error("parseWays()");
    }
}

void Script::parseWay()
{
    WayType* seg = new WayType(nextToken());
    m_segmentTypes[seg->name] = seg;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
        {
            seg->color = toColor(nextToken());
            return ;
        }
        else
        {
            throw std::runtime_error("parseWay()");
        }
    }
}

void Script::parseAgents()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "agent")
            parseAgent();
        else
            throw std::runtime_error("parseAgents()");
    }
}

void Script::parseAgent()
{
    AgentType* agent = new AgentType(nextToken());
    m_agentTypes[agent->name] = agent;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
            agent->color = toColor(nextToken());
        else if (token == "speed")
        {
            agent->speed = toFloat(nextToken());
            return ;
        }
        else
            throw std::runtime_error("parseAgents()");
    }
}

void Script::parseRules()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "mapRule")
            parseRuleMap();
        else if (token == "unitRule")
            parseRuleUnit();
        else
            throw std::runtime_error("parseRules()");
    }
}

void Script::parseRuleMap()
{
    RuleMapType type(nextToken());

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
        {
            RuleMap* rule = new RuleMap(type);
            m_ruleMaps[rule->id()] = rule;
            return ;
        }
        else if (token == "rate")
            type.rate = toUint(nextToken());
        else if (token == "randomTiles")
            type.randomTiles = toBool(nextToken());
        else if (token == "randomTilesPercent")
        {
            type.randomTiles = true;
            type.randomTilesPercent = toUint(nextToken());
        }
        else
            type.commands.push_back(parseCommand(token));
    }
}

void Script::parseRuleUnit()
{
    RuleUnitType type(nextToken());

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
        {
            RuleUnit* rule = new RuleUnit(type);
            m_ruleUnits[rule->id()] = rule;
            return ;
        }
        else if (token == "rate")
            type.rate = toUint(nextToken());
        //else if (token == "onFail") // TODO
        //{}
        else
            type.commands.push_back(parseCommand(token));
    }
}

IRuleCommand* Script::parseCommand(std::string const& token)
{
    IRuleValue* target = nullptr;
    IRuleCommand* command = nullptr;

    if (token == "local")
    {
        Resource const& resource = getResource(nextToken());
        target = new RuleValueLocal(resource);
    }
    else if (token == "global")
    {
        Resource const& resource = getResource(nextToken());
        target = new RuleValueGlobal(resource);
    }
    else if (token == "map")
    {
        target = new RuleValueMap(nextToken());
    }
    else if (token == "agent")
    {
        std::string name = nextToken();
        std::string target;
        Resources resources;

        while (true)
        {
            std::string const& cmd = nextToken();
            if (cmd == "to")
            {
                target = nextToken();
            }
            else if (cmd == "add")
            {
                parseResourcesArray(resources);
                break ;
            }
            else
            {
                throw std::runtime_error("parseCommand()");
            }
        }

        command = new RuleCommandAgent(getAgentType(name),
                                       target, resources);
    }
    else
    {
        throw std::runtime_error("parseCommand()");
    }

    if (target != nullptr)
    {
        std::string const& cmd = nextToken();
        if (cmd == "add")
        {
            command = new RuleCommandAdd(*target, toUint(nextToken()));
        }
        else if (cmd == "remove")
        {
            command = new RuleCommandRemove(*target, toUint(nextToken()));
        }
        else if (cmd == "greater")
        {
            command = new RuleCommandTest(*target, RuleCommandTest::Comparison::GREATER,
                                          toUint(nextToken()));
        }
        else if (cmd == "less")
        {
            command = new RuleCommandTest(*target, RuleCommandTest::Comparison::LESS,
                                          toUint(nextToken()));
        }
        else if (cmd == "equals")
        {
            command = new RuleCommandTest(*target, RuleCommandTest::Comparison::EQUALS,
                                          toUint(nextToken()));
        }
        else
        {
            throw std::runtime_error("parseCommand()");
        }
    }

    return command;
}

void Script::parseMaps()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "map")
            parseMap();
        else
            throw std::runtime_error("parseMaps()");
    }
}

void Script::parseMap()
{
    MapType* map = new MapType(nextToken());
    m_mapTypes[map->name] = map;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
            map->color = toColor(nextToken());
        else if (token == "capacity")
            map->capacity = toUint(nextToken());
        else if (token == "rules")
        {
            parseRuleMapArray(map->rules);
            return ;
        }
    }
}

void Script::parseUnits()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "unit")
            parseUnit();
        else
            throw std::runtime_error("parseUnits()");
    }
}

void Script::parseUnit()
{
    UnitType* unit = new UnitType(nextToken());
    m_unitTypes[unit->name] = unit;

    Resources caps;
    Resources resources;
    std::vector<std::string> todo;
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
            unit->color = toColor(nextToken());
        else if (token == "mapRadius")
            unit->radius = toUint(nextToken());
        else if (token == "rules")
            parseRuleUnitArray(unit->rules);
        else if (token == "targets")
            parseStringArray(unit->targets);
        else if (token == "caps")
        {
            parseCapacitiesArray(caps);
            unit->resources.setCapacities(caps);
        }
        else if (token == "resources")
        {
            parseResourcesArray(resources);
            unit->resources.addResources(resources);
            return ;
        }
        else
            throw std::runtime_error("parseUnit()");
    }
}

void Script::parseStringArray(std::vector<std::string>& vec)
{
    {
        std::string const& token = nextToken();
        if (token != "[")
            throw std::runtime_error("parseStringArray()");
    }

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "]")
            return ;
        vec.push_back(token);
    }
}

void Script::parseRuleMapArray(std::vector<RuleMap*>& rules)
{
    {
        std::string const& token = nextToken();
        if (token != "[")
            throw std::runtime_error("parseRuleMapArray()");
    }

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "]")
            return ;
        rules.push_back(m_ruleMaps[token]);
    }
}

void Script::parseRuleUnitArray(std::vector<RuleUnit*>& rules)
{
    {
        std::string const& token = nextToken();
        if (token != "[")
            throw std::runtime_error("parseRuleUnitArray()");
    }

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "]")
            return ;
        rules.push_back(m_ruleUnits[token]);
    }
}
