//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Lexer.hpp
//! \brief Split a simulation script into tokens with positions.

#ifndef OPEN_GLASSBOX_SCRIPT_LEXER_HPP
#define OPEN_GLASSBOX_SCRIPT_LEXER_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief One word of a simulation script and where it appears.
//!
//! Storing the position on the token lets errors report line and column even
//! when the error is found later in the parse.
//==============================================================================
struct Token
{
    //! \brief The word. Empty for the sentinel past the end of the input.
    std::string text;

    //! \brief Line number, counted from one to match editors.
    uint32_t line = 0u;

    //! \brief Column where the word starts, counted from one.
    uint32_t column = 0u;

    //! \return false for the end sentinel. The parser uses this to stop.
    bool valid() const
    {
        return !text.empty();
    }
};

//==============================================================================
//! \brief Split a simulation script into positioned words.
//!
//! The full source stays in memory. Scripts are small. This allows errors to
//! quote a line and lets the parser read the token stream twice, so a name can
//! be used before its definition section.
//==============================================================================
class Lexer
{
public:

    //--------------------------------------------------------------------------
    //! \brief Read a file and split it into words.
    //! \param[in] filename the script file. Its name appears in error messages.
    //! \return false when the file cannot be opened.
    //--------------------------------------------------------------------------
    bool openFile(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Split a script already in memory. Used by tests without a file.
    //! \param[in] source the script text.
    //! \param[in] name the name used in error messages.
    //--------------------------------------------------------------------------
    void openString(std::string const& source, std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Take the next word.
    //! \return that word, or the end sentinel after the input is exhausted.
    //! The end sentinel is returned on every later call. Running past the end is
    //! not an error; it is how the parser stops.
    //--------------------------------------------------------------------------
    Token const& next();

    //--------------------------------------------------------------------------
    //! \return the word next() would return, without taking it. One word
    //! of lookahead is enough for this language.
    //--------------------------------------------------------------------------
    Token const& peek() const;

    //--------------------------------------------------------------------------
    //! \return the word last taken by next(), or the end sentinel before
    //! the first call.
    //--------------------------------------------------------------------------
    Token const& current() const;

    //--------------------------------------------------------------------------
    //! \return true when every word was taken.
    //--------------------------------------------------------------------------
    bool eof() const
    {
        return m_index >= m_tokens.size();
    }

    //--------------------------------------------------------------------------
    //! \brief Go back to the first word. The parser uses this for a second pass
    //! to fill in bodies declared on the first pass.
    //--------------------------------------------------------------------------
    void rewind();

    //--------------------------------------------------------------------------
    //! \return the name of what was split, used in error messages.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::string const& getName() const
    {
        return m_name;
    }

    //--------------------------------------------------------------------------
    //! \brief Return one line of the script for error display.
    //! \param[in] line line number, counted from one.
    //! \return the line without its newline, or an empty string when missing.
    //--------------------------------------------------------------------------
    [[nodiscard]] std::string getSourceLine(uint32_t line) const;

    //--------------------------------------------------------------------------
    //! \return how many words the script contains.
    //--------------------------------------------------------------------------
    [[nodiscard]] size_t getTokenCount() const
    {
        return m_tokens.size();
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Split a script into words and drop comments.
    //! \param[in] source the script, also kept line by line for error quotes.
    //--------------------------------------------------------------------------
    void tokenize(std::string const& source);

private:

    //! \brief Name of what was split, used in error messages.
    std::string m_name;
    //! \brief The script line by line, for error quotes.
    std::vector<std::string> m_lines;
    //! \brief Every word in order, with its position.
    std::vector<Token> m_tokens;
    //! \brief Index of the next word to return.
    size_t m_index = 0u;
    //! \brief End sentinel, and current() before the first next(). Carries the
    //! position at the end of the script.
    Token m_end;
};

} // namespace ogb

#endif
