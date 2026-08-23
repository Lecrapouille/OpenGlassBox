//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file CitySave.hpp
//! \brief Load and write a city save (.ogc): header, geometry and live state.

#ifndef OPENGLASSBOX_DEMO_CITY_SAVE_HPP
#define OPENGLASSBOX_DEMO_CITY_SAVE_HPP

#include <string>
#include <vector>

namespace ogb
{

class Simulation;

//==============================================================================
//! \brief What a save claims about the ruleset it was written against.
//!
//! A save holds no rule of its own: it holds roads, buildings and the names of
//! the types they were built from. Those types live in the ruleset, so a save is
//! only meaningful together with the script it was written against, and this is
//! what lets the loader check that before rebuilding anything.
//==============================================================================
struct CitySaveHeader
{
    //! \brief Path of the ruleset the save was written against, as it was
    //! spelled when it was written.
    std::string ruleset;

    //! \brief Fingerprint of that ruleset at the time of writing. Compared
    //! against the file on disk before the save is loaded.
    std::string hash;

    //! \brief Names of every type the save refers to: kinds of building, of
    //! segment, of network, of layer, of zone. Each one has to exist in the
    //! ruleset for the save to be loadable.
    std::vector<std::string> types;
};

//==============================================================================
//! \brief Reader and writer of the \c .ogc city save format.
//!
//! A plain text format holding the state of a game: where the roads are, which
//! buildings stand on them, what each one holds, where the agents are, what the
//! layers of the environment look like, and what time it is. What it does not
//! hold is the rules, which stay in the \c .ogs ruleset next to it.
//!
//! That split is why every entry point here deals with the ruleset as well:
//! loading a save into the wrong ruleset would rebuild the same geometry out of
//! different types, which is worse than refusing.
//!
//! Nothing here throws. Every call reports what went wrong through an out
//! parameter, since a bad save is something a user does, not a bug.
//!
//! Example:
//! \code
//! std::string error;
//! CitySaveHeader header;
//!
//! // Look before leaping: which ruleset does this save want?
//! if (!CitySave::peekHeader("saves/paris.ogc", header, error))
//!     return complain(error);
//!
//! Simulation simulation;
//! simulation.script().parse(header.ruleset);
//! if (!CitySave::read("saves/paris.ogc", simulation, error))
//!     return complain(error);
//! \endcode
//==============================================================================
class CitySave
{
public:

    // -------------------------------------------------------------------------
    //! \brief Fingerprint of a file on disk, which is what a save stores to
    //! recognise its ruleset again.
    //! \param[in] path the file to read.
    //! \return the fingerprint, or an empty string when the file cannot be read.
    // -------------------------------------------------------------------------
    static std::string hashFile(std::string const& path);

    // -------------------------------------------------------------------------
    //! \brief The same fingerprint, over text already in memory. What the editor
    //! of the demo uses to work out the fingerprint of a script the player is
    //! still typing.
    //! \param[in] text the text to fingerprint.
    //! \return the fingerprint.
    // -------------------------------------------------------------------------
    static std::string hashString(std::string const& text);

    // -------------------------------------------------------------------------
    //! \brief Read only the header of a save, without touching the game.
    //!
    //! This is what lets a host load the right ruleset first: a save names the
    //! script it belongs to, so there is no need to ask the player for it.
    //!
    //! \param[in] path the save to read.
    //! \param[out] header what the save claims. Untouched on failure.
    //! \param[out] error why it failed, in words meant for a user.
    //! \return false when the file cannot be read or does not look like a save.
    // -------------------------------------------------------------------------
    static bool peekHeader(std::string const& path,
                           CitySaveHeader& header,
                           std::string& error);

    // -------------------------------------------------------------------------
    //! \brief Does the ruleset on disk still match the one the save was written
    //! against?
    //!
    //! A mismatch is a flat refusal rather than a warning: the geometry would be
    //! rebuilt out of the wrong types, giving a town that looks right and
    //! behaves like something else.
    //!
    //! \param[in] header what the save claims, from peekHeader().
    //! \param[in] rulesetPath the script to check against.
    //! \param[out] error why they do not match.
    //! \return true when the fingerprints agree.
    // -------------------------------------------------------------------------
    static bool matchesRuleset(CitySaveHeader const& header,
                               std::string const& rulesetPath,
                               std::string& error);

    // -------------------------------------------------------------------------
    //! \brief Write the state of a game out.
    //!
    //! Everything is written: the roads, the buildings and what they hold, the
    //! agents and where they are along their itinerary, the layers, and the
    //! clock. The traffic averages of the streets go out too, so that a loaded
    //! town does not start with every road looking empty.
    //!
    //! \param[in] path the file to write. Overwritten if it exists.
    //! \param[in] simulation the game to write out.
    //! \param[in] rulesetPath the script the game was loaded from, fingerprinted
    //! into the header.
    //! \param[out] error why it failed.
    //! \return false when the file cannot be written.
    // -------------------------------------------------------------------------
    static bool write(std::string const& path,
                      Simulation const& simulation,
                      std::string const& rulesetPath,
                      std::string& error);

    // -------------------------------------------------------------------------
    //! \brief Fill a game from a save.
    //!
    //! The ruleset has to be parsed into the Simulation beforehand: every type
    //! the save names is looked up in it, and a missing one stops the load with
    //! an error saying which. Whatever the game already held is replaced.
    //!
    //! \param[in] path the save to read.
    //! \param[in,out] simulation the game to fill, already carrying its ruleset.
    //! \param[out] error why it failed, naming the missing type or the line at
    //! fault.
    //! \return false when the save cannot be read or does not fit the ruleset,
    //! in which case the game is left in whatever state the load reached.
    // -------------------------------------------------------------------------
    static bool
    read(std::string const& path, Simulation& simulation, std::string& error);
};

} // namespace ogb

#endif
