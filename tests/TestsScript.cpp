#include "main.hpp"

#define protected public
#define private public
#  include "src/Core/Script.hpp"
#undef protected
#undef private

TEST(TestsScript, DoesNotExist)
{
    // Load a script that does not exist.
    Script script("fdsfhsdfgsdfdsf");
    ASSERT_EQ(script, false);
}

TEST(TestsScript, Constructor)
{
    // Load a script.
    Script script("../src/Simulation/TestCity2.txt");
    ASSERT_EQ(script, true);

    // Check states content:

    // -- Resource types
    {
        ASSERT_EQ(script.m_resources.size(), 3u);
        Resource const& r1 = script.getResource("Water");
        ASSERT_STREQ(r1.type().c_str(), "Water");
        ASSERT_EQ(r1.getCapacity(), Resource::MAX_CAPACITY);
        ASSERT_EQ(r1.getAmount(), 0u);

        Resource const& r2 = script.getResource("Grass");
        ASSERT_STREQ(r2.type().c_str(), "Grass");
        ASSERT_EQ(r2.getCapacity(), Resource::MAX_CAPACITY);
        ASSERT_EQ(r2.getAmount(), 0u);

        Resource const& r3 = script.getResource("People");
        ASSERT_STREQ(r3.type().c_str(), "People");
        ASSERT_EQ(r3.getCapacity(), Resource::MAX_CAPACITY);
        ASSERT_EQ(r3.getAmount(), 0u);
    }

    // -- Path types
    {
        ASSERT_EQ(script.m_pathTypes.size(), 1u);
        PathType const& p1 = script.getPathType("Road");
        ASSERT_STREQ(p1.name.c_str(), "Road");
    }

    // -- Path Way types
    {
        ASSERT_EQ(script.m_segmentTypes.size(), 1u);
        WayType const& s1 = script.getWayType("Dirt");
        ASSERT_STREQ(s1.name.c_str(), "Dirt");
        ASSERT_EQ(s1.color, 0xAAAAAA);
    }

    // -- Agent types:
    {
        ASSERT_EQ(script.m_agentTypes.size(), 2u);
        AgentType const& a1 = script.getAgentType("People");
        //ASSERT_STREQ(a1.name.c_str(), "People");
        ASSERT_EQ(a1.color, 0xFFFF00);
        ASSERT_EQ(a1.speed, 10u);

        AgentType const& a2 = script.getAgentType("Worker");
        //ASSERT_STREQ(a2.name.c_str(), "Worker");
        ASSERT_EQ(a2.color, 0xFFFFFF);
        ASSERT_EQ(a2.speed, 10u);
    }

    // -- Map types
    {
        ASSERT_EQ(script.m_mapTypes.size(), 2u);
        MapType const& m1 = script.getMapType("Water");
        ASSERT_EQ(m1.color, 0x0000FF);
        ASSERT_EQ(m1.capacity, 100u);
        ASSERT_EQ(m1.rules.size(), 0u);

        MapType const& m2 = script.getMapType("Grass");
        ASSERT_EQ(m2.color, 0x00FF00);
        ASSERT_EQ(m2.capacity, 10u);
        //TODO ASSERT_EQ(m2.rules.size(), 1u);
        //TODO ASSERT_STREQ(m2.rules[0].m_name, "CreateGrass");
    }

    // -- Unit types
    {
        ASSERT_EQ(script.m_mapTypes.size(), 2u);
        UnitType const& u1 = script.getUnitType("Home");
        ASSERT_EQ(u1.color, 0xFF00FF);
        ASSERT_EQ(u1.radius, 1u);
        //ASSERT_EQ(u1.rules.size(), 1u);
        // ASSERT_STREQ(u1.rules[0]->m_name, "SendPeopleToWork");
        ASSERT_EQ(u1.targets.size(), 1u);
        ASSERT_STREQ(u1.targets[0].c_str(), "Home");
        ASSERT_EQ(u1.resources.m_bin.size(), 1u);
        ASSERT_EQ(u1.resources.getCapacity("People"), 4u);
        ASSERT_EQ(u1.resources.getAmount("People"), 4u);

        UnitType const& u2 = script.getUnitType("Work");
        ASSERT_EQ(u2.color, 0x00FFFF);
        ASSERT_EQ(u2.radius, 3u);
        //ASSERT_EQ(u2.rules.size(), 2u);
        //ASSERT_STREQ(u2.rules[0]->m_name, "SendPeopleToHome");
        //ASSERT_STREQ(u2.rules[1]->m_name, "UsePeopleToWater");
        ASSERT_EQ(u2.targets.size(), 1u);
        ASSERT_STREQ(u2.targets[0].c_str(), "Work");
        ASSERT_EQ(u2.resources.m_bin.size(), 1u);
        ASSERT_EQ(u2.resources.getCapacity("People"), 2u);
        ASSERT_EQ(u2.resources.getAmount("People"), 0u);
    }

    // -- Map Rules
    {
         ASSERT_EQ(script.m_ruleMaps.size(), 1u);
         RuleMap const& rm1 = script.getRuleMap("CreateGrass");
         ASSERT_STREQ(rm1.m_id.c_str(), "CreateGrass");
         ASSERT_EQ(rm1.m_rate, 7u);
         // TODO map

    }

    // -- Unit Rules
    {
        ASSERT_EQ(script.m_ruleUnits.size(), 3u);
        RuleUnit const& ru1 = script.getRuleUnit("SendPeopleToWork");
        ASSERT_STREQ(ru1.m_id.c_str(), "SendPeopleToWork");
        ASSERT_EQ(ru1.m_rate, 20u);

        RuleUnit const& ru2 = script.getRuleUnit("SendPeopleToHome");
        ASSERT_STREQ(ru2.m_id.c_str(), "SendPeopleToHome");
        ASSERT_EQ(ru2.m_rate, 100u);

        RuleUnit const& ru3 = script.getRuleUnit("UsePeopleToWater");
        ASSERT_STREQ(ru3.m_id.c_str(), "UsePeopleToWater");
        ASSERT_EQ(ru3.m_rate, 5u);

        // TODO local
        // TODO agent
    }
}

TEST(TestsScript, BadSyntax)
{
    (void)system("echo \"foo\" > /tmp/foo");
    Script script("/tmp/foo");
    ASSERT_EQ(script, false);
}
