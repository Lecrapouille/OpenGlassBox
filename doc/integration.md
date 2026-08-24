# Integrating OpenGlassBox into your project

OpenGlassBox ships as a C++14 library. The demo application is optional: your project links against the library, loads a ruleset, creates one or more cities, and drives the simulation from your own game loop.

## Build and install the library

From the OpenGlassBox repository root:

```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
cd OpenGlassBox/
make download-external-libs
make -j8
sudo make install   # optional
```

After `make`, the build folder contains `libOpenGlassBox.a` and, on most platforms, `libOpenGlassBox.so`. Headers live in `include/OpenGlassBox/`.

After `make install`, pkg-config can find the library:

```sh
pkg-config --cflags --libs OpenGlassBox
```

## Link your project

### With pkg-config (recommended after install)

```makefile
CXXFLAGS += $(shell pkg-config --cflags OpenGlassBox)
LDFLAGS  += $(shell pkg-config --libs OpenGlassBox)
```

### Against a local build tree

Point your compiler at the OpenGlassBox tree without installing:

```makefile
OPENGLASSBOX := ../OpenGlassBox

CXXFLAGS += -I$(OPENGLASSBOX)/include
LDFLAGS  += -L$(OPENGLASSBOX)/build -lOpenGlassBox
```

### Working example

The [LinkAgainstMyLibs](https://github.com/Lecrapouille/LinkAgainstMyLibs) repository contains a minimal consumer project:

```sh
git clone https://github.com/Lecrapouille/LinkAgainstMyLibs.git --recursive
cd LinkAgainstMyLibs/OpenGlassBox
make -j8
./build/OpenGlassBox
```

## Minimal program

Every host follows the same pattern:

1. create a `Simulation`;
2. parse a `.ogs` ruleset into `simulation.script()`;
3. add at least one `City`;
4. attach a router to each city (required before agents can travel);
5. call `simulation.update()` from your game loop.

```cpp
#include "OpenGlassBox/Simulation.hpp"

int main()
{
    ogb::Simulation simulation(64u, 64u);

    if (!simulation.script().parse("my_rules.ogs"))
        return 1; // see simulation.script().formatErrors()

    ogb::City& city = simulation.addCity("MyCity", ogb::Vector3f(0.f, 0.f, 0.f));

    // A router is mandatory for agent routing. The default Dijkstra
    // implementation lives in demo/src/Routing/ (see below).
    // installDijkstraRouter(city, simulation.config());

    while (running)
    {
        simulation.update(secondsSinceLastFrame);
        // read city.units(), city.agents(), maps, etc., and render
    }
}
```

`Simulation::update()` converts wall time into fixed simulation ticks. A slow frame rate makes the simulation fall behind; it does not change the rules.

## Loading a ruleset

Rulesets are plain-text `.ogs` files. Parse them through `Script`:

```cpp
if (!simulation.script().parse("demo/data/Simulations/test_city.ogs"))
{
    std::cerr << simulation.script().formatErrors();
    return 1;
}
```

You can also load from a string with `script().parseString(source, "my_mod.ogs")`. On failure, the previous definitions are kept so a running city is not left empty.

The ruleset must **outlive every city** loaded from it: buildings hold references to their recipes (`UnitType`, `WayType`, …). `Simulation` stores its `Script` before its `World` so destruction order remains safe.

## Creating and updating cities

```cpp
ogb::City& city = simulation.addCity("Paris", ogb::Vector3f(0.f, 0.f, 0.f));
city.update(); // one tick at the configured duration
```

Use `simulation.world()` for shared map layers and `simulation.clock()` for the in-game calendar. Rules can depend on time (`hour between 8 18`) and on periods expressed in game minutes (`rate 30 minutes`).

## Routing

Agents need an `IRouter` on their city. The engine defines the interface in `OpenGlassBox/Router.hpp`; the shipped **Dijkstra** implementation is in `demo/src/Routing/DijkstraRouter.hpp`, with helpers in `demo/src/Routing/installRouter.hpp`:

```cpp
#include "Routing/installRouter.hpp"

installDijkstraRouter(city, simulation.config());
// or, for every city already in the simulation:
installDijkstraRouters(simulation);
```

You can provide your own `IRouter` and call `city.setRouter(std::move(router))` instead. See [engine documentation](engine.md#routing-irouter-and-dijkstra) for how travel times and destination search work.

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
- [Engine documentation](engine.md): classes, tick order, traffic model.
- [Demo documentation](demo.md): interactive editor and panels.
