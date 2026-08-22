#include "main.hpp"

#include "OpenGlassBox/CitySave.hpp"
#include "OpenGlassBox/Simulation.hpp"

#include <fstream>

static std::string testCityRuleset()
{
    char const* candidates[] = {
        "../demo/data/Simulations/test_city.ogs",
        "demo/data/Simulations/test_city.ogs",
    };
    for (char const* path: candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

static std::string testCitySave()
{
    char const* candidates[] = {
        "../demo/data/Simulations/test_city.ogc",
        "demo/data/Simulations/test_city.ogc",
    };
    for (char const* path: candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

TEST(TestsCitySave, LoadShippedParis)
{
    Simulation simulation;
    ASSERT_TRUE(simulation.script().parse(testCityRuleset()));

    CitySaveHeader header;
    std::string error;
    ASSERT_TRUE(CitySave::peekHeader(testCitySave(), header, error)) << error;
    ASSERT_EQ(header.ruleset, "test_city.ogs");
    ASSERT_TRUE(CitySave::matchesRuleset(header, testCityRuleset(), error)) << error;
    ASSERT_TRUE(CitySave::read(testCitySave(), simulation, error)) << error;

    ASSERT_EQ(simulation.cities().size(), 1u);
    City& city = *simulation.cities().begin()->second;
    ASSERT_STREQ(city.name().c_str(), "Paris");
    ASSERT_FALSE(city.paths().empty());
    ASSERT_GE(city.units().size(), 5u);
    ASSERT_FALSE(city.areas().empty());
}

TEST(TestsCitySave, MissingTypeIsRefused)
{
    Simulation simulation;
    ASSERT_TRUE(simulation.script().parse(testCityRuleset()));

    std::string const path = "/tmp/openglassbox-missing-type.ogc";
    {
        std::ofstream out(path);
        out << "save\n"
            << "\truleset test_city.ogs\n"
            << "\thash deadbeef\n"
            << "\ttypes [ Asphalt ]\n"
            << "end\n"
            << "clock 0\n"
            << "city Ghost size 4 4\n";
    }

    std::string error;
    ASSERT_FALSE(CitySave::read(path, simulation, error));
    ASSERT_NE(error.find("Asphalt"), std::string::npos);
}

TEST(TestsCitySave, LoadShippedBraessAndGrids)
{
    struct Case { char const* ogs; char const* ogc; };
    Case const cases[] = {
        { "demo/data/Simulations/braess.ogs", "demo/data/Simulations/braess.ogc" },
        { "demo/data/Simulations/regular.ogs", "demo/data/Simulations/regular.ogc" },
        { "demo/data/Simulations/chicago.ogs", "demo/data/Simulations/chicago.ogc" },
    };

    for (Case const& c: cases)
    {
        Simulation simulation;
        ASSERT_TRUE(simulation.script().parse(c.ogs)) << c.ogs;
        CitySaveHeader header;
        std::string error;
        ASSERT_TRUE(CitySave::peekHeader(c.ogc, header, error)) << error;
        ASSERT_TRUE(CitySave::matchesRuleset(header, c.ogs, error)) << error;
        ASSERT_TRUE(CitySave::read(c.ogc, simulation, error)) << c.ogc << ": " << error;
        ASSERT_FALSE(simulation.cities().empty()) << c.ogc;
    }
}

TEST(TestsCitySave, HashMismatch)
{
    CitySaveHeader header;
    header.ruleset = "test_city.ogs";
    header.hash = "not-the-real-hash";
    std::string error;
    ASSERT_FALSE(CitySave::matchesRuleset(header, testCityRuleset(), error));
    ASSERT_NE(error.find("hash"), std::string::npos);
}
