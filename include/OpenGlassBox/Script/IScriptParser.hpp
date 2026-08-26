//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file IScriptParser.hpp
//! \brief Parser interface, parse errors and factory for script language
//! backends.

#ifndef OPEN_GLASSBOX_SCRIPT_ISCRIPT_PARSER_HPP
#define OPEN_GLASSBOX_SCRIPT_ISCRIPT_PARSER_HPP

#include "OpenGlassBox/Script/ScriptDefinitions.hpp"

namespace ogb
{

//==============================================================================
//! \brief One thing wrong with a script, and where.
//==============================================================================
struct ParseError
{
    //! \brief Name the script was parsed under, which is its path when it came
    //! from a file.
    std::string file;

    //! \brief Line of the offending word, counted from one so that the numbers
    //! match what an editor shows.
    uint32_t line = 0u;

    //! \brief Column of the offending word, counted from one.
    uint32_t column = 0u;

    //! \brief What is wrong, in words meant for whoever wrote the script.
    std::string message;

    //! \brief The offending line itself, copied, so the error can be read
    //! without opening the file again. That matters for a script parsed from
    //! memory, which has no file to open.
    std::string source;

    //--------------------------------------------------------------------------
    //! \brief \return the error as \c file:line:column:\ message, followed by
    //! the offending line and a caret under the column. The first line is in
    //! the format every editor knows how to jump from.
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
    //! \brief Read a script file into a ruleset.
    //! \param[in] filename the script to read.
    //! \param[out] definitions where to put what was understood. Added to
    //! rather than replaced.
    //! \return true when nothing was wrong. On failure the ruleset holds
    //! whatever could be understood, which is what lets an editor show a
    //! half-broken script instead of nothing at all.
    //--------------------------------------------------------------------------
    virtual bool parseFile(std::string const& filename,
                           ScriptDefinitions& definitions) = 0;

    //--------------------------------------------------------------------------
    //! \brief The same, from a script already in memory.
    //! \param[in] source the script itself.
    //! \param[in] name what the errors should call it, there being no path.
    //! \param[out] definitions where to put what was understood.
    //! \return true when nothing was wrong.
    //--------------------------------------------------------------------------
    virtual bool parseString(std::string const& source,
                             std::string const& name,
                             ScriptDefinitions& definitions) = 0;

    //--------------------------------------------------------------------------
    //! \brief \return everything found wrong by the last parse, in the order it
    //! was found. Empty after a parse that went well.
    //--------------------------------------------------------------------------
    virtual std::vector<ParseError> const& errors() const = 0;

    //--------------------------------------------------------------------------
    //! \brief \return the errors of the last parse, one per line, ready to be
    //! put in front of a user.
    //--------------------------------------------------------------------------
    std::string formatErrors() const;
};

//==============================================================================
//! \brief Pick the parser for a script, from the extension of its name.
//!
//! An extension nobody claims falls back on the original language rather than
//! being refused, that being what every script in the wild is written in.
//!
//! \param[in] filename the script whose language is to be guessed.
//! \return a parser for it, never null.
//==============================================================================
std::unique_ptr<IScriptParser> makeScriptParser(std::string const& filename);

} // namespace ogb

#endif
