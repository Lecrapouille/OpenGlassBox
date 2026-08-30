// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#ifndef OPEN_GLASSBOX_TESTS_TEST_DATA_PATH_HPP
#define OPEN_GLASSBOX_TESTS_TEST_DATA_PATH_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

inline std::string testDataPath(std::string const& name)
{
    std::vector<std::string> const candidates = {
        "../demo/data/simulations/" + name,
        "demo/data/simulations/" + name,
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
