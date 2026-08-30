//-----------------------------------------------------------------------------
// Copyright (c) 2020-2026 Quentin Quadrat.
// https://github.com/Lecrapouille/OpenGlassBox
// Distributed under MIT License.
//-----------------------------------------------------------------------------

//! \file TestsScenario.cpp
//! \brief Runs the shipped saves for a few game hours and checks that the city
//! actually lives: houses appear along the roads, workers reach the factories,
//! goods are made and hauled to the shops, and people break for lunch.
//!
//! These are the checks a unit test on a single class cannot make, because
//! every one of them needs the clock, the rules, the graph and the agents at
//! the same time.

#include "main.hpp"

#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "Save/CitySave.hpp"

#include <fstream>
#include <functional>
#include <stdexcept>

//------------------------------------------------------------------------------
static std::string dataFile(std::string const& name)
{
    std::string const candidates[] = {
        "../demo/data/simulations/" + name,
        "demo/data/simulations/" + name,
    };
    for (std::string const& path : candidates)
    {
        std::ifstream file(path);
        if (file.good())
            return path;
    }
    return candidates[0];
}

//------------------------------------------------------------------------------
//! \brief A Building of the given type, or nullptr. The first one found is
//! enough: the saves used here hold one of each.
static Building* findBuilding(City& city, std::string const& type)
{
    for (auto const& building : city.getBuildings())
    {
        if (building->getTypeName() == type)
            return building.get();
    }
    return nullptr;
}

