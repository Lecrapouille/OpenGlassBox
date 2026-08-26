//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#ifndef OPEN_GLASSBOX_TESTS_TEST_DATA_PATH_HPP
#define OPEN_GLASSBOX_TESTS_TEST_DATA_PATH_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

inline std::string testDataPath(std::string const& name)
{
    std::vector<std::string> const candidates = {
        "../demo/data/Simulations/" + name,
        "demo/data/Simulations/" + name,
    };
    for (std::string const& path : candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

inline std::string tempTestPath(std::string const& name)
{
    return (std::filesystem::temp_directory_path() / name).string();
}

#endif
