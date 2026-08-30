#include "main.hpp"
#include <fstream>

#define protected public
#define private public
#include "OpenGlassBox/Ruleset.hpp"
#undef protected
#undef private

static std::string testCityPath()
{
    char const* candidates[] = {
        "../demo/data/simulations/sandbox.ogs",
        "demo/data/simulations/sandbox.ogs",
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
    Ruleset script;

    ASSERT_EQ(script.loadFile(testCityPath()), true);
    ASSERT_EQ(script.getErrors().size(), 0u);

    ScriptDefinitions const& defs = script.getDefinitions();

    // Check states content:

    // -- Resource types
    {
        ASSERT_GE(defs.getResources().size(), 3u);
        Resource const& r1 = script.getResource("Water");
        ASSERT_STREQ(r1.getTypeName().c_str(), "Water");
        ASSERT_EQ(r1.getCapacity(), Resource::MAX_CAPACITY);
        ASSERT_EQ(r1.getAmount(), 0u);

        Resource const& r2 = script.getResource("Grass");
        ASSERT_STREQ(r2.getTypeName().c_str(), "Grass");

        Resource const& r3 = script.getResource("People");
        ASSERT_STREQ(r3.getTypeName().c_str(), "People");

        ASSERT_STREQ(script.getResource("Money").getTypeName().c_str(),
                     "Money");
        ASSERT_STREQ(script.getResource("Goods").getTypeName().c_str(),
                     "Goods");
    }

    // -- Path types
    {
        ASSERT_EQ(defs.getPathTypes().size(), 1u);
        PathType const& p1 = script.getPathType("Road");
        ASSERT_STREQ(p1.name.c_str(), "Road");
    }

    // -- Path Segment types
    {
        ASSERT_EQ(defs.getSegmentTypes().size(), 2u);
        SegmentType const& s1 = script.getSegmentType("Dirt");
        ASSERT_STREQ(s1.name.c_str(), "Dirt");
        ASSERT_EQ(s1.color, 0xAAAAAAu);
        ASSERT_EQ(s1.speed, 30.0f);
        ASSERT_EQ(s1.capacity, 20.0f);
        ASSERT_EQ(s1.beta, 4.0f);
    }

    // -- Agent types:
    {
        ASSERT_GE(defs.getAgentTypes().size(), 3u);
        AgentType const& a1 = script.getAgentType("People");
        // ASSERT_STREQ(a1.name.c_str(), "People");
        ASSERT_EQ(a1.color, 0xFFFF00u);
        ASSERT_EQ(a1.speed, 10.0f);

        AgentType const& a2 = script.getAgentType("Worker");
        // ASSERT_STREQ(a2.name.c_str(), "Worker");
        ASSERT_EQ(a2.color, 0xFFFFFFu);
        ASSERT_EQ(a2.speed, 10.0f);
    }

    // -- Layer types
    {
        ASSERT_GE(defs.getLayerTypes().size(), 2u);
        // A cell of the water layer is the unit a tower supplies, so its
        // capacity is small and the tower gives away many times that amount.
        LayerType const& m1 = script.getLayerType("Water");
        ASSERT_EQ(m1.color, 0x0000FFu);
        ASSERT_EQ(m1.capacity, 20u);
        ASSERT_EQ(m1.decay, 10u);
        ASSERT_EQ(m1.getPeriodTicks(20u), 2u * 60u * 20u);
        ASSERT_EQ(m1.rules.size(), 0u);

        LayerType const& m2 = script.getLayerType("Grass");
        ASSERT_EQ(m2.color, 0x00FF00u);
        ASSERT_EQ(m2.capacity, 10u);
        ASSERT_EQ(m2.rules.size(), 1u);
        ASSERT_STREQ(m2.rules[0]->getName().c_str(), "CreateGrass");
    }

    // -- Building types
    {
        ASSERT_GE(defs.getBuildingTypes().size(), 2u);
        BuildingType const& u1 = script.getBuildingType("House");
        ASSERT_EQ(u1.color, 0xFF00FFu);
        ASSERT_EQ(u1.radius, 1u);
        ASSERT_GE(u1.rules.size(), 1u);
        ASSERT_STREQ(u1.rules[0]->getName().c_str(), "SendPeopleToWork");
        ASSERT_EQ(u1.targets.size(), 1u);
        ASSERT_STREQ(u1.targets[0].c_str(), "Home");
        ASSERT_GE(u1.resources.m_bin.size(), 1u);
        // A household holds more than the morning commute takes away, which is
        // who is left to go shopping later in the day.
        ASSERT_EQ(u1.resources.getCapacity("People"), 8u);
        ASSERT_EQ(u1.resources.getAmount("People"), 8u);

        BuildingType const& u2 = script.getBuildingType("FactoryDirty");
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
        BuildingType const& u3 = script.getBuildingType("Restaurant");
        ASSERT_EQ(u3.targets.size(), 1u);
        ASSERT_STREQ(u3.targets[0].c_str(), "Restaurant");
    }

    // Density is what the player paints and wealth is what an upgrade brings,
    // so a residential zone carries its density in its name and grows the
    // poorest of its three houses.
    ASSERT_STREQ(script.getZoneType("ResidentialLow").name.c_str(),
                 "ResidentialLow");
    ASSERT_GE(script.getRuleZone("GrowShack").getRate(), 1u);
    ASSERT_GE(script.getRuleZone("UpgradeShackToHouse").getRate(), 1u);
    ASSERT_GE(script.getRuleZone("UpgradeHouseToVilla").getRate(), 1u);

    // -- Layer Rules
    {
        ASSERT_GE(defs.getRuleLayers().size(), 1u);
        RuleLayer const& rm1 = script.getRuleLayer("CreateGrass");
        ASSERT_STREQ(rm1.m_type.c_str(), "CreateGrass");
        // "rate 20 minutes": twenty game minutes, four hundred ticks.
        ASSERT_EQ(rm1.getRateMinutes(), 20u);
        ASSERT_EQ(rm1.getPeriodTicks(20u), 400u);
        ASSERT_EQ(rm1.isRandom(), true);
        // Both 'layer' commands of the rule, in the order they were written.
        ASSERT_EQ(rm1.m_commands.size(), 2u);
    }

    // -- Building Rules
    {
        ASSERT_GE(defs.getRuleBuildings().size(), 3u);
        RuleBuilding const& ru1 = script.getRuleBuilding("SendPeopleToWork");
        ASSERT_STREQ(ru1.m_type.c_str(), "SendPeopleToWork");
        ASSERT_EQ(ru1.getPeriodTicks(20u), 90u * 20u);

        RuleBuilding const& ru2 = script.getRuleBuilding("SendPeopleToHome");
        ASSERT_STREQ(ru2.m_type.c_str(), "SendPeopleToHome");
        ASSERT_EQ(ru2.getPeriodTicks(20u), 20u * 20u);

        RuleBuilding const& ru3 = script.getRuleBuilding("SupplyWater");
        ASSERT_STREQ(ru3.m_type.c_str(), "SupplyWater");
        ASSERT_EQ(ru3.getPeriodTicks(20u), 60u * 20u);

        // The goods have to reach the shops on their own wheels, otherwise
        // nothing is ever for sale.
        RuleBuilding const& ship = script.getRuleBuilding("ShipGoodsToShop");
        ASSERT_EQ(ship.getPeriodTicks(20u), 45u * 20u);

        // A graded service costs what the player grants it. The station lists
        // only the first level and the engine walks down the chain.
        RuleBuilding const& patrol = script.getRuleBuilding("PatrolFull");
        RuleBuilding const* half = patrol.getOnFail();
        ASSERT_NE(half, nullptr);
        ASSERT_STREQ(half->getName().c_str(), "PatrolHalf");
        RuleBuilding const* skeleton = half->getOnFail();
        ASSERT_NE(skeleton, nullptr);
        ASSERT_STREQ(skeleton->getName().c_str(), "PatrolSkeleton");
        ASSERT_EQ(skeleton->getOnFail(), nullptr);
    }

    // Smoke travels to the street next door and fades within the hour. Neither
    // is something a rule can say, because a rule reads the one cell it stands
    // on: both are recipes of the layer itself.
    {
        LayerType const& pollution = script.getLayerType("Pollution");
        ASSERT_TRUE(pollution.spreads());
        ASSERT_GT(pollution.diffusion, 0u);
        ASSERT_GT(pollution.decay, 0u);
        ASSERT_LE(pollution.diffusion + pollution.decay, 100u);
        ASSERT_EQ(pollution.getPeriodTicks(20u), 60u * 20u);

        // What lands in the soil hardly moves and hardly leaves, which is what
        // makes an industrial wasteland a lasting problem.
        LayerType const& ground = script.getLayerType("GroundPollution");
        ASSERT_LT(ground.diffusion, pollution.diffusion);
        ASSERT_LT(ground.decay, pollution.decay);
        ASSERT_GT(ground.getPeriodTicks(20u), pollution.getPeriodTicks(20u));
    }
}

TEST(TestsScript, DoesNotExist)
{
    Ruleset script;

    // Load a script that does not exist.
    ASSERT_EQ(script.loadFile("fdsfhsdfgsdfdsf"), false);
}

TEST(TestsScript, BadSyntax)
{
    Ruleset script;

    // Load a script that contains error syntax.
    {
        std::ofstream out("/tmp/foo");
        out << "foo\n";
    }
    ASSERT_EQ(script.loadFile("/tmp/foo"), false);
}

TEST(TestsScript, EmptyFile)
{
    Ruleset script;

    // Load a script that contains error syntax.
    {
        std::ofstream out("/tmp/foo");
        out << "\n";
    }
    ASSERT_EQ(script.loadFile("/tmp/foo"), false);
}

//------------------------------------------------------------------------------
//! \brief An error carries where it happened, so the demo can point at the
//! line.
//------------------------------------------------------------------------------
TEST(TestsScript, ErrorPosition)
{
    Ruleset script;

    ASSERT_EQ(
        script.loadString("resources\n  resource Water\nend\n\nnonsense\n"),
        false);
    ASSERT_EQ(script.getErrors().size(), 1u) << script.formatErrors();

    ParseError const& e = script.getErrors()[0];
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
    Ruleset script;

    ASSERT_EQ(script.loadString("resourcess\n"
                                "  resource Water\n"
                                "end\n"
                                "paths\n"
                                "  path Road color notanumber\n"
                                "end\n"),
              false);
    ASSERT_EQ(script.getErrors().size(), 2u) << script.formatErrors();
    ASSERT_EQ(script.getErrors()[0].line, 1u);
    ASSERT_EQ(script.getErrors()[1].line, 5u);
}

//------------------------------------------------------------------------------
//! \brief An unterminated section used to spin forever reading the same empty
//! token. It now stops at the end of the file and says so.
//------------------------------------------------------------------------------
TEST(TestsScript, UnterminatedSection)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource Water\n"), false);
    ASSERT_EQ(script.getErrors().size(), 1u) << script.formatErrors();
    ASSERT_NE(script.getErrors()[0].message.find("end of script"),
              std::string::npos)
        << script.formatErrors();
}

