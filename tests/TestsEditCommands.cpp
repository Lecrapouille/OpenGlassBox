//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file TestsEditCommands.cpp
//! \brief The editor commands of the demo, and what undoing them gives back.

#include "main.hpp"

#include "Editor/EditCommands.hpp"
#include "OpenGlassBox/Simulation.hpp"

using namespace ogb::editor;

//! \brief A ruleset with one road network and one kind of street. The crossings
//! of a network are what decides whether two streets meet, so the tests that
//! care about it declare their own.
static char const* ROAD_SCRIPT =
    "resources\n"
    "  resource People\n"
    "end\n"
    "paths\n"
    "  path Road color 0xAAAAAA\n"
    "  path Pipe color 0x0000FF crossings false\n"
    "end\n"
    "segments\n"
    "  segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4\n"
    "end\n"
    "buildings\n"
    "  building Home color 0xFF00FF layerRadius 1 rules [ ] targets [ Home ] "
    "caps [ People 4 ] resources [ People 4 ]\n"
    "end\n";

// -----------------------------------------------------------------------------
//! \brief A simulation holding one empty city, ready to be drawn on.
// -----------------------------------------------------------------------------
struct EditableCity
{
    EditableCity()
    {
        EXPECT_TRUE(simulation.loadScriptString(ROAD_SCRIPT))
            << simulation.getRuleset().formatErrors();
        simulation.addCity("Testville", Vector3f(0.0f, 0.0f, 0.0f), 32u, 32u);
    }

    Path& road()
    {
        City& city = *simulation.getCities().begin()->second;
        return city.getPath("Road");
    }

    //! \brief Draw a street, the way the road tool does. The radius is what the
    //! ends snap to what is already there within, in world units.
    bool lay(Vector3f const& from, Vector3f const& to,
             std::string const& pathName = "Road")
    {
        return stack.push(simulation,
                          std::make_unique<AddSegmentCommand>(
                              "Testville", pathName, "Dirt", from, to, 1.0f));
    }

