//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Ruleset.hpp"

#include <iostream>
#include <sstream>

// -----------------------------------------------------------------------------
//! \brief Load into a scratch set of definitions and adopt them only once the
//! parse succeeded. A script that fails to load therefore leaves a running
//! simulation exactly as it was.
// -----------------------------------------------------------------------------
namespace ogb
{

template <class Load>
static bool loadInto(ScriptDefinitions& definitions,
                     std::vector<ParseError>& errors,
                     std::unique_ptr<IScriptParser>& parser,
                     Load load)
{
    ScriptDefinitions parsed;
    bool const success = load(*parser, parsed);

    errors = parser->errors();

    if (success)
    {
        definitions = std::move(parsed);
    }

    return success;
}

// -----------------------------------------------------------------------------
bool Ruleset::loadFile(std::string const& filename)
{
    std::cout << "Parsing script '" << filename << "'" << std::endl;

    auto parser = makeScriptParser(filename);
    bool const success =
        loadInto(m_definitions,
                 m_errors,
                 parser,
                 [&filename](IScriptParser& p, ScriptDefinitions& out)
                 { return p.parseFile(filename, out); });

    if (success)
    {
        std::cout << "  done" << std::endl;
    }
    else
    {
        std::cerr << formatErrors() << std::endl;
    }

    return success;
}

// -----------------------------------------------------------------------------
bool Ruleset::loadString(std::string const& source, std::string const& name)
{
    auto parser = makeScriptParser(name);

    return loadInto(m_definitions,
                    m_errors,
                    parser,
                    [&source, &name](IScriptParser& p, ScriptDefinitions& out)
                    { return p.parseString(source, name, out); });
}

// -----------------------------------------------------------------------------
std::string Ruleset::formatErrors() const
{
    std::ostringstream stream;
    bool first = true;

    for (auto const& e : m_errors)
    {
        if (!first)
        {
            stream << '\n';
        }
        stream << e.format();
        first = false;
    }

    return stream.str();
}

} // namespace ogb
