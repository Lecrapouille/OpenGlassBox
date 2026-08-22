//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_SCRIPT_LEXER_HPP
#  define OPEN_GLASSBOX_SCRIPT_LEXER_HPP

#  include <cstdint>
#  include <string>
#  include <vector>

namespace ogb {

//==============================================================================
//! \brief A word of a simulation script together with where it was written.
//!
//! Carrying the position on the token rather than on the stream is what lets an
//! error name the line and column of the thing it is complaining about, even
//! when the complaint comes from much later in the parse.
//==============================================================================
struct Token
{
    //! \brief The word itself. Empty at the end of the input.
    std::string text;
    //! \brief One based, so that the numbers match what an editor shows.
    uint32_t line = 0u;
    uint32_t column = 0u;

    //! \brief False for the sentinel returned past the end of the input.
    bool valid() const { return !text.empty(); }
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
    //! \brief Read and tokenize a file.
    //! \return false when the file cannot be opened.
    //--------------------------------------------------------------------------
    bool open(std::string const& filename);

    //--------------------------------------------------------------------------
    //! \brief Tokenize a string. \c name is what the errors are reported
    //! against. Used by the tests, which have no file to write.
    //--------------------------------------------------------------------------
    void openString(std::string const& source, std::string const& name);

    //--------------------------------------------------------------------------
    //! \brief Consume and return the next token. Returns the end sentinel, and
    //! keeps returning it, once the input is exhausted.
    //--------------------------------------------------------------------------
    Token const& next();

    //--------------------------------------------------------------------------
    //! \brief The next token without consuming it.
    //--------------------------------------------------------------------------
    Token const& peek() const;

    //--------------------------------------------------------------------------
    //! \brief The token last returned by next().
    //--------------------------------------------------------------------------
    Token const& current() const;

    //--------------------------------------------------------------------------
    //! \brief Whether the whole stream has been consumed.
    //--------------------------------------------------------------------------
    bool eof() const { return m_index >= m_tokens.size(); }

    //--------------------------------------------------------------------------
    //! \brief Go back to the first token, to walk the stream a second time.
    //--------------------------------------------------------------------------
    void rewind();

    //--------------------------------------------------------------------------
    //! \brief Name of what was tokenized, used in the error messages.
    //--------------------------------------------------------------------------
    std::string const& name() const { return m_name; }

    //--------------------------------------------------------------------------
    //! \brief The source of a given one based line, without its newline, or an
    //! empty string when out of range. Used to render an error in context.
    //--------------------------------------------------------------------------
    std::string sourceLine(uint32_t line) const;

    //--------------------------------------------------------------------------
    //! \brief Number of tokens read.
    //--------------------------------------------------------------------------
    size_t size() const { return m_tokens.size(); }

private:

    //--------------------------------------------------------------------------
    //! \brief Split the source into tokens, dropping the comments.
    //--------------------------------------------------------------------------
    void tokenize(std::string const& source);

private:

    std::string m_name;
    std::vector<std::string> m_lines;
    std::vector<Token> m_tokens;
    size_t m_index = 0u;
    //! \brief Returned past the end of the input, and by current() before the
    //! first next(). Holds the position of the end of the source so that a
    //! truncated script still reports a sensible place.
    Token m_end;
};

} // namespace ogb

#endif
