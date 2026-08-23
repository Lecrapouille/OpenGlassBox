//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Script/Lexer.hpp"

#include <fstream>
#include <sstream>

//! \brief Characters that are a token on their own even when written against a
//! word. Writing "[People 1]" then reads the same as "[ People 1 ]".
namespace ogb
{

static bool isDelimiter(char c)
{
    return (c == '[') || (c == ']');
}

// -----------------------------------------------------------------------------
bool Lexer::openFile(std::string const& filename)
{
    std::ifstream file(filename);
    if (!file)
        return false;

    std::stringstream buffer;
    buffer << file.rdbuf();

    openString(buffer.str(), filename);

    return true;
}

// -----------------------------------------------------------------------------
void Lexer::openString(std::string const& source, std::string const& name)
{
    m_name = name;
    m_lines.clear();
    m_tokens.clear();
    m_index = 0u;

    tokenize(source);
}

// -----------------------------------------------------------------------------
void Lexer::tokenize(std::string const& source)
{
    uint32_t line = 1u;
    uint32_t column = 1u;
    std::string word;
    uint32_t wordLine = 1u;
    uint32_t wordColumn = 1u;
    std::string currentLine;
    bool inComment = false;

    auto flush = [&]()
    {
        if (word.empty())
            return;

        m_tokens.push_back(Token{ word, wordLine, wordColumn });
        word.clear();
    };

    for (char const c : source)
    {
        if (c == '\n')
        {
            flush();
            m_lines.push_back(currentLine);
            currentLine.clear();
            inComment = false;
            ++line;
            column = 1u;
            continue;
        }

        currentLine.push_back(c);

        if (inComment)
        {
            ++column;
            continue;
        }

        // A hash comments out the rest of the line, the convention of every
        // configuration format the author of a script is likely to know.
        if (c == '#')
        {
            flush();
            inComment = true;
            ++column;
            continue;
        }

        if (isspace(static_cast<unsigned char>(c)))
        {
            flush();
        }
        else if (isDelimiter(c))
        {
            flush();
            m_tokens.push_back(Token{ std::string(1u, c), line, column });
        }
        else
        {
            if (word.empty())
            {
                wordLine = line;
                wordColumn = column;
            }
            word.push_back(c);
        }

        ++column;
    }

    flush();
    m_lines.push_back(currentLine);

    // Anything reported past the end of the input points at the end of the
    // last line, which is where the missing token should have been.
    m_end.text.clear();
    m_end.line = line;
    m_end.column = column;
}

// -----------------------------------------------------------------------------
Token const& Lexer::next()
{
    if (m_index >= m_tokens.size())
    {
        m_index = m_tokens.size() + 1u;
        return m_end;
    }

    return m_tokens[m_index++];
}

// -----------------------------------------------------------------------------
Token const& Lexer::peek() const
{
    if (m_index >= m_tokens.size())
        return m_end;

    return m_tokens[m_index];
}

// -----------------------------------------------------------------------------
Token const& Lexer::current() const
{
    if ((m_index == 0u) || (m_index > m_tokens.size()))
        return m_end;

    return m_tokens[m_index - 1u];
}

// -----------------------------------------------------------------------------
void Lexer::rewind()
{
    m_index = 0u;
}

// -----------------------------------------------------------------------------
std::string Lexer::sourceLine(uint32_t line) const
{
    if ((line == 0u) || (line > m_lines.size()))
        return {};

    return m_lines[line - 1u];
}

} // namespace ogb