//------------------------------------------------------------------------------
//! \brief A number that is not one is an error rather than a silent zero.
//------------------------------------------------------------------------------
TEST(TestsScript, BadNumber)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource Water\nend\n"
                                "paths\n  path Road color notanumber\nend\n"),
              false);
    ASSERT_EQ(script.getErrors().size(), 1u) << script.formatErrors();
    ASSERT_EQ(script.getErrors()[0].line, 5u);
}

//------------------------------------------------------------------------------
//! \brief Whether the lines of a network make a junction where they cross is a
//! property of the network, and streets crossing is the case that needs no
//! saying.
//------------------------------------------------------------------------------
TEST(TestsScript, PathCrossings)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource Water\nend\n"
                                "paths\n"
                                "  path Road color 0xAAAAAA\n"
                                "  path Pipe color 0x0000FF crossings false\n"
                                "  path Rail color 0x888888 crossings true\n"
                                "end\n"),
              true)
        << script.formatErrors();

    ASSERT_EQ(script.getPathType("Road").crossings, true);
    ASSERT_EQ(script.getPathType("Pipe").crossings, false);
    ASSERT_EQ(script.getPathType("Rail").crossings, true);
}

//------------------------------------------------------------------------------
//! \brief A word that is neither true nor false is an error rather than a
//! silent false.
//------------------------------------------------------------------------------
TEST(TestsScript, BadPathCrossings)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource Water\nend\n"
                                "paths\n  path Road crossings maybe\nend\n"),
              false);
    ASSERT_EQ(script.getErrors().size(), 1u) << script.formatErrors();
    ASSERT_EQ(script.getErrors()[0].line, 5u);
}