    Simulation simulation;
    CommandStack stack;
};

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, LayingAStreetOverAnotherMakesACrossroads)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));
    ASSERT_EQ(world.road().getNodes().size(), 2u);
    ASSERT_EQ(world.road().getSegments().size(), 1u);

    // A street across the first one: both come out in two halves meeting at a
    // node, which is what lets an agent turn there.
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 20.0f, 0.0f)));
    ASSERT_EQ(world.road().getNodes().size(), 5u);
    ASSERT_EQ(world.road().getSegments().size(), 4u);

    Node const* junction = nullptr;
    for (auto const& node: world.road().getNodes())
    {
        if (node->getSegments().size() == 4u)
            junction = node.get();
    }
    ASSERT_NE(junction, nullptr);
    ASSERT_FLOAT_EQ(junction->getPosition().x, 10.0f);
    ASSERT_FLOAT_EQ(junction->getPosition().y, 10.0f);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, UndoingACrossingStreetSewsTheOtherBackTogether)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));
    uint32_t const streetId = world.road().getSegments()[0]->getId();

    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 20.0f, 0.0f)));
    ASSERT_EQ(world.road().getSegments().size(), 4u);

    world.stack.undo(world.simulation);

    // Back to the street as it was drawn, whole and under the identifier the
    // commands stacked above it refer to.
    ASSERT_EQ(world.road().getNodes().size(), 2u);
    ASSERT_EQ(world.road().getSegments().size(), 1u);
    Segment const* street = world.road().findSegment(streetId);
    ASSERT_NE(street, nullptr);
    ASSERT_FLOAT_EQ(street->getLength(), 20.0f);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, RedoingACrossingStreetCutsItAgain)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 20.0f, 0.0f)));

    world.stack.undo(world.simulation);
    world.stack.redo(world.simulation);

    ASSERT_EQ(world.road().getNodes().size(), 5u);
    ASSERT_EQ(world.road().getSegments().size(), 4u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, AStreetCrossingTwoOthersComesOutInThreePieces)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));
    ASSERT_TRUE(world.lay(Vector3f(0.0f, 20.0f, 0.0f), Vector3f(20.0f, 20.0f, 0.0f)));
    ASSERT_EQ(world.road().getSegments().size(), 2u);

    // Down the two of them: two junctions, so four halves and three pieces.
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 30.0f, 0.0f)));
    ASSERT_EQ(world.road().getSegments().size(), 7u);
    ASSERT_EQ(world.road().getNodes().size(), 8u);

    world.stack.undo(world.simulation);
    ASSERT_EQ(world.road().getSegments().size(), 2u);
    ASSERT_EQ(world.road().getNodes().size(), 4u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, AStreetEndingOnAnotherCutsItWithoutAPieceToSpare)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));

    // A dead end running into the middle of it. The junction is the end of the
    // new street, so there is one piece, not two.
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 10.0f, 0.0f)));
    ASSERT_EQ(world.road().getSegments().size(), 3u);
    ASSERT_EQ(world.road().getNodes().size(), 4u);

    world.stack.undo(world.simulation);
    ASSERT_EQ(world.road().getSegments().size(), 1u);
    ASSERT_EQ(world.road().getNodes().size(), 2u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, PipesPassOverOneAnother)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f),
                          "Pipe"));
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 20.0f, 0.0f),
                          "Pipe"));

    // Nothing was cut: one line running over another says nothing about where
    // they can be joined.
    City& city = *world.simulation.getCities().begin()->second;
    Path& pipes = city.getPath("Pipe");
    ASSERT_EQ(pipes.getSegments().size(), 2u);
    ASSERT_EQ(pipes.getNodes().size(), 4u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, APipeEndingOnAnotherIsLeftHanging)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f),
                          "Pipe"));
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 10.0f, 0.0f),
                          "Pipe"));

    // A network that says its lines do not meet is taken at its word at an end
    // as well as in the middle: the branch is made by hand, with the node tool.
    City& city = *world.simulation.getCities().begin()->second;
    Path& pipes = city.getPath("Pipe");
    ASSERT_EQ(pipes.getSegments().size(), 2u);
    ASSERT_EQ(pipes.getNodes().size(), 4u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, TheNodeToolCutsAStreetAndTheUndoSewsItBack)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));
    uint32_t const streetId = world.road().getSegments()[0]->getId();

    ASSERT_TRUE(world.stack.push(world.simulation,
                                 std::make_unique<SplitSegmentCommand>(
                                     "Testville", "Road", streetId, 0.25f)));

    ASSERT_EQ(world.road().getNodes().size(), 3u);
    ASSERT_EQ(world.road().getSegments().size(), 2u);
    Node const* junction = nullptr;
    for (auto const& node: world.road().getNodes())
    {
        if (node->getSegments().size() == 2u)
            junction = node.get();
    }
    ASSERT_NE(junction, nullptr);
    ASSERT_FLOAT_EQ(junction->getPosition().x, 5.0f);

    world.stack.undo(world.simulation);
    ASSERT_EQ(world.road().getNodes().size(), 2u);
    ASSERT_EQ(world.road().getSegments().size(), 1u);
    ASSERT_NE(world.road().findSegment(streetId), nullptr);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, TheNodeToolIsWhatBranchesAPipe)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f),
                          "Pipe"));

    City& city = *world.simulation.getCities().begin()->second;
    Path& pipes = city.getPath("Pipe");
    uint32_t const pipeId = pipes.getSegments()[0]->getId();

    // A network that makes no crossing of its own still takes the junction the
    // player asks for by hand, which is what the two settings are for.
    ASSERT_TRUE(world.stack.push(world.simulation,
                                 std::make_unique<SplitSegmentCommand>(
                                     "Testville", "Pipe", pipeId, 0.5f)));
    ASSERT_EQ(pipes.getNodes().size(), 3u);
    ASSERT_EQ(pipes.getSegments().size(), 2u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, DraggingACrossroadsTakesItsRoadsWithIt)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(10.0f, 0.0f, 0.0f)));
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(20.0f, 0.0f, 0.0f)));

    Node* middle = nullptr;
    for (auto const& node: world.road().getNodes())
    {
        if (node->getSegments().size() == 2u)
            middle = node.get();
    }
    ASSERT_NE(middle, nullptr);
    uint32_t const middleId = middle->getId();

    ASSERT_TRUE(world.stack.push(
        world.simulation,
        std::make_unique<MoveNodeCommand>("Testville", "Road", middleId,
                                          Vector3f(10.0f, 0.0f, 0.0f),
                                          Vector3f(10.0f, 5.0f, 0.0f))));

    // Both roads keep their ends and are as long as they now look, which is
    // what the router charges an agent for.
    Node const* moved = world.road().findNode(middleId);
    ASSERT_NE(moved, nullptr);
    ASSERT_FLOAT_EQ(moved->getPosition().y, 5.0f);
    for (Segment const* segment: moved->getSegments())
    {
        ASSERT_FLOAT_EQ(segment->getLength(), std::sqrt(125.0f));
    }

    world.stack.undo(world.simulation);
    ASSERT_FLOAT_EQ(world.road().findNode(middleId)->getPosition().y, 0.0f);
    for (Segment const* segment: world.road().findNode(middleId)->getSegments())
    {
        ASSERT_FLOAT_EQ(segment->getLength(), 10.0f);
    }
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, ABuildingOnACrossroadsFollowsIt)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(10.0f, 0.0f, 0.0f)));

    City& city = *world.simulation.getCities().begin()->second;
    Node& corner = *world.road().getNodes()[0];
    uint32_t const cornerId = corner.getId();
    Building& home = city.addBuilding(
        world.simulation.getRuleset().getBuildingType("Home"), corner);

    ASSERT_TRUE(world.stack.push(
        world.simulation,
        std::make_unique<MoveNodeCommand>("Testville", "Road", cornerId,
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 4.0f, 0.0f))));

    ASSERT_FLOAT_EQ(home.getPosition().y, 4.0f);

    world.stack.undo(world.simulation);
    ASSERT_FLOAT_EQ(home.getPosition().y, 0.0f);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, ANodeDraggedNowhereIsNotWorthAnUndo)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 0.0f, 0.0f), Vector3f(10.0f, 0.0f, 0.0f)));
    uint32_t const cornerId = world.road().getNodes()[0]->getId();

    ASSERT_FALSE(world.stack.push(
        world.simulation,
        std::make_unique<MoveNodeCommand>("Testville", "Road", cornerId,
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 0.0f))));
    ASSERT_EQ(world.stack.history().size(), 1u);
}

