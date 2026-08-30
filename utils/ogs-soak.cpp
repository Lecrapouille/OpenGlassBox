// Throwaway soak test: runs a save for a number of game days and reports, per
// rule, how many attempts succeeded and which command blocked the rest.
//
// The question it answers is the one the Rule Log answers in the demo, without
// having to sit in front of it: is any rule blocked for ever, and if so on what.
//
// usage: ogs-soak <ruleset.ogs> <city.ogc> [days]
#include "OpenGlassBox/DijkstraRouter.hpp"
#include "OpenGlassBox/Simulation.hpp"
#include "Save/CitySave.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace ogb;

namespace
{
struct Tally
{
    uint64_t attempts = 0u;
    uint64_t successes = 0u;
    std::map<std::string, uint64_t> blockedBy;
};

class Counter: public IRule::Listener
{
public:

    void onRuleExecuted(IRule::Trace const& trace) override
    {
        Tally& tally = m_tallies[std::string(trace.rule->getName())];
        tally.attempts += 1u;
        if (trace.success)
        {
            tally.successes += 1u;
            return;
        }
        auto const& commands = trace.rule->getCommands();
        std::string blocker = "?";
        if (trace.blockingCommand < commands.size())
            blocker = commands[trace.blockingCommand]->getDescription();
        tally.blockedBy[blocker] += 1u;
    }

    std::map<std::string, Tally> const& tallies() const { return m_tallies; }

private:

    std::map<std::string, Tally> m_tallies;
};
} // namespace

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "usage: ogs-soak <ruleset.ogs> <city.ogc> [days]\n";
        return 2;
    }
    uint32_t const days = (argc > 3) ? uint32_t(std::atoi(argv[3])) : 3u;

    Simulation simulation;
    if (!simulation.loadScriptFile(argv[1]))
    {
        std::cerr << simulation.getRuleset().formatErrors() << '\n';
        return 1;
    }

    std::string error;
    if (!CitySave::read(argv[2], simulation, error))
    {
        std::cerr << error << '\n';
        return 1;
    }
    installDijkstraRouters(simulation);
    simulation.setTimeOfDay(0u, 8u, 0u);

    Counter counter;
    IRule::setListener(&counter);

    uint32_t const perMinute = simulation.getClock().getTicksPerMinute();
    uint64_t const ticks = uint64_t(days) * 24u * 60u * perMinute;
    for (uint64_t tick = 0u; tick < ticks; ++tick)
        simulation.stepOneTick();

    IRule::setListener(nullptr);

    // The rules that never once ran are the finding. Everything else is noise:
    // a rule that fires nine times out of ten is a rule that works.
    std::vector<std::string> never;
    std::vector<std::string> rare;
    for (auto const& it : counter.tallies())
    {
        if (it.second.successes == 0u)
            never.push_back(it.first);
        else if (it.second.successes * 20u < it.second.attempts)
            rare.push_back(it.first);
    }

    std::cout << argv[2] << ": " << days << " day(s), "
              << counter.tallies().size() << " rule(s) attempted, "
              << never.size() << " never ran, " << rare.size()
              << " ran under 5% of the time\n\n";

    auto report = [&](char const* title, std::vector<std::string> const& names)
    {
        if (names.empty())
            return;
        std::cout << title << '\n';
        for (std::string const& name : names)
        {
            Tally const& tally = counter.tallies().at(name);
            std::cout << "  " << std::left << std::setw(34) << name
                      << tally.successes << "/" << tally.attempts;
            // The command that blocked the most attempts is the one to fix.
            auto worst = std::max_element(tally.blockedBy.begin(),
                                          tally.blockedBy.end(),
                                          [](auto const& a, auto const& b)
                                          { return a.second < b.second; });
            if (worst != tally.blockedBy.end())
                std::cout << "   blocked on: " << worst->first;
            std::cout << '\n';
        }
        std::cout << '\n';
    };

    report("Never ran:", never);
    report("Ran rarely:", rare);

    std::cout << "City accounts after " << days << " day(s):\n";
    for (auto& it : simulation.getCities())
    {
        City& city = *it.second;
        std::cout << "  " << city.getName() << ": "
                  << city.getBuildings().size() << " building(s), "
                  << city.getAgents().size() << " agent(s)\n";
        for (Resource const& resource : city.getGlobals().getAll())
        {
            std::cout << "    " << std::left << std::setw(22)
                      << resource.getTypeName() << resource.getAmount() << '\n';
        }
        std::map<std::string, uint32_t> byType;
        for (auto const& building : city.getBuildings())
            byType[std::string(building->getTypeName())] += 1u;
        for (auto const& it2 : byType)
            std::cout << "    " << std::left << std::setw(22) << it2.first
                      << it2.second << '\n';

        std::cout << "  Ground (total over the map, then the busiest cell):\n";
        for (auto const& layerIt : city.getLayers())
        {
            Layer const& layer = *layerIt.second;
            uint64_t total = 0u;
            uint32_t peak = 0u;
            for (int32_t v = 0; v < int32_t(city.getRegion().sizeV); ++v)
            {
                for (int32_t u = 0; u < int32_t(city.getRegion().sizeU); ++u)
                {
                    uint32_t const held = layer.getResource({ u, v });
                    total += held;
                    peak = std::max(peak, held);
                }
            }
            std::cout << "    " << std::left << std::setw(22)
                      << std::string(layerIt.first) << total << "  peak "
                      << peak << " of " << layer.getCellCapacity() << '\n';
        }
    }

    return 0;
}
