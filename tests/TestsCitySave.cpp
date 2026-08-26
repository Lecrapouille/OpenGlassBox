#include "main.hpp"

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "Save/CitySave.hpp"

#include <fstream>

static std::string testCityRuleset()
{
    std::vector<std::string> candidates = {
        "../demo/data/Simulations/test_city.ogs",
        "demo/data/Simulations/test_city.ogs",
    };
    for (std::string const& path : candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

static std::string testCitySave()
{
    std::vector<std::string> candidates = {
        "../demo/data/Simulations/test_city.ogc",
        "demo/data/Simulations/test_city.ogc",
    };
    for (std::string const& path : candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

TEST(TestsCitySave, LoadShippedParis)
{
    Simulation simulation{ 32u, 32u };
    ASSERT_TRUE(simulation.script().parseFile(testCityRuleset()));

    CitySaveHeader header;
    std::string error;
    ASSERT_TRUE(CitySave::peekHeader(testCitySave(), header, error)) << error;
    ASSERT_EQ(header.ruleset, "test_city.ogs");
    ASSERT_TRUE(CitySave::matchesRuleset(header, testCityRuleset(), error))
        << error;
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
    Simulation simulation{ 32u, 32u };
    ASSERT_TRUE(simulation.script().parseFile(testCityRuleset()));

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

//! \brief Every shipped save opens against the ruleset it names. Which one that
//! is comes from the header rather than from a table here: a city saved from
//! the demo records whatever ruleset was open at the time, and hard coding the
//! pairs only made this test fail whenever one of them was re-saved.
TEST(TestsCitySave, LoadShippedBraessAndGrids)
{
    std::vector<std::string> saves = {
        "demo/data/Simulations/braess.ogc",
        "demo/data/Simulations/regular.ogc",
        "demo/data/Simulations/chicago.ogc",
    };

    for (std::string const& save : saves)
    {
        CitySaveHeader header;
        std::string error;
        ASSERT_TRUE(CitySave::peekHeader(save, header, error)) << error;

        std::string const ruleset = "demo/data/Simulations/" + header.ruleset;
        Simulation simulation{ 32u, 32u };
        ASSERT_TRUE(simulation.script().parseFile(ruleset)) << ruleset;
        ASSERT_TRUE(CitySave::matchesRuleset(header, ruleset, error)) << error;
        ASSERT_TRUE(CitySave::read(save, simulation, error))
            << save << ": " << error;
        installDijkstraRouters(simulation);
        ASSERT_FALSE(simulation.cities().empty()) << save;
    }
}

//------------------------------------------------------------------------------
//! \brief A save records what a building holds, never how much it can hold: the
//! capacities belong to the ruleset. Loading used to assign the whole bin, so
//! every building came back with a capacity of zero and refused every Agent and
//! every rule that adds anything.
TEST(TestsCitySave, LoadedUnitsKeepTheCapacitiesOfTheirType)
{
    Simulation simulation{ 32u, 32u };
    ASSERT_TRUE(simulation.script().parseFile(testCityRuleset()));

    std::string error;
    ASSERT_TRUE(CitySave::read(testCitySave(), simulation, error)) << error;

    City& city = *simulation.cities().begin()->second;
    uint32_t checked = 0u;
    for (auto& unit : city.units())
    {
        UnitType const& type = simulation.script().getUnitType(unit->type());
        for (Resource const& capped : type.resources.container())
        {
            ASSERT_EQ(unit->resources().getCapacity(capped.type()),
                      capped.getCapacity())
                << unit->type() << " lost the capacity of " << capped.type();
            ++checked;
        }
    }
    ASSERT_GT(checked, 0u);
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
