# Integrating OpenGlassBox into your project

OpenGlassBox ships as a C++14 library. The demo application is optional: the engine is meant to be linked from another application. Your project shall link against the library, load a `.ogs` ruleset, create one or more cities, attach a router, build the world (maps, roads, buildings), and drive the simulation from your own game loop by calling `simulation.update()`.

A working consumer project lives in [LinkAgainstMyLibs/OpenGlassBox](https://github.com/Lecrapouille/LinkAgainstMyLibs/tree/master/OpenGlassBox). The steps below follow its `src/main.cpp`.

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
CXXFLAGS += $(shell pkg-config --cflags OpenGlassBox)
LDFLAGS  += $(shell pkg-config --libs OpenGlassBox)
```

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

1. create a `Simulation`;
2. parse a `.ogs` ruleset into `simulation.script()`;
3. add at least one `City`;
4. attach a router to each city (required before agents can travel);
5. add maps, roads and buildings to the city;
6. call `simulation.update()` from your game loop (or run a fixed number of ticks).

The LinkAgainstMyLibs example embeds its ruleset as a string and builds a small triangle road network with two buildings:

```cpp
#include <OpenGlassBox/OpenGlassBox.hpp>

#include <cstdlib>
#include <iostream>

int main()
{
    ogb::Simulation simulation(64u, 64u);

    if (!simulation.script().parseFile("my_rules.ogs"))
    {
        std::cout << "Error parsing script: "
                  << simulation.script().formatErrors() << std::endl;
        return EXIT_FAILURE;
    }

    ogb::City& city = simulation.addCity("MyCity", ogb::Vector3f(0.f, 0.f, 0.f));
    ogb::installDijkstraRouter(city, simulation.config());

    city.addMap(simulation.script().getMapType("Water"));
    city.addMap(simulation.script().getMapType("Grass"));

    ogb::Path& road = city.addPath(simulation.script().getPathType("Road"));
    ogb::WayType const& dirt = simulation.script().getWayType("Dirt");

    ogb::Node& a = road.addNode(ogb::Vector3f(0.f, 0.f, 0.f));
    ogb::Node& b = road.addNode(ogb::Vector3f(60.f, 0.f, 0.f));
    ogb::Node& c = road.addNode(ogb::Vector3f(30.f, 52.f, 0.f));

    road.addWay(dirt, a, b);
    road.addWay(dirt, b, c);
    road.addWay(dirt, c, a);

    city.addUnit(simulation.script().getUnitType("Home"), a);
    city.addUnit(simulation.script().getUnitType("Work"), b);

    simulation.clock().setTimeOfDay(0u, 8u, 0u);
    simulation.setTotalTicks(simulation.clock().ticks());

    for (uint32_t tick = 0u; tick < 200u; ++tick)
        simulation.update(simulation.config().tickDuration());

    std::cout << city.units().size() << " units, "
              << city.agents().size() << " agents\n";

    return EXIT_SUCCESS;
}
```

In [LinkAgainstMyLibs/OpenGlassBox/src/main.cpp](https://github.com/Lecrapouille/LinkAgainstMyLibs/blob/master/OpenGlassBox/src/main.cpp), instead of `parseFile()`, the full embedded ruleset (`kScript`) is used. It defines two map layers (Water, Grass), a road path, two agent types, and two buildings (Home, Work) whose rules send people back and forth.

In a real game loop, replace the fixed tick loop with wall-time driven updates:

```cpp
while (running) {
    simulation.update(secondsSinceLastFrame);
}
```

`Simulation::update()` converts wall time into fixed simulation ticks. A slow frame rate makes the simulation fall behind; it does not change the rules.

## Loading a ruleset

Rulesets are plain-text `.ogs` files. Parse them through `Script`:

```cpp
if (!simulation.script().parseFile("demo/data/Simulations/test_city.ogs"))
{
    std::cerr << simulation.script().formatErrors();
    return 1;
}
```

You can also load from a string with `script().parseString(source)`, as in the LinkAgainstMyLibs example. On failure, the previous definitions are kept so a running city is not left empty.

The ruleset must **outlive every city** loaded from it: buildings hold references to their recipes (`UnitType`, `WayType`, …). `Simulation` stores its `Script` before its `World` so destruction order remains safe.

## Creating and updating cities

After parsing the ruleset and adding a city, populate it from the script definitions:

```cpp
ogb::City& city = simulation.addCity("MyCity", ogb::Vector3f(0.f, 0.f, 0.f));

city.addMap(simulation.script().getMapType("Water"));
city.addMap(simulation.script().getMapType("Grass"));

ogb::Path& road = city.addPath(simulation.script().getPathType("Road"));
ogb::Node& node = road.addNode(ogb::Vector3f(0.f, 0.f, 0.f));
city.addUnit(simulation.script().getUnitType("Home"), node);
```

Use `simulation.world()` for shared map layers and `simulation.clock()` for the in-game calendar. Rules can depend on time (`hour between 8 18`) and on periods expressed in game minutes (`rate 30 minutes`).

For a single tick outside a loop:

```cpp
city.update(); // one tick at the configured duration
```

## Routing

Agents need an `IRouter` on their city. The engine defines the interface in `OpenGlassBox/Router.hpp`; the default **Dijkstra** implementation and install helpers are in `OpenGlassBox/DijkstraRouter.hpp` (also pulled in by `OpenGlassBox/OpenGlassBox.hpp`):

```cpp
ogb::installDijkstraRouter(city, simulation.config());

// Alternative: for every city already in the simulation:
ogb::installDijkstraRouters(simulation);
```

Install the router **before** agents need to travel. You can provide your own `IRouter` and call `city.setRouter(std::move(router))` instead. See the [traffic documentation](traffic.md) for how travel times and destination search work.

## Loading and saving cities (optional)

The `.ogc` save format and `CitySave` loader live in the demo sources (`demo/src/Save/`). They are not part of the core library, but you can copy or link them if you need save games. A save stores geometry and live state and refers back to the `.ogs` ruleset it was built with.

See the [script language reference](script.md#ogc-save-structure) for the file layout.

## Extension points

| Hook | Purpose |
| ---- | ------- |
| `IRouter` | Replace pathfinding (traffic-aware routing). |
| `IScriptParser` | Plug in another ruleset front end. |
| `World::Listener` | Approve or reject roads crossing city borders. |
| `IRule::Listener` | Log rule attempts and refusals (used by the demo Rule Log). |
| `Simulation::Listener` | React when cities are added or removed. |

## Where to read next

- [Script language reference](script.md): write the gameplay in `.ogs`.
- [Engine documentation](engine.md): classes and tick order.
- [Traffic documentation](traffic.md): travel times, congestion, and destination search.
- [Demo documentation](demo.md): interactive editor and panels.