//------------------------------------------------------------------------------
//! \brief A name that no section defines is reported instead of being silently
//! turned into a null pointer the simulation would later dereference.
//------------------------------------------------------------------------------
TEST(TestsScript, UnknownReference)
{
    Ruleset script;

    ASSERT_EQ(
        script.loadString("resources\n  resource Water\nend\n"
                          "layers\n  layer Water color 0x0000FF capacity 10 "
                          "rules [ NoSuchRule ]\nend\n"),
        false);
    ASSERT_EQ(script.getErrors().size(), 1u) << script.formatErrors();
    ASSERT_NE(script.getErrors()[0].message.find("NoSuchRule"),
              std::string::npos);
}

//------------------------------------------------------------------------------
//! \brief Defining the same name twice is an error, not a silent overwrite.
//------------------------------------------------------------------------------
TEST(TestsScript, DuplicateDefinition)
{
    Ruleset script;

    ASSERT_EQ(script.loadString(
                  "resources\n  resource Water\n  resource Water\nend\n"),
              false);
    ASSERT_NE(script.getErrors().size(), 0u);
    ASSERT_NE(script.getErrors()[0].message.find("defined twice"),
              std::string::npos)
        << script.formatErrors();
}

//------------------------------------------------------------------------------
//! \brief Sections may come in any order: a rule can name a layer defined
//! further down the file. This is what the declaration pass buys.
//------------------------------------------------------------------------------
TEST(TestsScript, ForwardReference)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("layers\n"
                                "  layer Grass color 0x00FF00 capacity 10 "
                                "rules [ CreateGrass ]\n"
                                "end\n"
                                "rules\n"
                                "  layerRule CreateGrass\n"
                                "    rate 7\n"
                                "    layer Grass add 1\n"
                                "  end\n"
                                "end\n"
                                "resources\n"
                                "  resource Grass\n"
                                "end\n"),
              true)
        << script.formatErrors();

    ASSERT_EQ(script.getLayerType("Grass").rules.size(), 1u);
    ASSERT_EQ(script.getLayerType("Grass").rules[0],
              &script.getRuleLayer("CreateGrass"));
    ASSERT_EQ(script.getRuleLayer("CreateGrass").getRate(), 7u);
}

