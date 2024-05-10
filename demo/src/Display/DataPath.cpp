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

#include "Display/DataPath.hpp"
#include <sstream>
#include <sys/stat.h>

#ifdef __APPLE__
#  include <CoreFoundation/CFBundle.h>
#endif

//------------------------------------------------------------------------------
#ifdef __APPLE__
std::string osx_get_resources_dir(std::string const& file)
{
    struct stat exists; // folder exists ?
    std::string path;

    CFURLRef resourceURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    char resourcePath[PATH_MAX];
    if (CFURLGetFileSystemRepresentation(resourceURL, true,
                                         reinterpret_cast<UInt8 *>(resourcePath),
                                         PATH_MAX))
    {
        if (resourceURL != NULL)
        {
            CFRelease(resourceURL);
        }

        path = std::string(resourcePath) + "/" + file;
        if (stat(path.c_str(), &exists) == 0)
        {
            return path;
        }
    }

    path = "data/" + file;
    if (stat(path.c_str(), &exists) == 0)
    {
        return path;
    }

    return file;
}
#endif

//------------------------------------------------------------------------------
DataPath::DataPath(std::string const& path, char const delimiter)
  : m_delimiter(delimiter)
{
    add(path);
}

//------------------------------------------------------------------------------
void DataPath::add(std::string const& path)
{
    if (!path.empty())
    {
        split(path);
    }
}

//------------------------------------------------------------------------------
void DataPath::reset(std::string const& path)
{
    m_search_paths.clear();
    split(path);
}

//------------------------------------------------------------------------------
void DataPath::clear()
{
    m_search_paths.clear();
}

//------------------------------------------------------------------------------
void DataPath::remove(std::string const& path)
{
    m_search_paths.remove(path);
}

//------------------------------------------------------------------------------
bool DataPath::exist(std::string const& path) const
{
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

//------------------------------------------------------------------------------
std::pair<std::string, bool> DataPath::find(std::string const& filename) const
{
    if (DataPath::exist(filename))
        return std::make_pair(filename, true);

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        if (DataPath::exist(file))
            return std::make_pair(file, true);
    }

    // Not found
    return std::make_pair(std::string(), false);
}

//------------------------------------------------------------------------------
std::string DataPath::expand(std::string const& filename) const
{
    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        if (DataPath::exist(file))
            return file;
    }

    return filename;
}

//------------------------------------------------------------------------------
bool DataPath::open(std::string& filename, std::ifstream& ifs, std::ios_base::openmode mode) const
{
    ifs.open(filename.c_str(), mode);
    if (ifs)
        return true;

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        ifs.open(file.c_str(), mode);
        if (ifs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
bool DataPath::open(std::string& filename, std::ofstream& ofs, std::ios_base::openmode mode) const
{
    ofs.open(filename.c_str(), mode);
    if (ofs)
        return true;

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        ofs.open(file.c_str(), mode);
        if (ofs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
bool DataPath::open(std::string& filename, std::fstream& fs, std::ios_base::openmode mode) const
{
    fs.open(filename.c_str(), mode);
    if (fs)
        return true;

    for (auto const& it: m_search_paths)
    {
        std::string file(it + filename);
        fs.open(filename.c_str(), mode);
        if (fs)
        {
            filename = file;
            return true;
        }
    }

    // Not found
    return false;
}

//------------------------------------------------------------------------------
std::vector<std::string> DataPath::pathes() const
{
    std::vector<std::string> res;
    res.reserve(m_search_paths.size());
    for (auto const& it: m_search_paths)
    {
        res.push_back(it);
    }

    return res;
}

//------------------------------------------------------------------------------
std::string DataPath::toString() const
{
    std::string string_path;

    for (auto const& it: m_search_paths)
    {
        string_path += it;
        string_path.pop_back(); // Remove the '/' char
        string_path += m_delimiter;
    }
    string_path.pop_back();

    return string_path;
}

//------------------------------------------------------------------------------
void DataPath::split(std::string const& path)
{
    std::stringstream ss(path);
    std::string directory;

    while (std::getline(ss, directory, m_delimiter))
    {
        if (directory.empty())
            continue ;

        if ((*directory.rbegin() == '\\') || (*directory.rbegin() == '/'))
            m_search_paths.push_back(directory);
        else
            m_search_paths.push_back(directory + "/");
    }
}
