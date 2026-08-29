#include "main.hpp"

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "Save/CitySave.hpp"
#include "TestDataPath.hpp"

#include <fstream>

static std::string testCityRuleset()
{
    return testDataPath("test_city.ogs");
}

static std::string testCitySave()
{
    return testDataPath("test_city.ogc");
}

TEST(TestsCitySave, LoadShippedParis)
{
    Simulation simulation;
    ASSERT_TRUE(simulation.loadScriptFile(testCityRuleset()));

    CitySaveHeader header;
    std::string error;
    ASSERT_TRUE(CitySave::peekHeader(testCitySave(), header, error)) << error;
    ASSERT_EQ(header.ruleset, "test_city.ogs");
    ASSERT_TRUE(CitySave::matchesRuleset(header, testCityRuleset(), error))
        << error;
    ASSERT_TRUE(CitySave::read(testCitySave(), simulation, error)) << error;

    ASSERT_EQ(simulation.getCities().size(), 1u);
    City& city = *simulation.getCities().begin()->second;
    ASSERT_STREQ(city.getName().c_str(), "Paris");
    ASSERT_FALSE(city.getPaths().empty());
    ASSERT_GE(city.getBuildings().size(), 5u);
    ASSERT_FALSE(city.getZones().empty());
}

TEST(TestsCitySave, MissingTypeIsRefused)
{
    Simulation simulation;
    ASSERT_TRUE(simulation.loadScriptFile(testCityRuleset()));

    std::string const path = tempTestPath("openglassbox-missing-type.ogc");
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
        testDataPath("braess.ogc"),
        testDataPath("regular.ogc"),
        testDataPath("chicago.ogc"),
    };

    for (std::string const& save : saves)
    {
        CitySaveHeader header;
        std::string error;
        ASSERT_TRUE(CitySave::peekHeader(save, header, error)) << error;

        std::string const ruleset = testDataPath(header.ruleset);
        Simulation simulation;
        ASSERT_TRUE(simulation.loadScriptFile(ruleset)) << ruleset;
        ASSERT_TRUE(CitySave::matchesRuleset(header, ruleset, error)) << error;
        ASSERT_TRUE(CitySave::read(save, simulation, error))
            << save << ": " << error;
        installDijkstraRouters(simulation);
        ASSERT_FALSE(simulation.getCities().empty()) << save;
    }
}

//------------------------------------------------------------------------------
//! \brief A save records what a building holds, never how much it can hold: the
//! capacities belong to the ruleset. Loading used to assign the whole bin, so
//! every building came back with a capacity of zero and refused every Agent and
//! every rule that adds anything.
TEST(TestsCitySave, LoadedBuildingsKeepTheCapacitiesOfTheirType)
{
    Simulation simulation;
    ASSERT_TRUE(simulation.loadScriptFile(testCityRuleset()));

    std::string error;
    ASSERT_TRUE(CitySave::read(testCitySave(), simulation, error)) << error;

    City& city = *simulation.getCities().begin()->second;
    uint32_t checked = 0u;
    for (auto& building: city.getBuildings())
    {
        BuildingType const& type = simulation.getRuleset().getBuildingType(building->getTypeName().str());
        for (Resource const& capped : type.resources.getAll())
        {
            ASSERT_EQ(building->getResources().getCapacity(capped.getTypeName()),
                      capped.getCapacity())
                << building->getTypeName() << " lost the capacity of " << capped.getTypeName();
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