//------------------------------------------------------------------------------
//! \brief A failed load leaves the previously loaded script untouched, so a
//! typo during a hot reload does not empty a running simulation.
//------------------------------------------------------------------------------
TEST(TestsScript, FailedReloadKeepsPreviousDefinitions)
{
    Ruleset script;

    ASSERT_EQ(script.loadFile(testCityPath()), true);
    size_t const before = script.getDefinitions().getResources().size();

    ASSERT_EQ(script.loadString("this is not a script"), false);
    ASSERT_EQ(script.getDefinitions().getResources().size(), before);
    ASSERT_STREQ(script.getResource("Water").getTypeName().c_str(), "Water");
}

//------------------------------------------------------------------------------
//! \brief The calendar predicate and the Zone commands parse as first-class
//! rule bodies, and a fractional speed is kept as a float rather than truncated
//! by atoi.
//------------------------------------------------------------------------------
TEST(TestsScript, HourAndZone)
{
    Ruleset script;

    ASSERT_EQ(
        script.loadString(
            "resources\n  resource People\nend\n"
            "paths\n  path Road color 0xAAAAAA\nend\n"
            "segments\n  segment Dirt color 0xAAAAAA speed 10.5 "
            "capacity 20 beta 4\nend\n"
            "agents\n  agent Worker color 0xFFFFFF speed 10\nend\n"
            "rules\n"
            "  buildingRule Morning\n"
            "    rate 1\n"
            "    hour between 8 18\n"
            "    local People remove 1\n"
            "  end\n"
            "  zoneRule Grow\n"
            "    rate 2\n"
            "    count Home less 3\n"
            "    spawn Home at nearestSegment\n"
            "  end\n"
            "  zoneRule Replace\n"
            "    rate 3\n"
            "    upgrade Home to Shop\n"
            "  end\n"
            "end\n"
            "buildings\n"
            "  building Home color 0xFF00FF layerRadius 1 rules [ Morning ] "
            "targets [ Home ] caps [ People 4 ] resources [ People 1 ]\n"
            "  building Shop color 0xFFAA00 layerRadius 1 rules [ ] "
            "targets [ Shop ] caps [ People 4 ] resources [ ]\n"
            "end\n"
            "layers\n  layer People color 0xFFFF00 capacity 10 rules [ ]\nend\n"
            "zones\n  zone Residential color 0x44AA44 rules [ Grow Replace "
            "]\nend\n"),
        true)
        << script.formatErrors();

    ASSERT_EQ(script.getSegmentType("Dirt").speed, 10.5f);
    ASSERT_EQ(script.getSegmentType("Dirt").capacity, 20.0f);
    ASSERT_EQ(script.getSegmentType("Dirt").beta, 4.0f);

    RuleBuilding const& morning = script.getRuleBuilding("Morning");
    ASSERT_EQ(morning.m_commands.size(), 2u);
    ASSERT_EQ(morning.m_commands[0]->getDescription(),
              std::string("Hour between 8 and 18"));

    RuleZone const& grow = script.getRuleZone("Grow");
    ASSERT_EQ(grow.getRate(), 2u);
    ASSERT_EQ(grow.getCommands().size(), 2u);
    ASSERT_EQ(grow.getCommands()[0]->getDescription(),
              std::string("Count Home"));
    ASSERT_EQ(grow.getCommands()[1]->getDescription(),
              std::string("Spawn Home"));

    RuleZone const& replace = script.getRuleZone("Replace");
    ASSERT_EQ(replace.getCommands()[0]->getDescription(),
              std::string("Upgrade Home to Shop"));
    ASSERT_STREQ(script.getZoneType("Residential").name.c_str(), "Residential");
}

