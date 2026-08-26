//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file Name.hpp
//! \brief An interned string: a name the simulation compares by number.

#ifndef OPEN_GLASSBOX_NAME_HPP
#define OPEN_GLASSBOX_NAME_HPP

#include <cstdint>
#include <ostream>
#include <string>

namespace ogb
{

//==============================================================================
//! \brief A name of the ruleset, held as a number rather than as characters.
//!
//! Everything a script names -- resources, types of building, what an Agent is
//! looking for -- is a short string drawn from a list the ruleset fixes once
//! and never adds to afterwards. Yet those strings are compared in the hottest
//! loops there are: the router asks every building it walks past whether it
//! accepts what an Agent carries, and that question is a handful of string
//! comparisons.
//!
//! A Name is the answer: the text is stored once in a table shared by the whole
//! process, and what gets passed around and compared is its rank in that table.
//! Comparing two Names is comparing two integers, and copying one costs four
//! bytes rather than a heap block.
//!
//! It converts to and from \c std::string on its own, so a call site that reads
//! naturally with a literal keeps reading that way:
//!
//! \code
//! ogb::Name const people = "People";
//! if (unit.accepts("Work", agent.resources())) { ... }
//! std::cout << people << " " << people.str().size() << '\n';
//! \endcode
//!
//! The conversion from text is the expensive one -- a hash lookup -- so it
//! belongs where the ruleset is read, not in a loop. Interning the same text
//! twice gives the same Name, which is what makes equality trustworthy.
//!
//! \note Names are never removed from the table, so a Name stays valid for as
//! long as the process lives. Loading one ruleset after another leaks the names
//! of the first, which for a list of a few dozen words is not worth reclaiming.
//!
//! \note The table is not guarded by a lock. Interning from several threads at
//! once is not supported; reading a Name already interned is.
//==============================================================================
class Name
{
public:

    //--------------------------------------------------------------------------
    //! \brief The empty name, which is what an unset field reads as. Compares
    //! equal to Name("") and to nothing else.
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
    //! \brief \return the text behind the name. Valid for the life of the
    //! process.
    //--------------------------------------------------------------------------
    std::string const& str() const;

    //--------------------------------------------------------------------------
    //! \brief \return the text as a C string, for the printf-shaped calls of
    //! the demo.
    //--------------------------------------------------------------------------
    char const* c_str() const
    {
        return str().c_str();
    }

    //--------------------------------------------------------------------------
    //! \brief Read as a string wherever one is expected, so that a Name can be
    //! printed, concatenated or written to a save without ceremony.
    //--------------------------------------------------------------------------
    operator std::string const&() const
    {
        return str();
    }

    //--------------------------------------------------------------------------
    //! \brief \return the rank in the table, which is what makes a Name usable
    //! as an index. Zero is the empty name.
    //--------------------------------------------------------------------------
    uint32_t id() const
    {
        return m_id;
    }

    //--------------------------------------------------------------------------
    //! \brief \return whether the name is the empty one.
    //--------------------------------------------------------------------------
    bool empty() const
    {
        return m_id == 0u;
    }

    //--------------------------------------------------------------------------
    //! \brief \return whether the two names are the same word. One integer
    //! comparison, which is the whole point of the class.
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
    //! \brief Order by the text, so that a container keyed by Name comes out in
    //! the order a reader expects rather than in the order the ruleset happened
    //! to mention the names.
    //--------------------------------------------------------------------------
    bool operator<(Name const& other) const
    {
        return (m_id != other.m_id) && (str() < other.str());
    }

    //--------------------------------------------------------------------------
    //! \brief \return how many distinct names have been interned, the empty one
    //! included. The bound on id().
    //--------------------------------------------------------------------------
    static size_t count();

private:

    //! \brief Rank in the table of names. Zero is the empty name, which is why
    //! a default built Name needs no constructor to be valid.
    uint32_t m_id = 0u;
};

//! \brief Write the text behind the name.
std::ostream& operator<<(std::ostream& os, Name const& name);

//! \brief Compare a name against a literal without interning the literal twice
//! over. Present so that the assertions of the unit tests read naturally.
bool operator==(Name const& name, char const* text);

//! \copydoc operator==(Name const&, char const*)
bool operator==(char const* text, Name const& name);

//! \copydoc operator==(Name const&, char const*)
bool operator!=(Name const& name, char const* text);

//! \copydoc operator==(Name const&, char const*)
bool operator!=(char const* text, Name const& name);

//! \brief Order a name against a plain string by their texts. What lets a Name
//! look up a container keyed by \c std::string with a transparent comparator,
//! as ScriptDefinitions does.
bool operator<(Name const& name, std::string const& text);

//! \copydoc operator<(Name const&, std::string const&)
bool operator<(std::string const& text, Name const& name);

} // namespace ogb

namespace std
{
//! \brief Hash a Name by its rank, so that it can key an unordered container.
template <>
struct hash<ogb::Name>
{
    size_t operator()(ogb::Name const& name) const noexcept
    {
        return std::hash<uint32_t>()(name.id());
    }
};
} // namespace std

#endif
