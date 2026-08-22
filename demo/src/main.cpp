//-----------------------------------------------------------------------------
// Copyright (c) 2020 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Based on https://github.com/federicodangelo/MultiAgentSimulation
// Distributed under MIT License.
//-----------------------------------------------------------------------------

#include "Core/GlassBoxApp.hpp"

#include <cstring>
#include <iostream>
#include <string>

//------------------------------------------------------------------------------
static void usage(char const* program)
{
    std::cout <<
        "Usage: " << program << " [options] [script]\n"
        "\n"
        "  script                  simulation script to load, resolved against\n"
        "                          the data path. Defaults to\n"
        "                          Simulations/TestCity.txt\n"
        "\n"
        "Options:\n"
        "  --data-path <dirs>      colon separated directories searched first\n"
        "                          for the data files\n"
        "  --size <W>x<H>          initial size of the window\n"
        "  -h, --help              show this message\n"
        "\n"
        "The data files are searched, from the highest to the lowest priority,\n"
        "in --data-path, in the OPENGLASSBOX_DATA_PATH environment variable,\n"
        "next to the executable, then in the directory chosen at build time.\n";
}

//------------------------------------------------------------------------------
//! \brief Parse the command line into the application options.
//! \return false when the program shall exit without running.
//------------------------------------------------------------------------------
static bool parseArguments(int argc, char* argv[], ogb::core::GlassBoxApp::Options& options,
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

        if (argument == "--data-path")
        {
            if (++i >= argc)
            {
                std::cerr << "Missing directory after --data-path" << std::endl;
                exitCode = EXIT_FAILURE;
                return false;
            }
            options.dataPath = argv[i];
            continue;
        }

        if (argument == "--size")
        {
            int width = 0;
            int height = 0;
            if ((++i >= argc) ||
                (std::sscanf(argv[i], "%dx%d", &width, &height) != 2) ||
                (width <= 0) || (height <= 0))
            {
                std::cerr << "Expected --size <width>x<height>" << std::endl;
                exitCode = EXIT_FAILURE;
                return false;
            }
            options.width = width;
            options.height = height;
            continue;
        }

        if (argument.compare(0u, 2u, "--") == 0)
        {
            std::cerr << "Unknown option '" << argument << "'" << std::endl;
            usage(argv[0]);
            exitCode = EXIT_FAILURE;
            return false;
        }

        options.script = argument;
    }

    return true;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    ogb::core::GlassBoxApp::Options options;
    int exitCode = EXIT_SUCCESS;

    if (!parseArguments(argc, argv, options, exitCode))
        return exitCode;

    ogb::core::GlassBoxApp application(std::move(options));
    if (!application.run())
    {
        std::cerr << "Failure: " << application.error() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
