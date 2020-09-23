#include "Script.hpp"
#include <iostream>
#include <stdexcept>

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
    if (!m_file)
    {
        m_success = false;
        return ;
    }

    try
    {
        parseScript();
        m_success = true;
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
        std::cout << "I read '" << m_token << "'" << std::endl;
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
            parseSegments();
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
    std::cout << "parseResource\n";
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
    m_pathTypes[path->m_name] = path;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
        {
            //path->m_color = toColor(nextToken());
            nextToken();
            return ;
        }
        else
        {
            throw std::runtime_error("parsePath()");
        }
    }
}

void Script::parseSegments()
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "end")
            return ;
        else if (token == "segment")
            parseSegment();
        else
            throw std::runtime_error("parseSegments()");
    }
}

void Script::parseSegment()
{
    SegmentType* seg = new SegmentType(nextToken());
    m_segmentTypes[seg->m_name] = seg;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
        {
            seg->m_color = toColor(nextToken());
            return ;
        }
        else
        {
            throw std::runtime_error("parseSegment()");
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
    AgentType* agent = new AgentType();
    m_agentTypes[nextToken()] = agent;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
            agent->m_color = toColor(nextToken());
        else if (token == "speed")
        {
            agent->m_speed = toFloat(nextToken());
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
            parseMapRule();
        else if (token == "unitRule")
            parseUnitRule();
        else
            throw std::runtime_error("parseRules()");
    }
}

void Script::parseMapRule()
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
            type.m_rate = toUint(nextToken());
        else if (token == "randomTiles")
            type.m_randomTiles = toBool(nextToken());
        else if (token == "randomTilesPercent")
        {
            type.m_randomTiles = true;
            type.m_randomTilesPercent = toUint(nextToken());
        }
        else
            type.m_commands.push_back(parseCommand(token));
    }
}

void Script::parseUnitRule()
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
            type.m_rate = toUint(nextToken());
        //else if (token == "onFail") // TODO
        //{}
        else
            type.m_commands.push_back(parseCommand(token));
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
            //TODO
            //command = new RuleCommandAdd();
            //command->m_target = target;
            //command->m_amount =
            toUint(nextToken());
        }
        else if (cmd == "remove")
        {
            //TODO
            //command = new RuleCommandRemove();
            //command->m_target = target;
            //command->m_amount =
            toUint(nextToken());
        }
        else if (cmd == "greater")
        {
            //TODO
            toUint(nextToken());
        }
        else if (cmd == "less")
        {
            //TODO
            toUint(nextToken());
        }
        else if (cmd == "equals")
        {
            //TODO
            toUint(nextToken());
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
    m_mapTypes[map->m_type] = map;

    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
            map->m_color = toColor(nextToken());
        else if (token == "capacity")
            map->m_capacity = toUint(nextToken());
        else if (token == "rules")
        {
            parseRuleArray(map->m_rules);
            return ;
        }
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

void Script::parseRuleArray(std::vector<RuleMap>& rules)
{
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "]")
            return ;
        //TODO unitType
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
    m_unitTypes[unit->m_name] = unit;

    Resources caps;
    Resources resources;
    std::vector<std::string> todo;
    while (true)
    {
        std::string const& token = nextToken();
        if (token == "color")
            unit->m_color = toColor(nextToken());
        else if (token == "mapRadius")
            unit->m_radius = toUint(nextToken());
        else if (token == "rules")
            parseStringArray(todo);
            // TODO parseRuleArray(unit->m_rules);
        else if (token == "targets")
            parseStringArray(unit->m_targets);
        else if (token == "caps")
        {
            parseCapacitiesArray(caps);
            unit->m_resources.setCapacities(caps);
        }
        else if (token == "resources")
        {
            parseResourcesArray(resources);
            unit->m_resources.addResources(resources);
            return ;
        }
        else
            throw std::runtime_error("parseUnit()");
    }
}
