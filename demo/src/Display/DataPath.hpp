//=====================================================================
// MyLogger: A basic logger.
// Copyright 2018 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of MyLogger.
//
// MyLogger is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MyLogger.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef DATA_PATH_HPP
#  define DATA_PATH_HPP

#  include <list>
#  include <string>
#  include <vector>
#  include <fstream>

//------------------------------------------------------------------------------
#  if defined(__APPLE__)
std::string osx_get_resources_dir(std::string const& file);
#  endif

// *****************************************************************************
//! \brief Class manipulating a set of paths for searching files in the same
//! idea of the Unix environment variable $PATH. Paths are separated by ':' and
//! search is made left to right. Ie. "/foo/bar:/usr/lib/"
// *****************************************************************************
class DataPath
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor with a given path. Directories shall be separated
    //! by the character given by the param delimiter.
    //! Example: Path("/foo/bar:/usr/lib/", ':').
    //--------------------------------------------------------------------------
    DataPath(std::string const& path, char const delimiter = ':');

    //--------------------------------------------------------------------------
    //! \brief Destructor.
    //--------------------------------------------------------------------------
    ~DataPath() = default;

    //--------------------------------------------------------------------------
    //! \brief Append a new path. Directories are separated by the delimiter char
    //! (by default ':'). Example: add("/foo/bar:/usr/lib/").
    //--------------------------------------------------------------------------
    void add(std::string const& path);

    //--------------------------------------------------------------------------
    //! \brief Replace the path state by a new one. Directories are separated by
    //! the delimiter char (by default ':'). Example: reset("/foo/bar:/usr/lib/").
    //--------------------------------------------------------------------------
    void reset(std::string const& path);

    //--------------------------------------------------------------------------
    //! \brief Erase the path.
    //--------------------------------------------------------------------------
    void clear();

    //--------------------------------------------------------------------------
    //! \brief Erase the given directory from the path if found.
    //--------------------------------------------------------------------------
    void remove(std::string const& path);

    //--------------------------------------------------------------------------
    //! \brief Find if a file exists in the search path. Note that you have to
    //! check again the existence of this file when opening it (with functions
    //! such as iofstream, fopen, open ...). Indeed the file may have been
    //! suppress since this method have bee called.
    //!
    //! \return the full path (if found) and the existence of this path.
    //!
    //--------------------------------------------------------------------------
    std::pair<std::string, bool> find(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Return the full path for the file (if found) else return itself.
    //! Beware of race condition: even if found the file may have suppress after
    //! this function has been called.
    //--------------------------------------------------------------------------
    std::string expand(std::string const& filename) const;

    //--------------------------------------------------------------------------
    //! \brief Return the container of path
    //--------------------------------------------------------------------------
    std::vector<std::string> pathes() const;

    //--------------------------------------------------------------------------
    //! \brief Return pathes as string. The first path is always ".:"
    //--------------------------------------------------------------------------
    std::string toString() const;  

    bool open(std::string& filename, std::ifstream& ifs,
              std::ios_base::openmode mode = std::ios_base::in) const;
    bool open(std::string& filename, std::ofstream& ifs,
              std::ios_base::openmode mode = std::ios_base::out) const;
    bool open(std::string& filename, std::fstream& ifs,
              std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out) const;
protected:

    //--------------------------------------------------------------------------
    //! \brief Slipt paths separated by delimiter char into std::list
    //--------------------------------------------------------------------------
    void split(std::string const& path);

    //--------------------------------------------------------------------------
    //! \brief Return true if the path exists. be careful the file may not
    //! exist after the function ends.
    //--------------------------------------------------------------------------
    bool exist(std::string const& path) const;

protected:

    //! \brief Path separator when several pathes are given as a single string.
    const char m_delimiter = ':';
    //! \brief the list of pathes.
    std::list<std::string> m_search_paths;
};

#endif // UTILS_PATH_HPP
