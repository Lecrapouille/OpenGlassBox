//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file CitySave.hpp
//! \brief Load and write a city save (.ogc): header, geometry and live state.

#ifndef OPEN_GLASSBOX_CITY_SAVE_HPP
#  define OPEN_GLASSBOX_CITY_SAVE_HPP

#  include <string>
#  include <vector>

namespace ogb {

class Simulation;

//==============================================================================
//! \brief What the save claims about the ruleset it was written against.
//==============================================================================
struct CitySaveHeader
{
    std::string ruleset;
    std::string hash;
    std::vector<std::string> types;
};

//==============================================================================
//! \brief Read and write the `.ogc` city save format.
//==============================================================================
class CitySave
{
public:

    static std::string hashFile(std::string const& path);
    static std::string hashString(std::string const& text);

    static bool peekHeader(std::string const& path, CitySaveHeader& header,
                           std::string& error);

    //! \brief Compare the hash stored in the save with the file currently on
    //! disk. A mismatch is a hard refusal: the geometry would be rebuilt with
    //! the wrong types.
    static bool matchesRuleset(CitySaveHeader const& header,
                               std::string const& rulesetPath,
                               std::string& error);

    static bool write(std::string const& path, Simulation const& simulation,
                      std::string const& rulesetPath, std::string& error);

    //! \brief Fill an already script-loaded Simulation. Types named in the
    //! header must exist; otherwise \c error explains which one is missing.
    static bool read(std::string const& path, Simulation& simulation,
                     std::string& error);
};

} // namespace ogb

#endif
