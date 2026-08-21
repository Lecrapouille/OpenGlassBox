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

// *****************************************************************************
//! \brief Class manipulating a set of paths for searching files in the same
//! idea of the Unix environment variable $PATH. Paths are separated by ':' and
//! search is made left to right. Ie. "/foo/bar:/usr/lib/"
// *****************************************************************************
class DataPath
{
public:

    //--------------------------------------------------------------------------
    //! \brief Empty search path.
    //--------------------------------------------------------------------------
    DataPath() = default;

    //--------------------------------------------------------------------------
    //! \brief Constructor with a given path. Directories shall be separated
    //! by the character given by the param delimiter.
    //! Example: Path("/foo/bar:/usr/lib/", ':').
    //--------------------------------------------------------------------------
    explicit DataPath(std::string const& path, char const delimiter = ':');

    //--------------------------------------------------------------------------
    //! \brief Destructor.
    //--------------------------------------------------------------------------
    ~DataPath() = default;

    //--------------------------------------------------------------------------
    //! \brief Build the search path used by the demo, from the highest to the
    //! lowest priority:
    //!  -# the directories given on the command line with --data-path,
    //!  -# the directories held by the OPENGLASSBOX_DATA_PATH environment
    //!     variable,
    //!  -# the data folder shipped next to the executable, which covers both an
    //!     installed build and a run from the build directory,
    //!  -# the directories baked in at build time by MyMakefile.
    //!
    //! \param[in] commandLinePath: the value of --data-path, may be empty.
    //--------------------------------------------------------------------------
    static DataPath makeDefault(std::string const& commandLinePath);

    //--------------------------------------------------------------------------
    //! \brief Return the absolute path of the directory holding the running
    //! executable, with a trailing separator, or an empty string when the
    //! platform does not allow to retrieve it.
    //--------------------------------------------------------------------------
    static std::string executableDirectory();

    //--------------------------------------------------------------------------
    //! \brief Append a new path at the end of the search path, therefore with
    //! the lowest priority. Directories are separated by the delimiter char (by
    //! default ':'). Example: add("/foo/bar:/usr/lib/").
    //--------------------------------------------------------------------------
    void add(std::string const& path);

    //--------------------------------------------------------------------------
    //! \brief Insert a new path at the beginning of the search path, therefore
    //! with the highest priority.
    //--------------------------------------------------------------------------
    void prepend(std::string const& path);

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
    //! \brief Return pathes as a delimiter separated string.
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
    //! \brief Split paths separated by delimiter char and insert them either at
    //! the front or at the back of the search path.
    //--------------------------------------------------------------------------
    void split(std::string const& path, bool front);

    //--------------------------------------------------------------------------
    //! \brief Return true if the path exists. be careful the file may not
    //! exist after the function ends.
    //--------------------------------------------------------------------------
    bool exist(std::string const& path) const;

protected:

    //! \brief Path separator when several pathes are given as a single string.
    char m_delimiter = ':';
    //! \brief the list of pathes.
    std::list<std::string> m_search_paths;
};

#endif // DATA_PATH_HPP
