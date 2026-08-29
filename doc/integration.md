# Integrating OpenGlassBox into your project

OpenGlassBox ships as a C++17 library. The demo application is optional: the engine is meant to be linked from another application. Your project links against the library, loads a `.ogs` ruleset, founds one or more cities, gives each a router, builds what the cities are made of (layers, roads, buildings), and drives the game from its own loop by calling `simulation.update()`.

A working consumer project lives in [LinkAgainstMyLibs/OpenGlassBox](https://github.com/Lecrapouille/LinkAgainstMyLibs/tree/master/OpenGlassBox). The steps below follow its `src/main.cpp`, comments included.

## Build and install the library

From the OpenGlassBox repository root:

```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
cd OpenGlassBox/
make download-external-libs
make -j8
sudo make install   # optional
```

After `make`, the build folder contains `libOpenGlassBox.a` and, on most platforms, `libOpenGlassBox.so`. Headers live in `include/OpenGlassBox/`, with the entry point header `OpenGlassBox/OpenGlassBox.hpp`.

After `make install`, pkg-config can find the library:

```sh
pkg-config --cflags --libs OpenGlassBox
```

For static linking:

```sh
pkg-config --static --cflags --libs OpenGlassBox
```

## Link your project

### With pkg-config (recommended after install)

```makefile
CXXFLAGS += -std=c++17 $(shell pkg-config --cflags OpenGlassBox)
LDFLAGS  += $(shell pkg-config --libs OpenGlassBox)
```

Say `-std=c++17` yourself: pkg-config does not carry a language standard, and the public headers use `std::optional`.

For a static link, pass `--static` to pkg-config in `LDFLAGS`.

### Working example

The [LinkAgainstMyLibs](https://github.com/Lecrapouille/LinkAgainstMyLibs) repository contains a minimal consumer project:

```sh
git clone https://github.com/Lecrapouille/LinkAgainstMyLibs.git --recursive
cd LinkAgainstMyLibs/OpenGlassBox
make -j8
./build/OpenGlassBox
```

If OpenGlassBox is not installed system-wide, set `PKG_CONFIG_PATH` to its `build/` folder before running `make`.

## Minimal program

Every host follows the same pattern:

1. create a `Simulation`, optionally with a `Config`;
2. load a `.ogs` ruleset with `simulation.loadScriptFile()`;
3. found at least one `City`;
4. give each city a router, before anything travels;
5. add the layers, the roads and the buildings;
6. call `simulation.update()` from the game loop.

Here is the whole of it, with the reasons behind the order:

```cpp
#include <OpenGlassBox/OpenGlassBox.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    // Runtime settings. Everything has a default, so only say what differs
    // from it: here, how many cells wide a city is when no size is given.
    ogb::Config config;
    config.grid.defaultCitySizeU = 64u;
    config.grid.defaultCitySizeV = 64u;

    // The whole game. This is the only object an application has to hold: the
    // ruleset, the world, the cities and the clock all live inside it.
    ogb::Simulation simulation(config);

    // Load the gameplay before founding anything. A building keeps a reference
    // to the recipe it was built from, and those recipes live in the ruleset,
    // so the ruleset has to be in place first and outlive every city.
    if (!simulation.loadScriptFile("my_rules.ogs"))
    {
        std::cerr << simulation.formatScriptErrors() << std::endl;
        return EXIT_FAILURE;
    }

    // Found a city. The size comes from GridConfig above; the position is
    // where its top-left cell sits in the world.
    ogb::City& city = simulation.addCity("MyCity", ogb::Vector3f(0.f, 0.f, 0.f));

    // Give the city a router before anything travels. An agent with no router
    // never leaves the crossroads it was sent from, and nothing says so: the
    // city simply looks asleep.
    ogb::installDijkstraRouter(city, simulation.getConfig());

    // Ask for the layers of the environment the rules read and write. They are
    // owned by the world and shared by every city, so this creates each one
    // once whatever how many cities ask.
    ogb::Ruleset const& rules = simulation.getRuleset();
    city.addLayer(rules.getLayerType("Water"));
    city.addLayer(rules.getLayerType("Grass"));

    // A network of roads, and the recipe its segments are built from. An agent
    // never leaves the network it started on, so a city with a road network
    // and a rail network has two of these.
    ogb::Path& road = city.addPath(rules.getPathType("Road"));
    ogb::SegmentType const& dirt = rules.getSegmentType("Dirt");

    // Three crossroads, in world coordinates.
    ogb::Node& a = road.addNode(ogb::Vector3f(0.f, 0.f, 0.f));
    ogb::Node& b = road.addNode(ogb::Vector3f(60.f, 0.f, 0.f));
    ogb::Node& c = road.addNode(ogb::Vector3f(30.f, 52.f, 0.f));

    // The streets joining them. Segments are undirected: one-way traffic is
    // not modelled.
    road.addSegment(dirt, a, b);
    road.addSegment(dirt, b, c);
    road.addSegment(dirt, c, a);

    // Two buildings, each standing on a crossroads so that agents can reach
    // them. A building may also stand along a street, at an offset.
    city.addBuilding(rules.getBuildingType("Home"), a);
    city.addBuilding(rules.getBuildingType("Work"), b);

    // Open the working day. Rules may be written as "hour between 8 18", so
    // starting at midnight would mean watching a city where nothing is awake.
    simulation.setTimeOfDay(0u, 8u, 0u);

    // A new simulation starts paused, so that a game can be built before
    // anything moves.
    simulation.setPaused(false);

    // update() is handed seconds of wall time, not ticks. Here there is no
    // frame to wait for, so feed it exactly one tick at a time.
    for (uint32_t tick = 0u; tick < 200u; ++tick)
    {
        simulation.update(simulation.getConfig().time.tickDuration());
    }

    std::cout << city.getBuildings().size() << " buildings, "
              << city.getAgents().size() << " agents\n";

    return EXIT_SUCCESS;
}
```

In [LinkAgainstMyLibs/OpenGlassBox/src/main.cpp](https://github.com/Lecrapouille/LinkAgainstMyLibs/blob/master/OpenGlassBox/src/main.cpp) the ruleset is embedded as a string and loaded with `loadScriptString()` instead of `loadScriptFile()`. It declares two layers (Water, Grass), a road network, two kinds of traveller, and two buildings (Home, Work) whose rules send people back and forth.

In a real game loop, replace the fixed tick loop with wall-time driven updates:

```cpp
while (running)
{
    simulation.update(secondsSinceLastFrame);
}
```

`update()` takes seconds rather than ticks so that the game advances at the same rate whatever the frame rate: it scales the seconds by the speed the player chose, accumulates them, and runs as many fixed ticks as fit. What does not fill a tick is kept for the next call, so no game time is lost or invented. A slow machine falls behind; it does not simulate differently.

## Loading a ruleset

Rulesets are plain-text `.ogs` files, loaded through the simulation:

```cpp
if (!simulation.loadScriptFile("demo/data/Simulations/sandbox.ogs"))
{
    std::cerr << simulation.formatScriptErrors();
    return 1;
}
```

`loadScriptString(source)` does the same with text already in memory, as in the LinkAgainstMyLibs example. On failure the previous definitions are kept, so a bad reload leaves a running game alone rather than emptying it. `getScriptErrors()` returns the errors one by one, with a line and a column each, when a formatted block is not what you want.

What the script declared is read back through `simulation.getRuleset()`, which hands out a `Ruleset const&`: the recipes may be looked up but not changed, since the cities hold references into them. `getXxx()` throws when a name was never declared; `findXxx()` returns `nullptr` instead, for asking whether a name exists.

The ruleset must **outlive every city** loaded from it: buildings hold references to their recipes (`BuildingType`, `SegmentType`, …). `Simulation` declares its `Ruleset` before its `World`, so destruction, which runs in reverse, takes the cities away first.

## Creating and updating cities

`Simulation` is the way in for anything that changes the game, and the object that holds something is the way in for reading it. So a city is founded through the simulation and read through the `City` it returns:

```cpp
ogb::City& city = simulation.addCity("MyCity", ogb::Vector3f(0.f, 0.f, 0.f));

ogb::Ruleset const& rules = simulation.getRuleset();
city.addLayer(rules.getLayerType("Water"));

ogb::Path& road = city.addPath(rules.getPathType("Road"));
ogb::Node& node = road.addNode(ogb::Vector3f(0.f, 0.f, 0.f));
city.addBuilding(rules.getBuildingType("Home"), node);
```

The `World` behind all this has no accessor: everything it offers is on `Simulation`. Layers shared by every city are reached with `simulation.findLayer("Water")` and `simulation.getLayers()`, the grid with `simulation.worldToCell()` and `simulation.cellToWorld()`, and a road crossing a border with `simulation.addRoad()`.

The calendar is read through `simulation.getClock()`, which is const: move it with `simulation.setTimeOfDay(day, hour, minute)` or, when loading a save, `simulation.setTicks(ticks)`. Rules may depend on the time of day (`hour between 8 18`) and on periods written in game minutes (`rate 30 minutes`).

For a single tick outside a loop:

```cpp
simulation.stepOneTick(); // ignores the pause and the time scale
city.update();            // one city only, at the configured tick duration
```

## Routing

Agents need an `IRouter` on their city. The interface is in `OpenGlassBox/Router.hpp`, and the default **Dijkstra** implementation, along with the two helpers that install it, in `OpenGlassBox/DijkstraRouter.hpp`. Both come with `OpenGlassBox/OpenGlassBox.hpp`:

```cpp
ogb::installDijkstraRouter(city, simulation.getConfig());

// Or, for every city already founded, which is what a save loader calls:
ogb::installDijkstraRouters(simulation);
```

Install the router **before** agents need to travel: an agent whose city has none never leaves the crossroads it was sent from, and nothing reports it. To use your own, call `city.setRouter(std::move(router))` instead. See the [traffic documentation](traffic.md) for how travel times and destination search work.

## Runtime settings

`ogb::Config` gathers the settings in four groups: `time` (tick rate, ticks per game minute, hour of the day to start at), `grid` (cell size, default city size), `traffic` (how fast the flow averages settle, how many agents the metrics sample) and `routing` (how often an agent looks for a better itinerary, when it gives up). It is copied into the simulation, so changing one at runtime means reading the current settings, editing them, and handing them back:

```cpp
ogb::Config config = simulation.getConfig();
config.time.ticksPerSecond = 20.0f;
simulation.setConfig(config);
```

## Loading and saving cities (optional)

The `.ogc` save format and the `CitySave` loader live in the demo sources (`demo/src/Save/`). They are not part of the core library, but you can copy or link them if you need save games. A save stores the geometry and the live state, and refers back to the `.ogs` ruleset it was built with.

See the [script language reference](script.md#ogc-save-structure) for the file layout.

## Extension points

| Hook | Purpose |
| ---- | ------- |
| `IRouter` | Replace pathfinding (traffic-aware routing). |
| `IScriptParser` | Plug in another ruleset front end. |
| `IRule::Listener` | Log rule attempts and refusals (used by the demo Rule Log). |
| `SimulationListener` | React to cities appearing and going away, and approve roads crossing a city border. Aliased as both `Simulation::Listener` and `World::Listener`. |

## Where to read next

- [Naming conventions](naming.md): what `get`, `find`, `is` and `add` mean here.
- [Script language reference](script.md): write the gameplay in `.ogs`.
- [Engine documentation](engine.md): classes, class diagram and tick order.
- [Traffic documentation](traffic.md): travel times, congestion, and destination search.
- [Demo documentation](demo.md): interactive editor and panels.