//------------------------------------------------------------------------------
//! \brief A period may be written as a duration of game time. Ticks are an
//! implementation detail: nobody reading "rate 600" knows it means half an
//! hour.
//------------------------------------------------------------------------------
TEST(TestsScript, RatesInGameTime)
{
    Ruleset script;

    ASSERT_EQ(script.loadString(
                  "resources\n  resource People\nend\n"
                  "rules\n"
                  "  buildingRule EveryTick\n    rate 7\n"
                  "    local People remove 1\n  end\n"
                  "  buildingRule Spelled\n    rate 7 ticks\n"
                  "    local People remove 1\n  end\n"
                  "  buildingRule HalfHour\n    rate 30 minutes\n"
                  "    local People remove 1\n  end\n"
                  "  buildingRule OneMinute\n    rate 1 minute\n"
                  "    local People remove 1\n  end\n"
                  "  layerRule TwoHours\n    rate 2 hours\n"
                  "    layer People add 1\n  end\n"
                  "  zoneRule Daily\n    rate 1 day\n"
                  "    count Home less 3\n  end\n"
                  "end\n"
                  "buildings\n"
                  "  building Home color 0xFF00FF layerRadius 1 rules [ ] "
                  "targets [ Home ] caps [ People 4 ] resources [ ]\n"
                  "end\n"
                  "layers\n  layer People color 0xFFFF00 capacity 10 rules [ "
                  "]\nend\n"),
              true)
        << script.formatErrors();

    // Counted in ticks: unchanged, whatever the length of a minute.
    ASSERT_EQ(script.getRuleBuilding("EveryTick").getRateMinutes(), 0u);
    ASSERT_EQ(script.getRuleBuilding("EveryTick").getPeriodTicks(20u), 7u);
    ASSERT_EQ(script.getRuleBuilding("EveryTick").getPeriodTicks(30u), 7u);
    ASSERT_EQ(script.getRuleBuilding("Spelled").getPeriodTicks(20u), 7u);

    // Counted in game time: follows the length of a minute.
    ASSERT_EQ(script.getRuleBuilding("HalfHour").getRateMinutes(), 30u);
    ASSERT_EQ(script.getRuleBuilding("HalfHour").getPeriodTicks(20u), 600u);
    ASSERT_EQ(script.getRuleBuilding("HalfHour").getPeriodTicks(30u), 900u);
    ASSERT_EQ(script.getRuleBuilding("OneMinute").getPeriodTicks(20u), 20u);
    ASSERT_EQ(script.getRuleLayer("TwoHours").getPeriodTicks(20u), 2400u);
    ASSERT_EQ(script.getRuleZone("Daily").getPeriodTicks(20u), 28800u);
}

