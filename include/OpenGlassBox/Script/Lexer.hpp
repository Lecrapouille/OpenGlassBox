//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Lexer.hpp
//! \brief Tokenizer that splits a simulation script into positioned tokens.

#ifndef OPEN_GLASSBOX_SCRIPT_LEXER_HPP
#define OPEN_GLASSBOX_SCRIPT_LEXER_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace ogb
{

//==============================================================================
//! \brief A word of a simulation script together with where it was written.
//!
//! Carrying the position on the token rather than on the stream is what lets an
//! error name the line and column of the thing it is complaining about, even
//! when the complaint comes from much later in the parse.
//==============================================================================
struct Token
{
    //! \brief The word itself. Empty for the sentinel past the end of the
    //! input.
    std::string text;

    //! \brief Which line it was written on, counted from one so that the
    //! numbers match what an editor shows.
    uint32_t line = 0u;

    //! \brief Which column it starts at, counted from one.
    uint32_t column = 0u;

    //! \brief \return false for the sentinel returned past the end of the
    //! input, which is how a parser knows to stop.
    bool valid() const
    {
        return !text.empty();
    }
};

//==============================================================================
//! \brief Splits a simulation script into positioned words.
//!
//! The whole source is held in memory, which a simulation script easily fits
//! in, and buys two things: an error can quote the offending line, and the
//! parser can walk the token stream twice, which is how a name may be used
//! before the section that defines it.
//==============================================================================
class Lexer
{
public:

    //--------------------------------------------------------------------------
    //! \brief Read a file and split it into words.
    //! \param[in] filename the script to read. Its name is what the errors will
    //! be reported against.
    //! \return false when the file cannot be opened.
    //--------------------------------------------------------------------------
    bool open(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Split a script already in memory. What the tests use, having no
    //! file to write.
    //! \param[in] source the script itself.
    //! \param[in] name what the errors should call it.
    //--------------------------------------------------------------------------
    void openString(std::string const& source, std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Take the next word.
    //! \return that word, or the end sentinel once the input is exhausted,
    //! which it then keeps returning: running off the end is not an error, it
    //! is how a parser stops.
    //--------------------------------------------------------------------------
    Token const& next();

    //--------------------------------------------------------------------------
    //! \brief \return the word next() would return, without taking it. One word
    //! of lookahead is all the language needs.
    //--------------------------------------------------------------------------
    Token const& peek() const;

    //--------------------------------------------------------------------------
    //! \brief \return the word last taken by next(), or the end sentinel before
    //! the first call.
    //--------------------------------------------------------------------------
    Token const& current() const;

    //--------------------------------------------------------------------------
    //! \brief \return true once every word has been taken.
    //--------------------------------------------------------------------------
    bool eof() const
    {
        return m_index >= m_tokens.size();
    }

    //--------------------------------------------------------------------------
    //! \brief Go back to the first word, which is how the parser walks the
    //! whole script a second time to fill in the bodies it declared on the
    //! first.
    //--------------------------------------------------------------------------
    void rewind();

    //--------------------------------------------------------------------------
    //! \brief \return the name of what was split, which the errors are reported
    //! against.
    //--------------------------------------------------------------------------
    std::string const& name() const
    {
        return m_name;
    }

    //--------------------------------------------------------------------------
    //! \brief One line of the script as it was written, so that an error can
    //! quote it.
    //! \param[in] line which line, counted from one.
    //! \return the line without its newline, or an empty string when there is
    //! no such line.
    //--------------------------------------------------------------------------
    std::string sourceLine(uint32_t line) const;

    //--------------------------------------------------------------------------
    //! \brief \return how many words the script holds.
    //--------------------------------------------------------------------------
    size_t size() const
    {
        return m_tokens.size();
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Split a script into words, dropping the comments.
    //! \param[in] source the script itself, kept line by line as well so that
    //! an error can quote it.
    //--------------------------------------------------------------------------
    void tokenize(std::string const& source);

private:

    //! \brief Name of what was split, used in the error messages.
    std::string m_name;
    //! \brief The script, line by line, so that an error can quote the line it
    //! is complaining about.
    std::vector<std::string> m_lines;
    //! \brief Every word of the script, in order, with its position.
    std::vector<Token> m_tokens;
    //! \brief Which word comes next.
    size_t m_index = 0u;
    //! \brief Returned past the end of the input, and by current() before the
    //! first next(). Carries the position of the end of the script, so that a
    //! script cut short still reports a sensible place.
    Token m_end;
};

} // namespace ogb

#endif
