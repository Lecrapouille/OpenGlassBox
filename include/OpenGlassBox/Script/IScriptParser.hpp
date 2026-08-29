//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file IScriptParser.hpp
//! \brief Parser interface, parse errors, and factory for script backends.

#ifndef OPEN_GLASSBOX_SCRIPT_ISCRIPT_PARSER_HPP
#define OPEN_GLASSBOX_SCRIPT_ISCRIPT_PARSER_HPP

#include "OpenGlassBox/Script/ScriptDefinitions.hpp"

namespace ogb
{

//==============================================================================
//! \brief One parse error and where it occurred.
//==============================================================================
struct ParseError
{
    //! \brief Name the script was parsed under. This is the file path when read
    //! from disk.
    std::string file;

    //! \brief Line of the bad word, counted from one to match editors.
    uint32_t line = 0u;

    //! \brief Column of the bad word, counted from one.
    uint32_t column = 0u;

    //! \brief Error message for the script author.
    std::string message;

    //! \brief Copy of the offending line, so the error can be shown without
    //! reopening the file. Needed for scripts parsed from memory.
    std::string source;

    //--------------------------------------------------------------------------
    //! \return the error as \c file:line:column:\ message, then the line
    //! and a caret under the column. Editors can jump from the first line.
    //--------------------------------------------------------------------------
    std::string format() const;
};

//==============================================================================
//! \brief Turn a simulation script into definitions.
//!
//! The interface lets you replace the language without changing the engine. The
//! current language uses keywords; a Forth-like syntax is planned next.
//==============================================================================
class IScriptParser
{
public:

    virtual ~IScriptParser() = default;

    //--------------------------------------------------------------------------
    //! \brief Read a script file into a ruleset.
    //! \param[in] filename the script to read.
    //! \param[out] definitions where to store results. New items are added, not
    //! replaced.
    //! \return true when no errors were found. On failure, definitions hold
    //! whatever was parsed, so an editor can show a partial result.
    //--------------------------------------------------------------------------
    virtual bool parseFile(std::string const& filename,
                           ScriptDefinitions& definitions) = 0;

    //--------------------------------------------------------------------------
    //! \brief Parse a script already in memory.
    //! \param[in] source the script text.
    //! \param[in] name the name used in errors when there is no file path.
    //! \param[out] definitions where to store results.
    //! \return true when no errors were found.
    //--------------------------------------------------------------------------
    virtual bool parseString(std::string const& source,
                             std::string const& name,
                             ScriptDefinitions& definitions) = 0;

    //--------------------------------------------------------------------------
    //! \return all errors from the last parse, in order. Empty after a
    //! successful parse.
    //--------------------------------------------------------------------------
    virtual std::vector<ParseError> const& errors() const = 0;

    //--------------------------------------------------------------------------
    //! \return errors from the last parse, one per line, ready to show
    //! to the user.
    //--------------------------------------------------------------------------
    std::string formatErrors() const;
};

//==============================================================================
//! \brief Pick a parser from the script file extension.
//!
//! An unknown extension uses the original language, because most scripts use
//! it.
//!
//! \param[in] filename the script whose language to detect.
//! \return a parser for it, never null.
//==============================================================================
std::unique_ptr<IScriptParser> makeScriptParser(std::string const& filename);

} // namespace ogb

#endif
