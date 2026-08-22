//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SCRIPT_ISCRIPT_PARSER_HPP
#  define OPEN_GLASSBOX_SCRIPT_ISCRIPT_PARSER_HPP

#  include "OpenGlassBox/Script/ScriptDefinitions.hpp"
#  include <memory>
#  include <string>
#  include <vector>

namespace ogb {

//==============================================================================
//! \brief One thing wrong with a script, and where.
//==============================================================================
struct ParseError
{
    std::string file;
    //! \brief One based.
    uint32_t line = 0u;
    uint32_t column = 0u;
    std::string message;
    //! \brief The offending source line, so that the error can be read without
    //! opening the file.
    std::string source;

    //--------------------------------------------------------------------------
    //! \brief Render as "file:line:column: message", followed by the source
    //! line and a caret under the column. The first line is the format every
    //! editor knows how to jump from.
    //--------------------------------------------------------------------------
    std::string format() const;
};

//==============================================================================
//! \brief Turns a simulation script into definitions.
//!
//! The interface exists so that the language can be replaced without touching
//! the engine: the current one is a keyword soup, and a Forth-like syntax is
//! the intended successor.
//==============================================================================
class IScriptParser
{
public:

    virtual ~IScriptParser() = default;

    //--------------------------------------------------------------------------
    //! \brief Parse a file into \c definitions.
    //! \return true when nothing was wrong. On failure \c definitions may hold
    //! what could be understood, which is what lets an editor show a partially
    //! broken script.
    //--------------------------------------------------------------------------
    virtual bool parse(std::string const& filename,
                       ScriptDefinitions& definitions) = 0;

    //--------------------------------------------------------------------------
    //! \brief Parse a string, reported under the given name. Used by the tests.
    //--------------------------------------------------------------------------
    virtual bool parseString(std::string const& source, std::string const& name,
                             ScriptDefinitions& definitions) = 0;

    //--------------------------------------------------------------------------
    //! \brief Everything found wrong by the last parse, in the order it was
    //! found. Empty on success.
    //--------------------------------------------------------------------------
    virtual std::vector<ParseError> const& errors() const = 0;

    //--------------------------------------------------------------------------
    //! \brief The errors of the last parse, one per line, ready to be shown.
    //--------------------------------------------------------------------------
    std::string formatErrors() const;
};

//==============================================================================
//! \brief Pick a parser for a file, from its extension.
//!
//! Unknown extensions fall back on the historical language rather than
//! refusing, since that is what every script in the wild is written in.
//==============================================================================
std::unique_ptr<IScriptParser> makeScriptParser(std::string const& filename);

} // namespace ogb

#endif
