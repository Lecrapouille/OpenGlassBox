#include "main.hpp"

#include "OpenGlassBox/CellsInRadius.hpp"

int main(int argc, char *argv[])
{
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);

    // A Rule that writes on the ground fills the cells of its reach in a random
    // order, so where that order starts decides the state of the map. The
    // engine takes that order from the operating system, which is right for a
    // game and wrong for a test: a scenario that measures the ground after a
    // few game hours would then hold for one run only. One fixed sequence for
    // the whole suite makes every run repeat the previous one.
    ogb::CellsInRadius::setSeed(20260830u);

    return RUN_ALL_TESTS();
}