// -----------------------------------------------------------------------------
TEST(TestsEditCommands, ACutStreetCarryingABuildingIsLeftAlone)
{
    EditableCity world;

    ASSERT_TRUE(world.lay(Vector3f(0.0f, 10.0f, 0.0f), Vector3f(20.0f, 10.0f, 0.0f)));
    ASSERT_TRUE(world.lay(Vector3f(10.0f, 0.0f, 0.0f), Vector3f(10.0f, 20.0f, 0.0f)));

    // A house on the crossroads the second street made.
    City& city = *world.simulation.getCities().begin()->second;
    Node* junction = nullptr;
    for (auto const& node: world.road().getNodes())
    {
        if (node->getSegments().size() == 4u)
            junction = node.get();
    }
    ASSERT_NE(junction, nullptr);
    city.addBuilding(world.simulation.getRuleset().getBuildingType("Home"), *junction);

    world.stack.undo(world.simulation);

    // The pieces of the second street are gone, but the crossroads stays: the
    // player is left a node they did not ask for rather than a house standing
    // on nothing.
    ASSERT_EQ(city.getBuildings().size(), 1u);
    ASSERT_NE(world.road().findNode(junction->getId()), nullptr);
    ASSERT_EQ(world.road().getSegments().size(), 2u);
}
