#include "main.hpp"
#include <fstream>

#define protected public
#define private public
#include "OpenGlassBox/ScriptParser.hpp"
#undef protected
#undef private

static std::string testCityPath()
{
    char const* candidates[] = {
        "../demo/data/Simulations/test_city.ogs",
        "demo/data/Simulations/test_city.ogs",
    };
    for (char const* path : candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

TEST(TestsScript, Constructor)
{
    Script script;

    ASSERT_EQ(script.parseFile(testCityPath()), true);
    ASSERT_EQ(script.errors().size(), 0u);

    ScriptDefinitions const& defs = script.definitions();

    // Check states content:

    // -- Resource types
    {
        ASSERT_GE(defs.resources().size(), 3u);
        Resource const& r1 = script.getResource("Water");
        ASSERT_STREQ(r1.type().c_str(), "Water");
        ASSERT_EQ(r1.getCapacity(), Resource::MAX_CAPACITY);
        ASSERT_EQ(r1.getAmount(), 0u);

        Resource const& r2 = script.getResource("Grass");
        ASSERT_STREQ(r2.type().c_str(), "Grass");

        Resource const& r3 = script.getResource("People");
        ASSERT_STREQ(r3.type().c_str(), "People");

        ASSERT_STREQ(script.getResource("Money").type().c_str(), "Money");
        ASSERT_STREQ(script.getResource("Goods").type().c_str(), "Goods");
    }

    // -- Path types
    {
        ASSERT_EQ(defs.pathTypes().size(), 1u);
        PathType const& p1 = script.getPathType("Road");
        ASSERT_STREQ(p1.name.c_str(), "Road");
    }

    // -- Path Way types
    {
        ASSERT_EQ(defs.wayTypes().size(), 1u);
        WayType const& s1 = script.getWayType("Dirt");
        ASSERT_STREQ(s1.name.c_str(), "Dirt");
        ASSERT_EQ(s1.color, 0xAAAAAAu);
        ASSERT_EQ(s1.speed, 30.0f);
        ASSERT_EQ(s1.capacity, 20.0f);
        ASSERT_EQ(s1.beta, 4.0f);
    }

    // -- Agent types:
    {
        ASSERT_GE(defs.agentTypes().size(), 3u);
        AgentType const& a1 = script.getAgentType("People");
        // ASSERT_STREQ(a1.name.c_str(), "People");
        ASSERT_EQ(a1.color, 0xFFFF00u);
        ASSERT_EQ(a1.speed, 10u);

        AgentType const& a2 = script.getAgentType("Worker");
        // ASSERT_STREQ(a2.name.c_str(), "Worker");
        ASSERT_EQ(a2.color, 0xFFFFFFu);
        ASSERT_EQ(a2.speed, 10u);
    }

    // -- Map types
    {
        ASSERT_GE(defs.mapTypes().size(), 2u);
        MapType const& m1 = script.getMapType("Water");
        ASSERT_EQ(m1.color, 0x0000FFu);
        ASSERT_EQ(m1.capacity, 100u);
        ASSERT_EQ(m1.rules.size(), 0u);

        MapType const& m2 = script.getMapType("Grass");
        ASSERT_EQ(m2.color, 0x00FF00u);
        ASSERT_EQ(m2.capacity, 10u);
        ASSERT_EQ(m2.rules.size(), 1u);
        ASSERT_STREQ(m2.rules[0]->type().c_str(), "CreateGrass");
    }

    // -- Unit types
    {
        ASSERT_GE(defs.unitTypes().size(), 2u);
        UnitType const& u1 = script.getUnitType("Home");
        ASSERT_EQ(u1.color, 0xFF00FFu);
        ASSERT_EQ(u1.radius, 1u);
        ASSERT_GE(u1.rules.size(), 1u);
        ASSERT_STREQ(u1.rules[0]->type().c_str(), "SendPeopleToWork");
        ASSERT_EQ(u1.targets.size(), 1u);
        ASSERT_STREQ(u1.targets[0].c_str(), "Home");
        ASSERT_GE(u1.resources.m_bin.size(), 1u);
        // A household holds more than the morning commute takes away, which is
        // who is left to go shopping later in the day.
        ASSERT_EQ(u1.resources.getCapacity("People"), 8u);
        ASSERT_EQ(u1.resources.getAmount("People"), 8u);

        UnitType const& u2 = script.getUnitType("Work");
        ASSERT_EQ(u2.color, 0x00AAFFu);
        ASSERT_EQ(u2.radius, 3u);
        ASSERT_GE(u2.rules.size(), 2u);
        ASSERT_EQ(u2.targets.size(), 1u);
        ASSERT_STREQ(u2.targets[0].c_str(), "Work");
        ASSERT_GE(u2.resources.m_bin.size(), 1u);
        ASSERT_EQ(u2.resources.getCapacity("People"), 12u);

        // A restaurant answers to its own name only. Answering to Shop as well
        // is how the freight and the shoppers ended up at the canteen and the
        // shops were never opened.
        UnitType const& u3 = script.getUnitType("Restaurant");
        ASSERT_EQ(u3.targets.size(), 1u);
        ASSERT_STREQ(u3.targets[0].c_str(), "Restaurant");
    }

    ASSERT_STREQ(script.getAreaType("Residential").name.c_str(), "Residential");
    ASSERT_GE(script.getRuleArea("GrowHomes").rate(), 1u);

    // -- Map Rules
    {
        ASSERT_GE(defs.ruleMaps().size(), 1u);
        RuleMap const& rm1 = script.getRuleMap("CreateGrass");
        ASSERT_STREQ(rm1.m_type.c_str(), "CreateGrass");
        // "rate 20 minutes": twenty game minutes, four hundred ticks.
        ASSERT_EQ(rm1.rateMinutes(), 20u);
        ASSERT_EQ(rm1.periodTicks(20u), 400u);
        ASSERT_EQ(rm1.isRandom(), true);
        // Both 'map' commands of the rule, in the order they were written.
        ASSERT_EQ(rm1.m_commands.size(), 2u);
    }

    // -- Unit Rules
    {
        ASSERT_GE(defs.ruleUnits().size(), 3u);
        RuleUnit const& ru1 = script.getRuleUnit("SendPeopleToWork");
        ASSERT_STREQ(ru1.m_type.c_str(), "SendPeopleToWork");
        ASSERT_EQ(ru1.periodTicks(20u), 45u * 20u);

        RuleUnit const& ru2 = script.getRuleUnit("SendPeopleToHome");
        ASSERT_STREQ(ru2.m_type.c_str(), "SendPeopleToHome");
        ASSERT_EQ(ru2.periodTicks(20u), 20u * 20u);

        RuleUnit const& ru3 = script.getRuleUnit("UsePeopleToWater");
        ASSERT_STREQ(ru3.m_type.c_str(), "UsePeopleToWater");
        ASSERT_EQ(ru3.periodTicks(20u), 60u * 20u);

        // The goods have to reach the shops on their own wheels, otherwise
        // nothing is ever for sale.
        RuleUnit const& ship = script.getRuleUnit("ShipGoods");
        ASSERT_EQ(ship.periodTicks(20u), 45u * 20u);
    }

    // Pollution must have a rule that takes it away, not only one that adds
    // some: it used to saturate and pin Desirability to zero for ever.
    {
        MapType const& pollution = script.getMapType("Pollution");
        bool spreads = false;
        bool cleans = false;
        for (auto const* rule : pollution.rules)
        {
            spreads = spreads || (rule->type() == "SpreadPollution");
            cleans = cleans || (rule->type() == "CleanPollution");
        }
        ASSERT_TRUE(spreads);
        ASSERT_TRUE(cleans);
    }
}

TEST(TestsScript, DoesNotExist)
{
    Script script;

    // Load a script that does not exist.
    ASSERT_EQ(script.parseFile("fdsfhsdfgsdfdsf"), false);
}

TEST(TestsScript, BadSyntax)
{
    Script script;

    // Load a script that contains error syntax.
    (void)system("echo \"foo\" > /tmp/foo");
    ASSERT_EQ(script.parseFile("/tmp/foo"), false);
}

TEST(TestsScript, EmptyFile)
{
    Script script;

    // Load a script that contains error syntax.
    (void)system("echo \"\" > /tmp/foo");
    ASSERT_EQ(script.parseFile("/tmp/foo"), false);
}

//------------------------------------------------------------------------------
//! \brief An error carries where it happened, so the demo can point at the
//! line.
//------------------------------------------------------------------------------
TEST(TestsScript, ErrorPosition)
{
    Script script;

    ASSERT_EQ(
        script.parseString("resources\n  resource Water\nend\n\nnonsense\n"),
        false);
    ASSERT_EQ(script.errors().size(), 1u) << script.formatErrors();

    ParseError const& e = script.errors()[0];
    ASSERT_EQ(e.line, 5u);
    ASSERT_EQ(e.column, 1u);
    ASSERT_STREQ(e.source.c_str(), "nonsense");
    // The message quotes the offending word and the position.
    ASSERT_NE(e.format().find("nonsense"), std::string::npos);
    ASSERT_NE(e.format().find("5:1"), std::string::npos);
}

//------------------------------------------------------------------------------
//! \brief Parsing does not stop on the first error: a single run reports all of
//! them, so the user fixes the file in one go instead of one error per attempt.
//! A bad section header costs one error and parsing resumes at the next section
//! instead of drowning the report in the body it could not read.
//------------------------------------------------------------------------------
TEST(TestsScript, ReportsSeveralErrors)
{
    Script script;

    ASSERT_EQ(script.parseString("resourcess\n"
                                 "  resource Water\n"
                                 "end\n"
                                 "paths\n"
                                 "  path Road color notanumber\n"
                                 "end\n"),
              false);
    ASSERT_EQ(script.errors().size(), 2u) << script.formatErrors();
    ASSERT_EQ(script.errors()[0].line, 1u);
    ASSERT_EQ(script.errors()[1].line, 5u);
}

//------------------------------------------------------------------------------
//! \brief An unterminated section used to spin forever reading the same empty
//! token. It now stops at the end of the file and says so.
//------------------------------------------------------------------------------
TEST(TestsScript, UnterminatedSection)
{
    Script script;

    ASSERT_EQ(script.parseString("resources\n  resource Water\n"), false);
    ASSERT_EQ(script.errors().size(), 1u) << script.formatErrors();
    ASSERT_NE(script.errors()[0].message.find("end of script"),
              std::string::npos)
        << script.formatErrors();
}

//------------------------------------------------------------------------------
//! \brief A number that is not one is an error rather than a silent zero.
//------------------------------------------------------------------------------
TEST(TestsScript, BadNumber)
{
    Script script;

    ASSERT_EQ(script.parseString("resources\n  resource Water\nend\n"
                                 "paths\n  path Road color notanumber\nend\n"),
              false);
    ASSERT_EQ(script.errors().size(), 1u) << script.formatErrors();
    ASSERT_EQ(script.errors()[0].line, 5u);
}

//------------------------------------------------------------------------------
//! \brief A name that no section defines is reported instead of being silently
//! turned into a null pointer the simulation would later dereference.
//------------------------------------------------------------------------------
TEST(TestsScript, UnknownReference)
{
    Script script;

    ASSERT_EQ(script.parseString("resources\n  resource Water\nend\n"
                                 "maps\n  map Water color 0x0000FF capacity 10 "
                                 "rules [ NoSuchRule ]\nend\n"),
              false);
    ASSERT_EQ(script.errors().size(), 1u) << script.formatErrors();
    ASSERT_NE(script.errors()[0].message.find("NoSuchRule"), std::string::npos);
}

//------------------------------------------------------------------------------
//! \brief Defining the same name twice is an error, not a silent overwrite.
//------------------------------------------------------------------------------
TEST(TestsScript, DuplicateDefinition)
{
    Script script;

    ASSERT_EQ(script.parseString(
                  "resources\n  resource Water\n  resource Water\nend\n"),
              false);
    ASSERT_NE(script.errors().size(), 0u);
    ASSERT_NE(script.errors()[0].message.find("defined twice"),
              std::string::npos)
        << script.formatErrors();
}

//------------------------------------------------------------------------------
//! \brief Sections may come in any order: a rule can name a map defined further
//! down the file. This is what the declaration pass buys.
//------------------------------------------------------------------------------
TEST(TestsScript, ForwardReference)
{
    Script script;

    ASSERT_EQ(script.parseString("maps\n"
                                 "  map Grass color 0x00FF00 capacity 10 "
                                 "rules [ CreateGrass ]\n"
                                 "end\n"
                                 "rules\n"
                                 "  mapRule CreateGrass\n"
                                 "    rate 7\n"
                                 "    map Grass add 1\n"
                                 "  end\n"
                                 "end\n"
                                 "resources\n"
                                 "  resource Grass\n"
                                 "end\n"),
              true)
        << script.formatErrors();

    ASSERT_EQ(script.getMapType("Grass").rules.size(), 1u);
    ASSERT_EQ(script.getMapType("Grass").rules[0],
              &script.getRuleMap("CreateGrass"));
    ASSERT_EQ(script.getRuleMap("CreateGrass").rate(), 7u);
}

//------------------------------------------------------------------------------
//! \brief A failed load leaves the previously loaded script untouched, so a
//! typo during a hot reload does not empty a running simulation.
//------------------------------------------------------------------------------
TEST(TestsScript, FailedReloadKeepsPreviousDefinitions)
{
    Script script;

    ASSERT_EQ(script.parseFile(testCityPath()), true);
    size_t const before = script.definitions().resources().size();

    ASSERT_EQ(script.parseString("this is not a script"), false);
    ASSERT_EQ(script.definitions().resources().size(), before);
    ASSERT_STREQ(script.getResource("Water").type().c_str(), "Water");
}

//------------------------------------------------------------------------------
//! \brief The calendar predicate and the Area commands parse as first-class
//! rule bodies, and a fractional speed is kept as a float rather than truncated
//! by atoi.
//------------------------------------------------------------------------------
TEST(TestsScript, HourAndArea)
{
    Script script;

    ASSERT_EQ(
        script.parseString(
            "resources\n  resource People\nend\n"
            "paths\n  path Road color 0xAAAAAA\nend\n"
            "segments\n  segment Dirt color 0xAAAAAA speed 10.5 "
            "capacity 20 beta 4\nend\n"
            "agents\n  agent Worker color 0xFFFFFF speed 10\nend\n"
            "rules\n"
            "  unitRule Morning\n"
            "    rate 1\n"
            "    hour between 8 18\n"
            "    local People remove 1\n"
            "  end\n"
            "  areaRule Grow\n"
            "    rate 2\n"
            "    count Home less 3\n"
            "    spawn Home at nearestWay\n"
            "  end\n"
            "  areaRule Replace\n"
            "    rate 3\n"
            "    upgrade Home to Shop\n"
            "  end\n"
            "end\n"
            "units\n"
            "  unit Home color 0xFF00FF mapRadius 1 rules [ Morning ] "
            "targets [ Home ] caps [ People 4 ] resources [ People 1 ]\n"
            "  unit Shop color 0xFFAA00 mapRadius 1 rules [ ] "
            "targets [ Shop ] caps [ People 4 ] resources [ ]\n"
            "end\n"
            "maps\n  map People color 0xFFFF00 capacity 10 rules [ ]\nend\n"
            "areas\n  area Residential color 0x44AA44 rules [ Grow Replace "
            "]\nend\n"),
        true)
        << script.formatErrors();

    ASSERT_EQ(script.getWayType("Dirt").speed, 10.5f);
    ASSERT_EQ(script.getWayType("Dirt").capacity, 20.0f);
    ASSERT_EQ(script.getWayType("Dirt").beta, 4.0f);

    RuleUnit const& morning = script.getRuleUnit("Morning");
    ASSERT_EQ(morning.m_commands.size(), 2u);
    ASSERT_EQ(morning.m_commands[0]->type(),
              std::string("Hour between 8 and 18"));

    RuleArea const& grow = script.getRuleArea("Grow");
    ASSERT_EQ(grow.rate(), 2u);
    ASSERT_EQ(grow.commands().size(), 2u);
    ASSERT_EQ(grow.commands()[0]->type(), std::string("Count Home"));
    ASSERT_EQ(grow.commands()[1]->type(), std::string("Spawn Home"));

    RuleArea const& replace = script.getRuleArea("Replace");
    ASSERT_EQ(replace.commands()[0]->type(),
              std::string("Upgrade Home to Shop"));
    ASSERT_STREQ(script.getAreaType("Residential").name.c_str(), "Residential");
}

//------------------------------------------------------------------------------
//! \brief A period may be written as a duration of game time. Ticks are an
//! implementation detail: nobody reading "rate 600" knows it means half an
//! hour.
//------------------------------------------------------------------------------
TEST(TestsScript, RatesInGameTime)
{
    Script script;

    ASSERT_EQ(
        script.parseString(
            "resources\n  resource People\nend\n"
            "rules\n"
            "  unitRule EveryTick\n    rate 7\n"
            "    local People remove 1\n  end\n"
            "  unitRule Spelled\n    rate 7 ticks\n"
            "    local People remove 1\n  end\n"
            "  unitRule HalfHour\n    rate 30 minutes\n"
            "    local People remove 1\n  end\n"
            "  unitRule OneMinute\n    rate 1 minute\n"
            "    local People remove 1\n  end\n"
            "  mapRule TwoHours\n    rate 2 hours\n"
            "    map People add 1\n  end\n"
            "  areaRule Daily\n    rate 1 day\n"
            "    count Home less 3\n  end\n"
            "end\n"
            "units\n"
            "  unit Home color 0xFF00FF mapRadius 1 rules [ ] "
            "targets [ Home ] caps [ People 4 ] resources [ ]\n"
            "end\n"
            "maps\n  map People color 0xFFFF00 capacity 10 rules [ ]\nend\n"),
        true)
        << script.formatErrors();

    // Counted in ticks: unchanged, whatever the length of a minute.
    ASSERT_EQ(script.getRuleUnit("EveryTick").rateMinutes(), 0u);
    ASSERT_EQ(script.getRuleUnit("EveryTick").periodTicks(20u), 7u);
    ASSERT_EQ(script.getRuleUnit("EveryTick").periodTicks(30u), 7u);
    ASSERT_EQ(script.getRuleUnit("Spelled").periodTicks(20u), 7u);

    // Counted in game time: follows the length of a minute.
    ASSERT_EQ(script.getRuleUnit("HalfHour").rateMinutes(), 30u);
    ASSERT_EQ(script.getRuleUnit("HalfHour").periodTicks(20u), 600u);
    ASSERT_EQ(script.getRuleUnit("HalfHour").periodTicks(30u), 900u);
    ASSERT_EQ(script.getRuleUnit("OneMinute").periodTicks(20u), 20u);
    ASSERT_EQ(script.getRuleMap("TwoHours").periodTicks(20u), 2400u);
    ASSERT_EQ(script.getRuleArea("Daily").periodTicks(20u), 28800u);
}

//------------------------------------------------------------------------------
//! \brief A period of zero would run the rule at every tick and used to divide
//! by zero. It is reported, and the rule falls back to one tick.
//------------------------------------------------------------------------------
TEST(TestsScript, RateOfZeroIsRefused)
{
    Script script;

    ASSERT_EQ(script.parseString("resources\n  resource People\nend\n"
                                 "rules\n"
                                 "  unitRule Never\n    rate 0\n"
                                 "    local People remove 1\n  end\n"
                                 "end\n"),
              false);
    ASSERT_NE(script.formatErrors().find("period of zero"), std::string::npos);
}
