//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Script/SimpleScriptParser.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <sstream>

namespace ogb {

// =============================================================================
// PARSER
// =============================================================================

// -----------------------------------------------------------------------------
bool SimpleScriptParser::parseFile(std::string const& filename,
                               ScriptDefinitions& definitions)
{
    m_errors.clear();

    if (!m_lexer.openFile(filename))
    {
        m_errors.push_back(ParseError{ filename, 0u, 0u,
                                       std::string("Cannot open the file: ") +
                                           std::strerror(errno),
                                       {} });
        return false;
    }

    return run(definitions);
}

// -----------------------------------------------------------------------------
bool SimpleScriptParser::parseString(std::string const& source,
                                     std::string const& name,
                                     ScriptDefinitions& definitions)
{
    m_errors.clear();
    m_lexer.openString(source, name);

    return run(definitions);
}

// -----------------------------------------------------------------------------
bool SimpleScriptParser::run(ScriptDefinitions& definitions)
{
    if (m_lexer.getTokenCount() == 0u)
    {
        m_errors.push_back(
            ParseError{ m_lexer.getName(), 1u, 1u, "The script is empty", {} });
        return false;
    }

    m_definitions = &definitions;

    // First pass: only the names, so that the second one can resolve any
    // reference whatever the order of the sections.
    runPass(Pass::Declare);

    // A structural error in the first pass means the second one would walk a
    // stream it already failed to make sense of, and report the same problems
    // again from a worse position.
    if (m_errors.empty())
    {
        runPass(Pass::Define);
    }

    m_definitions = nullptr;

    return m_errors.empty();
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::runPass(Pass pass)
{
    m_pass = pass;
    m_lexer.rewind();

    while (!m_lexer.eof() && !tooManyErrors())
    {
        Token const& token = m_lexer.next();
        if (!token.valid())
            return;

        if (token.text == "resources")
            parseResources();
        else if (token.text == "rules")
            parseRules();
        else if (token.text == "layers")
            parseLayers();
        else if (token.text == "paths")
            parsePaths();
        else if (token.text == "segments")
            parseSegments();
        else if (token.text == "agents")
            parseAgents();
        else if (token.text == "units")
            parseUnits();
        else if (token.text == "zones")
            parseZones();
        else
        {
            error(token, "Expected a section (resources, rules, layers, paths, "
                         "segments, agents, units, zones) but read '" +
                             token.text + "'");
            // Resume at the next section rather than reporting its whole body
            // one word at a time: a misspelled header should cost one error,
            // and the sections after it are still worth checking.
            skipToNextSection();
        }
    }
}

// =============================================================================
// TOKEN HELPERS
// =============================================================================

// -----------------------------------------------------------------------------
Token const& SimpleScriptParser::expectToken(char const* what)
{
    Token const& token = m_lexer.next();
    if (!token.valid())
    {
        error(token, std::string("Unexpected end of script, expected ") + what);
    }

    return token;
}

// -----------------------------------------------------------------------------
bool SimpleScriptParser::expectWord(char const* word)
{
    Token const& token = expectToken(word);
    if (!token.valid())
        return false;

    if (token.text != word)
    {
        error(token, std::string("Expected '") + word + "' but read '" +
                         token.text + "'");
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::skipToEnd()
{
    while (true)
    {
        Token const& token = m_lexer.next();
        if (!token.valid() || (token.text == "end"))
            return;
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::skipToNextSection()
{
    while (!m_lexer.eof())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid() || isSectionKeyword(token.text))
            return;

        m_lexer.next();
    }
}

// -----------------------------------------------------------------------------
bool SimpleScriptParser::isSectionKeyword(std::string const& text)
{
    return (text == "resources") || (text == "rules") || (text == "layers") ||
           (text == "paths") || (text == "segments") || (text == "agents") ||
           (text == "units") || (text == "zones");
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::skipToCloseBracket()
{
    while (true)
    {
        Token const& token = m_lexer.next();
        if (!token.valid() || (token.text == "]"))
            return;
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::error(Token const& token, std::string const& message)
{
    m_errors.push_back(ParseError{ m_lexer.getName(), token.line, token.column,
                                   message, m_lexer.getSourceLine(token.line) });
}

// -----------------------------------------------------------------------------
bool SimpleScriptParser::tooManyErrors() const
{
    return m_errors.size() >= MAX_ERRORS;
}

// =============================================================================
// CHECKED CONVERSIONS
// =============================================================================

// -----------------------------------------------------------------------------
uint32_t SimpleScriptParser::toUint(Token const& token)
{
    if (!token.valid())
        return 0u;

    char* end = nullptr;
    errno = 0;
    unsigned long const value = std::strtoul(token.text.c_str(), &end, 10);

    if ((end == token.text.c_str()) || (*end != '\0'))
    {
        error(token, "'" + token.text + "' is not a whole number");
        return 0u;
    }
    if ((errno == ERANGE) || (value > 0xFFFFFFFFul))
    {
        error(token, "'" + token.text + "' does not fit in 32 bits");
        return 0u;
    }

    return uint32_t(value);
}

// -----------------------------------------------------------------------------
float SimpleScriptParser::toFloat(Token const& token)
{
    if (!token.valid())
        return 0.0f;

    char* end = nullptr;
    errno = 0;
    float const value = std::strtof(token.text.c_str(), &end);

    // This is the conversion that used to go through atoi, which silently read
    // "10.5" as ten and quietly changed what the script said.
    if ((end == token.text.c_str()) || (*end != '\0'))
    {
        error(token, "'" + token.text + "' is not a number");
        return 0.0f;
    }
    if (errno == ERANGE)
    {
        error(token, "'" + token.text + "' is out of range");
        return 0.0f;
    }

    return value;
}

// -----------------------------------------------------------------------------
uint32_t SimpleScriptParser::toColor(Token const& token)
{
    if (!token.valid())
        return 0xFFFFFFu;

    std::string text = token.text;
    if ((text.size() > 2u) && (text[0] == '0') &&
        ((text[1] == 'x') || (text[1] == 'X')))
    {
        text = text.substr(2u);
    }

    char* end = nullptr;
    errno = 0;
    unsigned long const value = std::strtoul(text.c_str(), &end, 16);

    if (text.empty() || (end == text.c_str()) || (*end != '\0'))
    {
        error(token, "'" + token.text +
                         "' is not a color, expected something like 0xRRGGBB");
        return 0xFFFFFFu;
    }
    if ((errno == ERANGE) || (value > 0xFFFFFFFFul))
    {
        error(token, "'" + token.text + "' is not a valid color");
        return 0xFFFFFFu;
    }

    return uint32_t(value);
}

// -----------------------------------------------------------------------------
bool SimpleScriptParser::toBool(Token const& token)
{
    if (!token.valid())
        return false;

    if ((token.text == "true") || (token.text == "1"))
        return true;
    if ((token.text == "false") || (token.text == "0"))
        return false;

    error(token, "'" + token.text + "' is not true or false");

    return false;
}

// -----------------------------------------------------------------------------
uint32_t SimpleScriptParser::nextUint(char const* what)
{
    return toUint(expectToken(what));
}

float SimpleScriptParser::nextFloat(char const* what)
{
    return toFloat(expectToken(what));
}

uint32_t SimpleScriptParser::nextColor(char const* what)
{
    return toColor(expectToken(what));
}

bool SimpleScriptParser::nextBool(char const* what)
{
    return toBool(expectToken(what));
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRate(uint32_t& rate, uint32_t& rateMinutes)
{
    Token const number = expectToken("a period, in ticks or in game time");
    uint32_t const value = toUint(number);

    rate = value;
    rateMinutes = 0u;

    // "rate 600" counts ticks, which is only readable to someone who knows how
    // many of them make a minute. "rate 30 minutes" says the same thing out
    // loud. The word is optional so that the older scripts keep working, and it
    // has to sit on the same line as the number: "hour" is also the name of a
    // command, and a rule that reads "rate 1" then "hour between 8 18" on the
    // next line is not asking for a period of one hour.
    Token const& unit = m_lexer.peek();
    if (unit.valid() && (unit.line == number.line))
    {
        if ((unit.text == "tick") || (unit.text == "ticks"))
        {
            m_lexer.next();
        }
        else if ((unit.text == "minute") || (unit.text == "minutes"))
        {
            m_lexer.next();
            rateMinutes = value;
        }
        else if ((unit.text == "hour") || (unit.text == "hours"))
        {
            m_lexer.next();
            rateMinutes = value * 60u;
        }
        else if ((unit.text == "day") || (unit.text == "days"))
        {
            m_lexer.next();
            rateMinutes = value * 60u * 24u;
        }
    }

    if ((rateMinutes == 0u) && (rate == 0u))
    {
        error(number, "A period of zero would run the rule at every tick, "
                      "write 1 tick instead");
        rate = 1u;
    }
}

// =============================================================================
// RESOURCES
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseResources()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'resource' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "resource")
        {
            parseResource();
        }
        else
        {
            error(token, "Expected 'resource' or 'end' but read '" +
                             token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseResource()
{
    Token const& name = expectToken("the name of the resource");
    if (!name.valid())
        return;

    if (defining())
        return;

    if (m_definitions->addResource(name.text) == nullptr)
    {
        error(name, "The resource '" + name.text + "' is defined twice");
    }
}

// =============================================================================
// PATHS AND SEGMENTS
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parsePaths()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'path' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "path")
        {
            parsePath();
        }
        else
        {
            error(token,
                  "Expected 'path' or 'end' but read '" + token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parsePath()
{
    Token const name = expectToken("the name of the path");
    if (!name.valid())
        return;

    PathType* path = nullptr;
    if (defining())
    {
        path = m_definitions->findPathType(name.text);
    }
    else if (m_definitions->addPathType(name.text) == nullptr)
    {
        error(name, "The path '" + name.text + "' is defined twice");
    }

    // A path has a single optional property, so the loop stops as soon as the
    // next token is not one it knows: it belongs to whatever follows.
    while (!tooManyErrors())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid())
            return;

        if (token.text == "color")
        {
            m_lexer.next();
            uint32_t const color = nextColor("a color");
            if (path != nullptr)
            {
                path->color = color;
            }
        }
        else
        {
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseSegments()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'segment' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "segment")
        {
            parseSegment();
        }
        else
        {
            error(token,
                  "Expected 'segment' or 'end' but read '" + token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseSegment()
{
    Token const name = expectToken("the name of the segment");
    if (!name.valid())
        return;

    SegmentType* segment = nullptr;
    if (defining())
    {
        segment = m_definitions->findSegmentType(name.text);
    }
    else if (m_definitions->addSegmentType(name.text) == nullptr)
    {
        error(name, "The segment '" + name.text + "' is defined twice");
    }

    while (!tooManyErrors())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid())
            return;

        if (token.text == "color")
        {
            m_lexer.next();
            uint32_t const color = nextColor("a color");
            if (segment != nullptr)
            {
                segment->color = color;
            }
        }
        // The three parameters of the arc performance function. Left out, they
        // keep the defaults, which is why every existing script still reads.
        else if (token.text == "speed")
        {
            m_lexer.next();
            Token const value = expectToken("the free flow speed");
            float const speed = toFloat(value);
            if (value.valid() && (speed <= 0.0f))
            {
                error(value, "The speed must be greater than zero");
            }
            else if (segment != nullptr)
            {
                segment->speed = speed;
            }
        }
        else if (token.text == "capacity")
        {
            m_lexer.next();
            Token const value = expectToken("the capacity");
            float const capacity = toFloat(value);
            if (value.valid() && (capacity <= 0.0f))
            {
                error(value, "The capacity must be greater than zero");
            }
            else if (segment != nullptr)
            {
                segment->capacity = capacity;
            }
        }
        else if (token.text == "beta")
        {
            m_lexer.next();
            float const beta = nextFloat("the exponent of the BPR function");
            if (segment != nullptr)
            {
                segment->beta = beta;
            }
        }
        else
        {
            return;
        }
    }
}

// =============================================================================
// AGENTS
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseAgents()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'agent' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "agent")
        {
            parseAgent();
        }
        else
        {
            error(token,
                  "Expected 'agent' or 'end' but read '" + token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseAgent()
{
    Token const name = expectToken("the name of the agent");
    if (!name.valid())
        return;

    AgentType* agent = nullptr;
    if (defining())
    {
        agent = m_definitions->findAgentType(name.text);
    }
    else if (m_definitions->addAgentType(name.text) == nullptr)
    {
        error(name, "The agent '" + name.text + "' is defined twice");
    }

    while (!tooManyErrors())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid())
            return;

        if (token.text == "color")
        {
            m_lexer.next();
            uint32_t const color = nextColor("a color");
            if (agent != nullptr)
            {
                agent->color = color;
            }
        }
        else if (token.text == "speed")
        {
            m_lexer.next();
            float const speed = nextFloat("a speed");
            if (agent != nullptr)
            {
                agent->speed = speed;
            }
        }
        else if (token.text == "radius")
        {
            m_lexer.next();
            uint32_t const radius = nextUint("a radius");
            if (agent != nullptr)
            {
                agent->radius = radius;
            }
        }
        else
        {
            return;
        }
    }
}

// =============================================================================
// LAYERS
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseLayers()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'layer' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "layer")
        {
            parseLayer();
        }
        else
        {
            error(token,
                  "Expected 'layer' or 'end' but read '" + token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseLayer()
{
    Token const name = expectToken("the name of the layer");
    if (!name.valid())
        return;

    LayerType* layer = nullptr;
    if (defining())
    {
        layer = m_definitions->findLayerType(name.text);
    }
    else if (m_definitions->addLayerType(name.text) == nullptr)
    {
        error(name, "The layer '" + name.text + "' is defined twice");
    }

    while (!tooManyErrors())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid())
            return;

        if (token.text == "color")
        {
            m_lexer.next();
            uint32_t const color = nextColor("a color");
            if (layer != nullptr)
            {
                layer->color = color;
            }
        }
        else if (token.text == "capacity")
        {
            m_lexer.next();
            uint32_t const capacity = nextUint("a capacity");
            if (layer != nullptr)
            {
                layer->capacity = capacity;
            }
        }
        else if (token.text == "rules")
        {
            m_lexer.next();
            std::vector<RuleLayer*> rules;
            parseRuleLayerArray(rules);
            if (layer != nullptr)
            {
                layer->rules = std::move(rules);
            }
        }
        else
        {
            return;
        }
    }
}

// =============================================================================
// UNITS
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseUnits()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'unit' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "unit")
        {
            parseUnit();
        }
        else
        {
            error(token,
                  "Expected 'unit' or 'end' but read '" + token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseUnit()
{
    Token const name = expectToken("the name of the unit");
    if (!name.valid())
        return;

    UnitType* unit = nullptr;
    if (defining())
    {
        unit = m_definitions->findUnitType(name.text);
    }
    else if (m_definitions->addUnitType(name.text) == nullptr)
    {
        error(name, "The unit '" + name.text + "' is defined twice");
    }

    while (!tooManyErrors())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid())
            return;

        if (token.text == "color")
        {
            m_lexer.next();
            uint32_t const color = nextColor("a color");
            if (unit != nullptr)
            {
                unit->color = color;
            }
        }
        else if (token.text == "layerRadius")
        {
            m_lexer.next();
            uint32_t const radius = nextUint("a radius");
            if (unit != nullptr)
            {
                unit->radius = radius;
            }
        }
        else if (token.text == "rules")
        {
            m_lexer.next();
            std::vector<RuleUnit*> rules;
            parseRuleUnitArray(rules);
            if (unit != nullptr)
            {
                unit->rules = std::move(rules);
            }
        }
        else if (token.text == "targets")
        {
            m_lexer.next();
            std::vector<std::string> targets;
            parseStringArray(targets);
            if (unit != nullptr)
            {
                unit->targets.assign(targets.begin(), targets.end());
            }
        }
        else if (token.text == "caps")
        {
            m_lexer.next();
            Resources caps;
            parseCapacitiesArray(caps);
            if (unit != nullptr)
            {
                unit->resources.setCapacities(caps);
            }
        }
        else if (token.text == "resources")
        {
            m_lexer.next();
            Resources resources;
            parseResourcesArray(resources);
            if (unit != nullptr)
            {
                unit->resources.addAll(resources);
            }
        }
        else
        {
            return;
        }
    }
}

// =============================================================================
// RULES
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRules()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'layerRule', 'unitRule', 'zoneRule' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "layerRule")
        {
            parseRuleLayer();
        }
        else if (token.text == "unitRule")
        {
            parseRuleUnit();
        }
        else if (token.text == "zoneRule")
        {
            parseRuleZone();
        }
        else
        {
            error(token, "Expected 'layerRule', 'unitRule', 'zoneRule' or 'end' but read '" +
                             token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRuleLayer()
{
    Token const name = expectToken("the name of the rule");
    if (!name.valid())
        return;

    if (!defining())
    {
        if (m_definitions->addRuleLayer(name.text) == nullptr)
        {
            error(name, "The layer rule '" + name.text + "' is defined twice");
        }
        skipToEnd();
        return;
    }

    RuleLayerType type(name.text);

    while (!tooManyErrors())
    {
        Token const token = expectToken("a command or 'end'");
        if (!token.valid())
            return;

        if (token.text == "end")
        {
            RuleLayer* rule = m_definitions->findRuleLayer(name.text);
            if (rule != nullptr)
            {
                rule->configureFrom(type);
            }
            return;
        }

        if (token.text == "rate")
        {
            parseRate(type.rate, type.rateMinutes);
        }
        else if (token.text == "randomTiles")
        {
            type.randomTiles = nextBool("true or false");
        }
        else if (token.text == "randomTilesPercent")
        {
            type.randomTiles = true;
            type.randomTilesPercent = nextUint("a percentage");
        }
        else
        {
            IRuleCommand* command = parseCommand(token);
            if (command == nullptr)
            {
                skipToEnd();
                return;
            }
            type.commands.push_back(command);
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRuleUnit()
{
    Token const name = expectToken("the name of the rule");
    if (!name.valid())
        return;

    if (!defining())
    {
        if (m_definitions->addRuleUnit(name.text) == nullptr)
        {
            error(name, "The unit rule '" + name.text + "' is defined twice");
        }
        skipToEnd();
        return;
    }

    RuleUnitType type(name.text);

    while (!tooManyErrors())
    {
        Token const token = expectToken("a command or 'end'");
        if (!token.valid())
            return;

        if (token.text == "end")
        {
            RuleUnit* rule = m_definitions->findRuleUnit(name.text);
            if (rule != nullptr)
            {
                rule->configureFrom(type);
            }
            return;
        }

        if (token.text == "rate")
        {
            parseRate(type.rate, type.rateMinutes);
        }
        else if (token.text == "onFail")
        {
            // The fallback rule may well be written after this one, which is
            // exactly what the declaration pass is for.
            Token const target = expectToken("the name of the fallback rule");
            if (!target.valid())
                return;

            if (target.text == name.text)
            {
                error(target, "The rule '" + name.text +
                                  "' falls back on itself, which would loop");
            }
            else
            {
                type.onFail = m_definitions->findRuleUnit(target.text);
                if (type.onFail == nullptr)
                {
                    error(target,
                          "Unknown unit rule '" + target.text + "'");
                }
            }
        }
        else
        {
            IRuleCommand* command = parseCommand(token);
            if (command == nullptr)
            {
                skipToEnd();
                return;
            }
            type.commands.push_back(command);
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRuleZone()
{
    Token const name = expectToken("the name of the rule");
    if (!name.valid())
        return;

    if (!defining())
    {
        if (m_definitions->addRuleZone(name.text) == nullptr)
        {
            error(name, "The zone rule '" + name.text + "' is defined twice");
        }
        skipToEnd();
        return;
    }

    RuleZoneType type(name.text);

    while (!tooManyErrors())
    {
        Token const token = expectToken("a command or 'end'");
        if (!token.valid())
            return;

        if (token.text == "end")
        {
            RuleZone* rule = m_definitions->findRuleZone(name.text);
            if (rule != nullptr)
                rule->configureFrom(type);
            return;
        }

        if (token.text == "rate")
        {
            parseRate(type.rate, type.rateMinutes);
        }
        else
        {
            IRuleCommand* command = parseCommand(token);
            if (command == nullptr)
            {
                skipToEnd();
                return;
            }
            type.commands.push_back(command);
        }
    }
}

// -----------------------------------------------------------------------------
IRuleCommand* SimpleScriptParser::parseCommand(Token const& token)
{
    IRuleValue* target = nullptr;

    if ((token.text == "local") || (token.text == "global"))
    {
        Token const name = expectToken("the name of a resource");
        if (!name.valid())
            return nullptr;

        Resource const* const resource = m_definitions->findResource(name.text);
        if (resource == nullptr)
        {
            error(name, "Unknown resource '" + name.text + "'");
            return nullptr;
        }

        if (token.text == "local")
        {
            target = m_definitions->own(
                std::unique_ptr<IRuleValue>(new RuleValueLocal(*resource)));
        }
        else
        {
            target = m_definitions->own(
                std::unique_ptr<IRuleValue>(new RuleValueGlobal(*resource)));
        }
    }
    else if (token.text == "layer")
    {
        Token const name = expectToken("the name of a layer");
        if (!name.valid())
            return nullptr;

        // The layer is looked up by name at run time, on the City the rule runs
        // on, but a name no layer type answers to can only ever fail.
        if (m_definitions->findLayerType(name.text) == nullptr)
        {
            error(name, "Unknown layer '" + name.text + "'");
            return nullptr;
        }

        target = m_definitions->own(
            std::unique_ptr<IRuleValue>(new RuleValueLayer(name.text)));
    }
    else if (token.text == "agent")
    {
        Token const name = expectToken("the name of an agent");
        if (!name.valid())
            return nullptr;

        AgentType const* const agent = m_definitions->findAgentType(name.text);
        if (agent == nullptr)
        {
            error(name, "Unknown agent '" + name.text + "'");
            return nullptr;
        }

        std::string searchTarget;
        Resources resources;
        bool loaded = false;

        while (!tooManyErrors() && !loaded)
        {
            Token const cmd = expectToken("'to' or 'add'");
            if (!cmd.valid())
                return nullptr;

            if (cmd.text == "to")
            {
                Token const unit = expectToken("the type of unit to look for");
                if (!unit.valid())
                    return nullptr;
                searchTarget = unit.text;
            }
            else if (cmd.text == "add")
            {
                parseResourcesArray(resources);
                loaded = true;
            }
            else
            {
                error(cmd, "Expected 'to' or 'add' but read '" + cmd.text + "'");
                return nullptr;
            }
        }

        if (!loaded)
            return nullptr;

        return m_definitions->own(std::unique_ptr<IRuleCommand>(
            new RuleCommandAgent(*agent, searchTarget, resources)));
    }
    else if (token.text == "hour")
    {
        if (!expectWord("between"))
            return nullptr;
        uint32_t const from = nextUint("the start hour");
        uint32_t const to = nextUint("the end hour");
        return m_definitions->own(std::unique_ptr<IRuleCommand>(
            new RuleCommandHour(from, to)));
    }
    else if (token.text == "spawn")
    {
        Token const name = expectToken("the name of a unit");
        if (!name.valid())
            return nullptr;
        UnitType* const unit = m_definitions->findUnitType(name.text);
        if (defining() && (unit == nullptr))
        {
            error(name, "Unknown unit '" + name.text + "'");
            return nullptr;
        }
        if (!expectWord("at"))
            return nullptr;
        Token const where = expectToken("'nearestSegment' or 'freeCell'");
        RuleCommandSpawn::Placement placement = RuleCommandSpawn::Placement::NearestSegment;
        if (where.text == "freeCell")
            placement = RuleCommandSpawn::Placement::FreeCell;
        else if (where.text != "nearestSegment")
        {
            error(where, "Expected 'nearestSegment' or 'freeCell' but read '" +
                             where.text + "'");
            return nullptr;
        }
        if (unit == nullptr)
            return nullptr;
        return m_definitions->own(std::unique_ptr<IRuleCommand>(
            new RuleCommandSpawn(*unit, placement)));
    }
    else if (token.text == "count")
    {
        Token const name = expectToken("the name of a unit");
        if (!name.valid())
            return nullptr;
        Token const cmp = expectToken("'greater', 'less' or 'equals'");
        RuleCommandTest::Comparison comparison = RuleCommandTest::Comparison::LESS;
        if (cmp.text == "greater")
            comparison = RuleCommandTest::Comparison::GREATER;
        else if (cmp.text == "equals")
            comparison = RuleCommandTest::Comparison::EQUALS;
        else if (cmp.text != "less")
        {
            error(cmp, "Expected 'greater', 'less' or 'equals' but read '" +
                           cmp.text + "'");
            return nullptr;
        }
        uint32_t const amount = nextUint("a count");
        return m_definitions->own(std::unique_ptr<IRuleCommand>(
            new RuleCommandCount(name.text, comparison, amount)));
    }
    else if (token.text == "upgrade")
    {
        Token const from = expectToken("the unit to replace");
        if (!from.valid())
            return nullptr;
        if (!expectWord("to"))
            return nullptr;
        Token const to = expectToken("the unit to replace it with");
        if (!to.valid())
            return nullptr;
        UnitType* const fromType = m_definitions->findUnitType(from.text);
        UnitType* const toType = m_definitions->findUnitType(to.text);
        if (defining())
        {
            if (fromType == nullptr)
            {
                error(from, "Unknown unit '" + from.text + "'");
                return nullptr;
            }
            if (toType == nullptr)
            {
                error(to, "Unknown unit '" + to.text + "'");
                return nullptr;
            }
        }
        if ((fromType == nullptr) || (toType == nullptr))
            return nullptr;
        return m_definitions->own(std::unique_ptr<IRuleCommand>(
            new RuleCommandUpgrade(*fromType, *toType)));
    }
    else if (token.text == "destroy")
    {
        Token const name = expectToken("the name of a unit");
        if (!name.valid())
            return nullptr;
        return m_definitions->own(std::unique_ptr<IRuleCommand>(
            new RuleCommandDestroy(name.text)));
    }
    else
    {
        error(token, "Expected a command (local, global, layer, agent, hour, spawn, "
                     "count, upgrade, destroy) but read '" +
                         token.text + "'");
        return nullptr;
    }

    Token const cmd = expectToken("'add', 'remove', 'greater', 'less' or 'equals'");
    if (!cmd.valid())
        return nullptr;

    std::unique_ptr<IRuleCommand> command;

    if (cmd.text == "add")
    {
        command.reset(new RuleCommandAdd(*target, nextUint("an amount")));
    }
    else if (cmd.text == "remove")
    {
        command.reset(new RuleCommandRemove(*target, nextUint("an amount")));
    }
    else if (cmd.text == "greater")
    {
        command.reset(new RuleCommandTest(
            *target, RuleCommandTest::Comparison::GREATER, nextUint("a value")));
    }
    else if (cmd.text == "less")
    {
        command.reset(new RuleCommandTest(
            *target, RuleCommandTest::Comparison::LESS, nextUint("a value")));
    }
    else if (cmd.text == "equals")
    {
        command.reset(new RuleCommandTest(
            *target, RuleCommandTest::Comparison::EQUALS, nextUint("a value")));
    }
    else
    {
        error(cmd, "Expected 'add', 'remove', 'greater', 'less' or 'equals' "
                   "but read '" +
                       cmd.text + "'");
        return nullptr;
    }

    return m_definitions->own(std::move(command));
}

// =============================================================================
// ARRAYS
// =============================================================================

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseResourcesArray(Resources& resources)
{
    if (!expectWord("["))
    {
        skipToCloseBracket();
        return;
    }

    while (!tooManyErrors())
    {
        Token const name = expectToken("a resource or ']'");
        if (!name.valid() || (name.text == "]"))
            return;

        // Nothing is resolved during the declaration pass: the very point of it
        // is that the names it refers to may not exist yet.
        if (!defining())
        {
            expectToken("an amount");
            continue;
        }

        Resource const* const resource = m_definitions->findResource(name.text);
        uint32_t const amount = nextUint("an amount");

        if (resource == nullptr)
        {
            error(name, "Unknown resource '" + name.text + "'");
            continue;
        }

        resources.addResource(resource->getTypeName(), amount);
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseCapacitiesArray(Resources& resources)
{
    if (!expectWord("["))
    {
        skipToCloseBracket();
        return;
    }

    while (!tooManyErrors())
    {
        Token const name = expectToken("a resource or ']'");
        if (!name.valid() || (name.text == "]"))
            return;

        if (!defining())
        {
            expectToken("a capacity");
            continue;
        }

        Resource const* const resource = m_definitions->findResource(name.text);
        uint32_t const capacity = nextUint("a capacity");

        if (resource == nullptr)
        {
            error(name, "Unknown resource '" + name.text + "'");
            continue;
        }

        resources.setCapacity(resource->getTypeName(), capacity);
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseStringArray(std::vector<std::string>& out)
{
    if (!expectWord("["))
    {
        skipToCloseBracket();
        return;
    }

    while (!tooManyErrors())
    {
        Token const& token = expectToken("a name or ']'");
        if (!token.valid() || (token.text == "]"))
            return;

        out.push_back(token.text);
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRuleLayerArray(std::vector<RuleLayer*>& rules)
{
    if (!expectWord("["))
    {
        skipToCloseBracket();
        return;
    }

    while (!tooManyErrors())
    {
        Token const name = expectToken("a layer rule or ']'");
        if (!name.valid() || (name.text == "]"))
            return;

        if (!defining())
            continue;

        // This is the lookup that used to push whatever operator[] default
        // constructed, so a misspelled rule became a null pointer the engine
        // dereferenced later.
        RuleLayer* const rule = m_definitions->findRuleLayer(name.text);
        if (rule == nullptr)
        {
            error(name, "Unknown layer rule '" + name.text + "'");
            continue;
        }

        rules.push_back(rule);
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRuleUnitArray(std::vector<RuleUnit*>& rules)
{
    if (!expectWord("["))
    {
        skipToCloseBracket();
        return;
    }

    while (!tooManyErrors())
    {
        Token const name = expectToken("a unit rule or ']'");
        if (!name.valid() || (name.text == "]"))
            return;

        if (!defining())
            continue;

        RuleUnit* const rule = m_definitions->findRuleUnit(name.text);
        if (rule == nullptr)
        {
            error(name, "Unknown unit rule '" + name.text + "'");
            continue;
        }

        rules.push_back(rule);
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseRuleZoneArray(std::vector<RuleZone*>& rules)
{
    if (!expectWord("["))
    {
        skipToCloseBracket();
        return;
    }

    while (!tooManyErrors())
    {
        Token const name = expectToken("an zone rule or ']'");
        if (!name.valid() || (name.text == "]"))
            return;

        if (!defining())
            continue;

        RuleZone* const rule = m_definitions->findRuleZone(name.text);
        if (rule == nullptr)
        {
            error(name, "Unknown zone rule '" + name.text + "'");
            continue;
        }

        rules.push_back(rule);
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseZones()
{
    while (!tooManyErrors())
    {
        Token const& token = expectToken("'zone' or 'end'");
        if (!token.valid() || (token.text == "end"))
            return;

        if (token.text == "zone")
        {
            parseZone();
        }
        else
        {
            error(token, "Expected 'zone' or 'end' but read '" + token.text + "'");
            skipToEnd();
            return;
        }
    }
}

// -----------------------------------------------------------------------------
void SimpleScriptParser::parseZone()
{
    Token const name = expectToken("the name of the zone");
    if (!name.valid())
        return;

    ZoneType* zone = nullptr;
    if (defining())
    {
        zone = m_definitions->findZoneType(name.text);
    }
    else if (m_definitions->addZoneType(name.text) == nullptr)
    {
        error(name, "The zone '" + name.text + "' is defined twice");
    }

    while (!tooManyErrors())
    {
        Token const& token = m_lexer.peek();
        if (!token.valid())
            return;

        if (token.text == "color")
        {
            m_lexer.next();
            uint32_t const color = nextColor("a color");
            if (zone != nullptr)
                zone->color = color;
        }
        else if (token.text == "rules")
        {
            m_lexer.next();
            std::vector<RuleZone*> rules;
            parseRuleZoneArray(rules);
            if (zone != nullptr)
                zone->rules = std::move(rules);
        }
        else
        {
            return;
        }
    }
}

// =============================================================================
// FACTORY
// =============================================================================

// -----------------------------------------------------------------------------
std::unique_ptr<IScriptParser> makeScriptParser(std::string const& filename)
{
    (void)filename;

    // Only one language so far. The Forth flavored one will be selected here by
    // its extension, which is why this is a factory and not a constructor.
    return std::unique_ptr<IScriptParser>(new SimpleScriptParser());
}

} // namespace ogb
