//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "OpenGlassBox/Name.hpp"

#include <deque>
#include <unordered_map>

namespace ogb
{

namespace
{

//==============================================================================
//! \brief The one table every Name refers into.
//!
//! The texts live in a deque rather than a vector so that a \c std::string
//! const& handed out by Name::str() stays put as more names are interned.
//==============================================================================
struct SymbolTable
{
    SymbolTable()
    {
        // Rank zero is the empty name, so that a default built Name is valid
        // without the table having been touched.
        texts.emplace_back();
        ids.emplace(std::string(), 0u);
    }

    //! \brief The texts, indexed by rank.
    std::deque<std::string> texts;

    //! \brief The ranks, keyed by text. Only ever read while a ruleset is being
    //! parsed or a save loaded.
    std::unordered_map<std::string, uint32_t> ids;
};

//------------------------------------------------------------------------------
SymbolTable& table()
{
    // A function local static rather than a namespace scope one: the Names held
    // by the types of a ruleset are interned from other statics in the tests,
    // and this is what makes the order of initialisation a non-question.
    static SymbolTable instance;
    return instance;
}

//------------------------------------------------------------------------------
uint32_t intern(std::string const& text)
{
    if (text.empty())
        return 0u;

    SymbolTable& symbols = table();

    auto const it = symbols.ids.find(text);
    if (it != symbols.ids.end())
        return it->second;

    auto const id = uint32_t(symbols.texts.size());
    symbols.texts.push_back(text);
    symbols.ids.emplace(text, id);

    return id;
}

} // namespace

//------------------------------------------------------------------------------
Name::Name(char const* text)
    : m_id(intern((text == nullptr) ? std::string() : std::string(text)))
{
}

//------------------------------------------------------------------------------
Name::Name(std::string const& text) : m_id(intern(text)) {}

//------------------------------------------------------------------------------
std::string const& Name::str() const
{
    return table().texts[m_id];
}

//------------------------------------------------------------------------------
size_t Name::count()
{
    return table().texts.size();
}

//------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& os, Name const& name)
{
    return os << name.str();
}

//------------------------------------------------------------------------------
bool operator==(Name const& name, char const* text)
{
    if (text == nullptr)
        return name.empty();
    return name.str() == text;
}

//------------------------------------------------------------------------------
bool operator==(char const* text, Name const& name)
{
    return name == text;
}

//------------------------------------------------------------------------------
bool operator!=(Name const& name, char const* text)
{
    return !(name == text);
}

//------------------------------------------------------------------------------
bool operator!=(char const* text, Name const& name)
{
    return !(name == text);
}

//------------------------------------------------------------------------------
bool operator==(Name const& name, std::string const& text)
{
    return name.str() == text;
}

//------------------------------------------------------------------------------
bool operator==(std::string const& text, Name const& name)
{
    return name.str() == text;
}

//------------------------------------------------------------------------------
bool operator!=(Name const& name, std::string const& text)
{
    return !(name == text);
}

//------------------------------------------------------------------------------
bool operator!=(std::string const& text, Name const& name)
{
    return !(name == text);
}

//------------------------------------------------------------------------------
bool operator<(Name const& name, std::string const& text)
{
    return name.str() < text;
}

//------------------------------------------------------------------------------
bool operator<(std::string const& text, Name const& name)
{
    return text < name.str();
}

} // namespace ogb
