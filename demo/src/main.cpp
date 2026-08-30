// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-OpenGlassBox-Commercial
// Copyright (c) 2020-2026 Quentin Quadrat
//
// This file is part of OpenGlassBox. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Game/GlassBoxApp.hpp"

#include <iostream>
#include <string>

static void usage(char const* program)
{
    std::cout
        << "Usage: " << program
        << " [file]\n"
           "\n"
           "  file                    optional .ogs ruleset or .ogc city "
           "save.\n"
           "                          Resolved as given if the path exists,\n"
           "                          otherwise under the build-time data "
           "path\n"
           "                          in simulations/. Defaults to "
           "sandbox.ogs\n"
           "                          then its sibling sandbox.ogc.\n"
           "\n"
           "  -h, --help              show this message\n";
}

static bool parseArguments(int argc,
                           char* argv[],
                           ogb::game::GlassBoxApp::Options& options,
                           int& exitCode)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string const argument(argv[i]);

        if ((argument == "-h") || (argument == "--help"))
        {
            usage(argv[0]);
            exitCode = EXIT_SUCCESS;
            return false;
        }

        if (argument.compare(0u, 2u, "--") == 0)
        {
            std::cerr << "Unknown option '" << argument << "'" << std::endl;
            usage(argv[0]);
            exitCode = EXIT_FAILURE;
            return false;
        }

        options.file = argument;
    }

    return true;
}

int main(int argc, char* argv[])
{
    ogb::game::GlassBoxApp::Options options;
    int exitCode = EXIT_SUCCESS;

    if (!parseArguments(argc, argv, options, exitCode))
        return exitCode;

    ogb::game::GlassBoxApp application(std::move(options));
    if (!application.run())
    {
        std::cerr << "Failure: " << application.error() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