//------------------------------------------------------------------------------
//! \brief A period of zero would run the rule at every tick and used to divide
//! by zero. It is reported, and the rule falls back to one tick.
//------------------------------------------------------------------------------
TEST(TestsScript, RateOfZeroIsRefused)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource People\nend\n"
                                "rules\n"
                                "  buildingRule Never\n    rate 0\n"
                                "    local People remove 1\n  end\n"
                                "end\n"),
              false);
    ASSERT_NE(script.formatErrors().find("period of zero"), std::string::npos);
}

//------------------------------------------------------------------------------
//! \brief A layer rule runs on a cell of the map, and a cell owns no resources,
//! so 'local' has nothing to name there. The engine used to read a null
//! pointer.
//------------------------------------------------------------------------------
TEST(TestsScript, LocalInALayerRuleIsRefused)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource People\nend\n"
                                "rules\n"
                                "  layerRule Wrong\n    rate 1\n"
                                "    local People remove 1\n  end\n"
                                "end\n"),
              false);
    ASSERT_NE(script.formatErrors().find("cannot read 'local'"),
              std::string::npos);

    // The same command inside a building rule or a zone rule is fine, and a
    // layer rule still reaches the city with 'global'.
    Ruleset good;
    ASSERT_EQ(good.loadString("resources\n  resource People\nend\n"
                              "rules\n"
                              "  buildingRule Fine\n    rate 1\n"
                              "    local People remove 1\n  end\n"
                              "  layerRule AlsoFine\n    rate 1\n"
                              "    global People remove 1\n  end\n"
                              "end\n"),
              true)
        << good.formatErrors();
}

//------------------------------------------------------------------------------
//! \brief A layer may transport and lose its amounts by itself. Smoke moves to
//! the cells nearby and fades, and no rule can say that: a layer rule reads and
//! writes the single cell it stands on.
//------------------------------------------------------------------------------
TEST(TestsScript, LayerDiffusionAndDecay)
{
    Ruleset script;

    ASSERT_EQ(script.loadString(
                  "resources\n  resource Water\nend\n"
                  "layers\n"
                  "  layer Water color 0x0000FF capacity 100 rules [ ]\n"
                  "  layer Pollution color 0x806040 capacity 100 "
                  "diffusion 24 decay 8 rate 30 minutes rules [ ]\n"
                  "  layer Noise color 0x888888 capacity 100 "
                  "diffusion 40 rate 5 rules [ ]\n"
                  "end\n"),
              true)
        << script.formatErrors();

    // A layer that says nothing keeps every amount where a rule put it, which
    // is what every layer written before this existed expects.
    ASSERT_EQ(script.getLayerType("Water").diffusion, 0u);
    ASSERT_EQ(script.getLayerType("Water").decay, 0u);
    ASSERT_FALSE(script.getLayerType("Water").spreads());

    ASSERT_EQ(script.getLayerType("Pollution").diffusion, 24u);
    ASSERT_EQ(script.getLayerType("Pollution").decay, 8u);
    ASSERT_TRUE(script.getLayerType("Pollution").spreads());
    ASSERT_EQ(script.getLayerType("Pollution").getPeriodTicks(20u), 600u);

    ASSERT_EQ(script.getLayerType("Noise").diffusion, 40u);
    ASSERT_EQ(script.getLayerType("Noise").decay, 0u);
    ASSERT_EQ(script.getLayerType("Noise").getPeriodTicks(20u), 5u);
}

//------------------------------------------------------------------------------
//! \brief The engine takes both shares from the same amount, so a cell cannot
//! give away more than it holds.
//------------------------------------------------------------------------------
TEST(TestsScript, LayerGivingAwayMoreThanACellHoldsIsRefused)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource Water\nend\n"
                                "layers\n"
                                "  layer Pollution color 0x806040 capacity 100 "
                                "diffusion 80 decay 40 rules [ ]\n"
                                "end\n"),
              false);
    ASSERT_NE(script.formatErrors().find("add up to 100 at most"),
              std::string::npos);
}

//------------------------------------------------------------------------------
//! \brief A share above one hundred has no meaning and is reported.
//------------------------------------------------------------------------------
TEST(TestsScript, LayerShareAboveOneHundredIsRefused)
{
    Ruleset script;

    ASSERT_EQ(script.loadString("resources\n  resource Water\nend\n"
                                "layers\n"
                                "  layer Pollution color 0x806040 capacity 100 "
                                "decay 140 rules [ ]\n"
                                "end\n"),
              false);
    ASSERT_NE(script.formatErrors().find("between 0 and 100"),
              std::string::npos);
}