//------------------------------------------------------------------------------
//! \brief Whether a Building answers to the given name.
//!
//! The checks below look for a role and not for a type, the way an Agent does:
//! the ruleset names its houses Shack, House and Villa according to how rich
//! the district became, and all three answer to "Home". Asking for a type would
//! make this file depend on which rung of the ladder the city happened to be
//! on.
//!
//! This reads the targets rather than calling Building::accepts, which also
//! asks whether there is room for a load. A full shop is still a shop.
static bool answersTo(Building const& building, std::string const& target)
{
    for (Name const& name : building.getTargets())
    {
        if (name == target)
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
static Building* findByTarget(City& city, std::string const& target)
{
    for (auto const& building : city.getBuildings())
    {
        if (answersTo(*building, target))
            return building.get();
    }
    return nullptr;
}

//------------------------------------------------------------------------------
static uint32_t countByTarget(City& city, std::string const& target)
{
    uint32_t count = 0u;
    for (auto const& building : city.getBuildings())
    {
        if (answersTo(*building, target))
            ++count;
    }
    return count;
}

//------------------------------------------------------------------------------
static bool
hasAgent(City& city, std::string const& type, std::string const& target)
{
    for (auto const& agent : city.getAgents())
    {
        if ((agent->getTypeName() == type) && (agent->getTarget() == target))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
//! \brief What the scenario is watching for. Each flag is latched the first
//! time it happens, with the game hour it happened at, because most of these
//! events do not last: a shop sells the goods it receives within the hour.
struct Sightings
{
    bool home = false;
    bool peopleAtWork = false;
    bool goodsAtWork = false;
    bool goodsAtShop = false;
    bool diner = false;
    bool peopleAtRestaurant = false;
    bool backToWork = false;
    bool shopper = false;
    bool peopleAtShop = false;
    //! \brief Somebody stood in a restaurant before lunch time. Nobody is sent
    //! to one before noon, so that is a customer who was looking for a shop and
    //! took the canteen for one.
    bool restaurantBeforeLunch = false;
    uint32_t lunchHour = 99u;
};

//------------------------------------------------------------------------------
static Sightings watch(Simulation& simulation,
                       City& city,
                       uint32_t ticks,
                       std::function<void()> const& onTick = nullptr)
{
    Sightings seen;

    for (uint32_t i = 0u; i < ticks; ++i)
    {
        simulation.stepOneTick();
        if (onTick != nullptr)
            onTick();

        uint32_t const hour = simulation.getClock().getHourOfDay();
        Building* const work = findByTarget(city, "Work");
        Building* const shop = findByTarget(city, "Shop");
        Building* const restaurant = findByTarget(city, "Restaurant");

        seen.home = seen.home || (countByTarget(city, "Home") > 0u);
        if (work != nullptr)
        {
            seen.peopleAtWork = seen.peopleAtWork ||
                                (work->getResources().getAmount("People") > 0u);
            seen.goodsAtWork = seen.goodsAtWork ||
                               (work->getResources().getAmount("Goods") > 0u);
        }
        if (shop != nullptr)
        {
            seen.goodsAtShop = seen.goodsAtShop ||
                               (shop->getResources().getAmount("Goods") > 0u);
            seen.peopleAtShop = seen.peopleAtShop ||
                                (shop->getResources().getAmount("People") > 0u);
        }
        seen.shopper = seen.shopper || hasAgent(city, "Shopper", "Shop");
        if (hasAgent(city, "Diner", "Restaurant"))
        {
            if (!seen.diner)
                seen.lunchHour = hour;
            seen.diner = true;
        }
        if (restaurant != nullptr)
        {
            bool const busy =
                (restaurant->getResources().getAmount("People") > 0u);
            seen.peopleAtRestaurant = seen.peopleAtRestaurant || busy;
            seen.restaurantBeforeLunch =
                seen.restaurantBeforeLunch || (busy && (hour < 12u));
        }
        if ((hour >= 13u) && hasAgent(city, "Worker", "Work"))
        {
            seen.backToWork = true;
        }
    }

    return seen;
}

//------------------------------------------------------------------------------
//! \brief Load a save against the ruleset it was written for, and open the
//! working day. A save carries the tick it was written at, and qq.ogc was
//! written at half past midnight, when no rule of the commute is awake.
static City& openAtEightInTheMorning(Simulation& simulation,
                                     std::string const& save)
{
    EXPECT_TRUE(simulation.loadScriptFile(dataFile("sandbox.ogs")))
        << simulation.getRuleset().formatErrors();

    CitySaveHeader header;
    std::string error;
    EXPECT_TRUE(CitySave::peekHeader(dataFile(save), header, error)) << error;
    // The saves carry the fingerprint of the ruleset. Changing the rules means
    // refreshing it, and a stale one is exactly what this catches.
    EXPECT_TRUE(
        CitySave::matchesRuleset(header, dataFile("sandbox.ogs"), error))
        << error;
    EXPECT_TRUE(CitySave::read(dataFile(save), simulation, error)) << error;
    installDijkstraRouters(simulation);

    simulation.setTimeOfDay(0u, 8u, 0u);

    // A load that failed leaves no City behind, and every check below would
    // read freed memory instead of naming what went wrong.
    if (simulation.getCities().empty())
        throw std::runtime_error("'" + save + "' loaded no city");

    return *(simulation.getCities().begin()->second);
}

//------------------------------------------------------------------------------
//! \brief From eight in the morning to three in the afternoon on the two-zone
//! save: the whole chain from an empty plot to a shop with something to sell.
TEST(TestsScenario, ADayInQqCity)
{
    Simulation simulation;
    City& city = openAtEightInTheMorning(simulation, "qq.ogc");

    ASSERT_EQ(countByTarget(city, "Home"), 0u);
    ASSERT_NE(findByTarget(city, "Work"), nullptr);
    ASSERT_NE(findByTarget(city, "Shop"), nullptr);

    // The Commercial zone of that save is a single cell and already holds a
    // shop, so the canteen goes next to it by hand. What is under test here is
    // the lunch rules, not where a zone puts a building.
    Path& road = *(city.getPaths().begin()->second);
    Segment* const segment = road.findSegment(3u);
    ASSERT_NE(segment, nullptr);
    city.addBuilding(simulation.getRuleset().getBuildingType("Restaurant"),
                     road,
                     *segment,
                     0.6f);
    ASSERT_NE(findBuilding(city, "Restaurant"), nullptr);

    // Nine game hours: the zone takes two to grow its first house, and the
    // chain from a new resident to a shop with something on its shelves is a
    // commute, a half-hourly production and a delivery long. The window has to
    // cover the lunch hours as well, which is why it reaches five in the
    // afternoon rather than stopping at noon.
    Sightings const seen = watch(simulation, city, 9u * 60u * 20u);

    ASSERT_TRUE(seen.home) << "the Residential zones grew nothing";
    ASSERT_TRUE(seen.peopleAtWork) << "nobody ever reached the factory";
    ASSERT_TRUE(seen.goodsAtWork) << "the factory produced nothing";
    ASSERT_TRUE(seen.goodsAtShop) << "no freight ever reached a shop";
    ASSERT_TRUE(seen.diner) << "nobody went for lunch";
    ASSERT_GE(seen.lunchHour, 12u);
    ASSERT_LE(seen.lunchHour, 14u);
    ASSERT_TRUE(seen.peopleAtRestaurant) << "the diners never arrived";
    ASSERT_TRUE(seen.backToWork) << "the diners never went back to work";

    // The morning commute used to empty the houses, so the shops opened onto
    // an empty street and every agent of the city was a worker.
    ASSERT_TRUE(seen.shopper) << "nobody ever went shopping";
    ASSERT_TRUE(seen.peopleAtShop) << "the shoppers never reached a shop";

    // A restaurant used to answer to Shop as well as to Restaurant, so the
    // shoppers, and the freight bound for the shops, stopped at the canteen.
    ASSERT_FALSE(seen.restaurantBeforeLunch)
        << "somebody was served at the restaurant before noon";

    // Selling is what pays for the city.
    ASSERT_GT(city.getGlobals().getAmount("Money"), 0u);

    // Houses stand along a road, not in a field, and no two share a cell.
    std::vector<Vector3f> homes;
    for (auto const& building : city.getBuildings())
    {
        if (!answersTo(*building, "Home"))
            continue;
        ASSERT_TRUE((building->getSegment() != nullptr) ||
                    (building->getNode() != nullptr))
            << "a house grew with no road to reach it";
        for (Vector3f const& other : homes)
        {
            ASSERT_GT(length(building->getPosition() - other), 0.1f)
                << "two houses on the same spot";
        }
        homes.push_back(building->getPosition());
    }
}

//------------------------------------------------------------------------------
//! \brief The claims Agents hold on their destinations balance out.
//!
//! A building whose count never comes back down is invisible to every Agent for
//! the rest of the game, which would be worse than the crowding the claims
//! prevent. The invariant is exact: over a whole city, the claims outstanding
//! are the Agents that have somewhere to go. A drift means an itinerary was
//! written somewhere other than Agent::route.
TEST(TestsScenario, ClaimsOnDestinationsNeverLeak)
{
    Simulation simulation;
    City& city = openAtEightInTheMorning(simulation, "qq.ogc");

    // Long enough for houses to grow, commutes to run, buildings to be
    // demolished and Agents to give up: every way a trip can end.
    for (uint32_t tick = 0u; tick < 4u * 60u * 20u; ++tick)
    {
        simulation.stepOneTick();

        uint32_t claimed = 0u;
        for (auto const& building : city.getBuildings())
            claimed += building->getReservedCount();

        uint32_t bound = 0u;
        for (auto const& agent : city.getAgents())
        {
            if (agent->getRoute().getDestination() != nullptr)
                ++bound;
        }

        ASSERT_EQ(claimed, bound)
            << "at tick " << tick << ": " << claimed << " places held for "
            << bound << " Agents with somewhere to go";
    }
}

//------------------------------------------------------------------------------
//! \brief The slow variables have to be slow. Desirability used to move every
//! ten ticks, half a game minute, so a district was built and abandoned within
//! the hour; and pollution grew faster than it faded, saturated its cap and
//! pinned desirability to zero for the rest of the run.
TEST(TestsScenario, PollutionFadesAndDesirabilityMovesInDays)
{
    Simulation simulation;
    City& city = openAtEightInTheMorning(simulation, "qq.ogc");

    Layer const& pollution = city.getLayer("Pollution");
    Layer const& desirability = city.getLayer("Desirability");

    // A cell no polluting building reaches. The factory of that save stands at
    // (10,4) and reads three cells around itself, and the coal plant stands at
    // (1,5) and reads six, so the far corner is the only one outside both. The
    // near corner used to serve, until the save gained the power plant a
    // residential zone needs to grow at all.
    int32_t const u = 11;
    int32_t const v = 11;
    uint32_t const pollutionAtStart = pollution.getResource({ u, v });
    uint32_t const desirabilityAtStart = desirability.getResource({ u, v });
    ASSERT_GT(pollutionAtStart, 0u) << "this save is meant to start polluted";

    uint32_t const homesAtStart = countByTarget(city, "Home");
    uint32_t homesSeen = homesAtStart;
    uint32_t pollutionPeak = pollutionAtStart;

    // One game hour.
    for (uint32_t i = 0u; i < 60u * 20u; ++i)
    {
        simulation.stepOneTick();

        uint32_t const homes = countByTarget(city, "Home");
        ASSERT_GE(homes, homesSeen) << "a house was demolished within the hour "
                                       "it was built";
        homesSeen = homes;
        pollutionPeak =
            std::max(pollutionPeak, pollution.getResource({ u, v }));
    }

    // Two hourly rules at most, each moving it by two.
    uint32_t const afterAnHour = desirability.getResource({ u, v });
    uint32_t const moved = (afterAnHour > desirabilityAtStart)
                               ? (afterAnHour - desirabilityAtStart)
                               : (desirabilityAtStart - afterAnHour);
    ASSERT_LE(moved, 2u) << "desirability still swings within the hour";

    // Five more hours: what fouls a district is the factories, not the passing
    // of time, so a corner far from one gets cleaner.
    for (uint32_t i = 0u; i < 5u * 60u * 20u; ++i)
    {
        simulation.stepOneTick();
        pollutionPeak =
            std::max(pollutionPeak, pollution.getResource({ u, v }));
    }

    ASSERT_LT(pollution.getResource({ u, v }), pollutionAtStart)
        << "pollution never fades";
    ASSERT_LT(pollutionPeak, pollution.getCellCapacity())
        << "pollution saturated its cap again";
}

//------------------------------------------------------------------------------
//! \brief The other save has a single zone. Growth and production still have to
//! start, and nothing may crash for want of a second one.
TEST(TestsScenario, ADayInQq2City)
{
    Simulation simulation;
    City& city = openAtEightInTheMorning(simulation, "qq2.ogc");

    // The factory of that save stands at a fifth of the first street, and the
    // west end of it is a dead end: nothing stands there and no road goes on.
    // Everything the factory deals with is on the other side, so an Agent seen
    // west of its door left the building by the wrong end, which is what every
    // truck bound for the shop used to do before turning back at the corner.
    Building* const work = findByTarget(city, "Work");
    ASSERT_NE(work, nullptr);
    Segment* const street = work->getSegment();
    ASSERT_NE(street, nullptr);
    ASSERT_EQ(street->getFrom().getSegments().size(), 1u);
    ASSERT_TRUE(street->getFrom().getBuildings().empty());

    float const door = work->getSegmentOffset();
    std::string wrongSegment;
    Sightings const seen =
        watch(simulation,
              city,
              6u * 60u * 20u,
              [&]()
              {
                  for (auto const& agent : city.getAgents())
                  {
                      if (!wrongSegment.empty())
                          return;
                      if ((agent->getSegment() != street) ||
                          (agent->getOffset() >= door - 0.01f))
                      {
                          continue;
                      }
                      wrongSegment =
                          agent->getTypeName().str() + " looking for " +
                          agent->getTarget().str() +
                          (agent->getRoute().isFound() ? " with a route"
                                                       : " with none");
                  }
              });

    ASSERT_TRUE(wrongSegment.empty())
        << wrongSegment
        << " drove to the dead end behind the factory instead of "
           "towards its destination";

    // The four buildings of that save are placed by hand, and the canteen sits
    // closer to the factory than the shop does. While a restaurant answered to
    // Shop, that is where the freight and the shoppers went, and the shop was
    // never opened at all.
    ASSERT_TRUE(seen.goodsAtShop) << "no freight ever reached the shop";
    ASSERT_TRUE(seen.peopleAtShop) << "no customer ever reached the shop";
    ASSERT_FALSE(seen.restaurantBeforeLunch)
        << "somebody was served at the restaurant before noon";

    ASSERT_TRUE(seen.home || (countByTarget(city, "Shop") > 0u) ||
                (countByTarget(city, "Work") > 0u))
        << "the zone grew nothing at all";

    // Whatever grew, it hangs off the network.
    for (auto const& building : city.getBuildings())
    {
        ASSERT_TRUE((building->getSegment() != nullptr) ||
                    (building->getNode() != nullptr))
            << building->getTypeName() << " grew with no road to reach it";
    }

    // No Agent is left floating with nothing under it.
    for (auto const& agent : city.getAgents())
    {
        ASSERT_TRUE((agent->getSegment() != nullptr) ||
                    (agent->getPreviousNode() != nullptr));
    }
}

//------------------------------------------------------------------------------
//! \brief An hour on the imported Chicago network, which is two orders of
//! magnitude larger than the hand-drawn saves.
//!
//! The router keeps its bookkeeping in arrays indexed by Node::index(), sized
//! to the network and stamped rather than cleared. A city of thousands of
//! crossroads is what tells whether that indexing holds: a stale stamp or an
//! index gone out of step would show up here as a crash or as agents that never
//! arrive, and nowhere else.
TEST(TestsScenario, AnHourOnTheChicagoNetwork)
{
    Simulation simulation;
    ASSERT_TRUE(simulation.loadScriptFile(dataFile("sandbox.ogs")))
        << simulation.getRuleset().formatErrors();

    std::string error;
    ASSERT_TRUE(CitySave::read(dataFile("chicago.ogc"), simulation, error))
        << error;
    installDijkstraRouters(simulation);

    ASSERT_FALSE(simulation.getCities().empty());
    City& city = *(simulation.getCities().begin()->second);

    Path const& road = *(city.getPaths().begin()->second);
    ASSERT_GT(road.getNodeCount(), 100u)
        << "not the large network this is about";

    simulation.setTimeOfDay(0u, 8u, 0u);
    for (uint32_t tick = 0u; tick < 60u * 20u; ++tick)
    {
        simulation.update(simulation.getConfig().time.tickDuration());
    }

    // The indices stay dense and in step with the list they number, which is
    // the invariant the router indexes by.
    uint32_t expected = 0u;
    for (auto const& node : road.getNodes())
    {
        ASSERT_EQ(node->getIndex(), expected++);
    }

    for (auto const& agent : city.getAgents())
    {
        ASSERT_TRUE((agent->getSegment() != nullptr) ||
                    (agent->getPreviousNode() != nullptr));
    }
}
