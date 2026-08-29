//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Name.hpp
//! \brief Interned string: a name compared by number, not by text.

#ifndef OPEN_GLASSBOX_NAME_HPP
#define OPEN_GLASSBOX_NAME_HPP

#include <cstdint>
#include <ostream>
#include <string>

namespace ogb
{

//==============================================================================
//! \brief A ruleset name stored as a number, not as characters.
//!
//! The script names resources, Building types, and what an Agent seeks. These names
//! are compared often in hot loops. The router checks every Building it passes.
//!
//! A Name stores each text once in a shared table. Comparisons use the table
//! index. Two Names compare as two integers. Copying one costs four bytes.
//!
//! It converts to and from \c std::string, so literals still work:
//!
//! \code
//! ogb::Name const people = "People";
//! if (building.accepts("Work", agent.resources())) { ... }
//! std::cout << people << " " << people.str().size() << '\n';
//! \endcode
//!
//! The text-to-Name conversion is slow (hash lookup). Do it when the ruleset
//! loads, not in a loop. The same text always gives the same Name.
//!
//! \note Names are never removed. A Name stays valid for the process lifetime.
//! Loading a new ruleset keeps old names in memory. For a few dozen names this
//! is acceptable.
//!
//! \note The table is not thread-safe. Do not intern from several threads at
//! once. Reading an already interned Name from several threads is safe.
//==============================================================================
class Name
{
public:

    //--------------------------------------------------------------------------
    //! \brief The empty name. Same as Name(""). Compares equal to nothing else.
    //--------------------------------------------------------------------------
    Name() = default;

    //--------------------------------------------------------------------------
    //! \brief Intern a name given as a literal.
    //! \param[in] text the name. A null pointer reads as the empty name.
    //--------------------------------------------------------------------------
    Name(char const* text); // NOSONAR: no explicit constructor needed

    //--------------------------------------------------------------------------
    //! \brief Intern a name given as a string.
    //! \param[in] text the name.
    //--------------------------------------------------------------------------
    Name(std::string const& text); // NOSONAR: no explicit constructor needed

    //--------------------------------------------------------------------------
    //! \return the text for this name. Valid for the process lifetime.
    //--------------------------------------------------------------------------
    std::string const& str() const;

    //--------------------------------------------------------------------------
    //! \return the text as a C string. Used by printf-style calls in the demo.
    //--------------------------------------------------------------------------
    char const* c_str() const
    {
        return str().c_str();
    }

    //--------------------------------------------------------------------------
    //! \brief Converts to string for print, concat, or save.
    //--------------------------------------------------------------------------
    explicit operator std::string const&() const
    {
        return str();
    }

    //--------------------------------------------------------------------------
    //! \return the index in the table. Zero is the empty name.
    //! Use this to index arrays keyed by Name.
    //--------------------------------------------------------------------------
    [[nodiscard]] uint32_t getId() const
    {
        return m_id;
    }

    //--------------------------------------------------------------------------
    //! \return whether the name is the empty one.
    //--------------------------------------------------------------------------
    bool empty() const
    {
        return m_id == 0u;
    }

    //--------------------------------------------------------------------------
    //! \return true if the two names are the same. One integer comparison.
    //--------------------------------------------------------------------------
    bool operator==(Name const& other) const noexcept
    {
        return m_id == other.m_id;
    }

    //! \copydoc operator==
    bool operator!=(Name const& other) const noexcept
    {
        return m_id != other.m_id;
    }

    //--------------------------------------------------------------------------
    //! \brief Sort by text, not by table index.
    //! Containers keyed by Name appear in alphabetical order.
    //--------------------------------------------------------------------------
    bool operator<(Name const& other) const
    {
        return (m_id != other.m_id) && (str() < other.str());
    }

    //--------------------------------------------------------------------------
    //! \return how many distinct names were interned, including the empty one.
    //--------------------------------------------------------------------------
    static size_t count();

private:

    //! \brief Index in the name table. Zero is the empty name.
    //! A default Name needs no constructor.
    uint32_t m_id = 0u;
};

//! \brief Write the name text to a stream.
std::ostream& operator<<(std::ostream& os, Name const& name);

//! \brief Compare a Name to a literal without interning the literal twice.
//! Used by unit test assertions.
bool operator==(Name const& name, char const* text);

//! \copydoc operator==(Name const&, char const*)
bool operator==(char const* text, Name const& name);

//! \copydoc operator==(Name const&, char const*)
bool operator!=(Name const& name, char const* text);

//! \copydoc operator==(Name const&, char const*)
bool operator!=(char const* text, Name const& name);

//! \copydoc operator==(Name const&, char const*)
bool operator==(Name const& name, std::string const& text);

//! \copydoc operator==(Name const&, char const*)
bool operator==(std::string const& text, Name const& name);

//! \copydoc operator==(Name const&, char const*)
bool operator!=(Name const& name, std::string const& text);

//! \copydoc operator==(Name const&, char const*)
bool operator!=(std::string const& text, Name const& name);

//! \brief Compare a Name to a string by text.
//! Lets a Name look up a container keyed by \c std::string, as ScriptDefinitions does.
bool operator<(Name const& name, std::string const& text);

//! \copydoc operator<(Name const&, std::string const&)
bool operator<(std::string const& text, Name const& name);

} // namespace ogb

namespace std
{
//! \brief Hash a Name by its table index. Use in unordered containers.
template <>
struct hash<ogb::Name>
{
    size_t operator()(ogb::Name const& name) const noexcept
    {
        return std::hash<uint32_t>()(name.getId());
    }
};
} // namespace std

#endif
