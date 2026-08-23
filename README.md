# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an implementation of the GlassBox simulation engine from Maxis's SimCity (2013), based on the [2012 GDC talk slides](http://www.andrewwillmott.com/talks/inside-glassbox). This project is neither Maxis's official source code nor affiliated with Maxis. It is a C++14 port of the well-written [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation) project — originally written in C# for the Unity game engine more than 8 years ago.

This project builds:

- static and shared libraries for the simulation engine;
- a standalone demo application rendered with GLFW, OpenGL 3.3 and [Dear ImGui](https://github.com/ocornut/imgui) (docking branch), with [ImPlot](https://github.com/epezent/implot) charts and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog).

The simulation engine and the demo renderer are kept separate, because I was more interested in the simulation logic than in the rendering. GLFW and Dear ImGui were chosen for the demo simply because they were the quickest way to see the simulation run, so feel free to plug in your own rendering engine instead. I am also looking for a game developer or an artist able to turn this into a more interesting game.

## Screenshot of the standalone demo application

This screenshot may not match the current state of the code, and what you see also depends on the loaded ruleset. Click on the image to watch a video of the simulation running.

[OpenGlassBox](https://youtu.be/zyLO9Ls_hME?feature=shared)

In this screenshot:

- In pink: houses (static).
- In cyan: factories (static).
- In yellow: people traveling from houses to factories (dynamic).
- In white: people traveling from factories to houses (dynamic).
- In grey: nodes (crossroads) and ways (roads) (static).
- In blue: water produced by factories (dynamic).
- In green: grass consuming water (dynamic).
- Grid: the city holding maps (grass, water), paths (ways, nodes), and units (agents moving along paths and carrying resources from one unit to another).



## Prerequisites

- **Operating systems**: Linux, macOS. Should compile on Windows as well.
- **Build tools**: C++14 compiler (`g++`or`clang++`), GNU Make, Git. C++14 is required for `std::make_unique`; otherwise, the code is largely C++11-compatible.
- **Renderer libraries**: GLFW 3, GLEW and OpenGL 3.3 (must be installed on your system; only needed for the demo — see below).
- **GUI libraries**: [Dear ImGui](https://github.com/ocornut/imgui) (docking), [ImPlot](https://github.com/epezent/implot) and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog) — automatically downloaded and built by the Makefile (not installed system-wide; only needed for the demo).
- **Debug library** (debug builds only): [backward-cpp](https://github.com/bombela/backward-cpp) — automatically downloaded and built by the Makefile (not installed system-wide).
- **Unit tests** (optional, for developers): [Google Test](https://github.com/google/googletest) (must be downloaded, built, and installed manually), plus coverage tools (see below).
- **Makefile helper** [MyMakefile](https://github.com/Lecrapouille/MyMakefile): automatically fetched when cloning with `--recursive`.



### Installing system packages

**Debian / Ubuntu**

```sh
# Required to build the demo
sudo apt-get install build-essential git pkg-config libglfw3-dev libglew-dev

# Optional: debug builds (backward-cpp) and code coverage
sudo apt-get install libdw-dev lcov
```

**Fedora**

```sh
# Required to build the demo
sudo dnf install gcc-c++ make git pkgconf-pkg-config glfw-devel glew-devel

# Optional: debug builds (backward-cpp) and code coverage
sudo dnf install elfutils-devel lcov cmake
```

On Fedora, `cmake` is only needed if you build and install Google Test from source (see the CI workflow for an example).

## Download, compile, and run

Clone the repository recursively:

```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
```

Build the project (libraries + demo):

```sh
cd OpenGlassBox/
make download-external-libs
make -j8
```

Adjust `-j8` to the number of cores of your machine, and pick a compiler with `make CXX=clang++ -j8` if you prefer.

Builds are optimised with debug symbols by default (`COMPILATION_MODE := normal` in `Makefile.common`). Use `make COMPILATION_MODE=debug -j8` to step through the code, and `make COMPILATION_MODE=release -j8` to ship. The mode matters: a map rule runs over every cell of a city, so the Chicago save with its three hundred thousand cells is an order of magnitude slower to simulate when compiled without optimisations.

Run the demo (from the project root, after `make`):

```sh
./build/OpenGlassBox-demo
./build/OpenGlassBox-demo braess.ogc
```

The simulation starts paused: press Play on the map toolbar or the space bar to start it. The Simulation clock panel steps it tick by tick while paused and changes the speed from x0.25 to x16. One window shows one city. Opening a ruleset (`.ogs`) starts from an empty city, while a city save (`.ogc`) holds the geometry, the live state and a hash of the ruleset it was made with.

The bundled simulations are:

- `test_city.ogs` and `test_city.ogc`, loaded by default: Home, Work and Shop buildings, residential and commercial zones, and BPR traffic. This is the one to read first.
- `braess.ogs` and `braess.ogc`: the four-node network of the Braess paradox, where adding a road makes everybody slower. Watch the relative gap and the total travel time in the Traffic panel.
- `regular.ogs` and `regular.ogc`: a `Regular(6,6)` grid of bidirectional roads, in the style of the CiudadSim examples.
- `chicago.ogs` and `chicago.ogc`: a simplified set of arteries. This is not the 546-node Scilab network.

The `.ogs` language is specified in `[demo/data/Simulations/README.md](demo/data/Simulations/README.md)`.

(Optional) Run unit tests with code coverage:

```sh
cd OpenGlassBox/tests
make coverage -j8
```

(Optional) Install OpenGlassBox on your system:

```sh
sudo make install
```

Example: link against OpenGlassBox from another project (after installation):

```sh
git clone https://github.com/Lecrapouille/LinkAgainstMyLibs.git --recursive
cd LinkAgainstMyLibs/OpenGlassBox
make -j8
./build/OpenGlassBox
```

On macOS, a bundle application is also created inside the build folder.

## How this differs from the original project

Three things are easily confused, so it is worth naming them apart. **GlassBox** is the engine Maxis described in a 2012 GDC talk; there is no source code for it. **MultiAgentSimulation** is Federico D'Angelo's C# implementation of that talk for Unity, and it is the code this project was ported from. **OpenGlassBox** is what you are reading about, and this section is what it does differently.

### The language and the shape of the project

- The original is C# for Unity. This is C++14, with no game engine underneath.
- The simulation is a library that knows nothing about drawing, and the demo is a separate program on top of it, so you can plug in your own renderer. The Unity decorator classes were not ported. The demo uses GLFW, OpenGL 3.3 and Dear ImGui rather than a full engine. There is no SDL.
- The original reused the names of the GDC talk. The ones I found confusing were renamed: `Box` became `City`, `Point` and `Segment` became `Node` and `Way` (the vocabulary of graph theory), `ResourceBinCollection` became `Resources`, and `SimulationDefinitionLoader` became `ScriptParser`.
- The original had neither unit tests nor comments. This has both: every header carries its documentation and a small example, and the test suite covers the engine down to individual rule commands.

### What the simulation models

- **Zones exist.** The original declared the `Area` class, also called `Zone`, and never implemented it. Here `Area` and `AreaRule` are real, with `spawn`, `upgrade`, `destroy` and `count` commands. A zone is a rectangle of cells, and a building is not a graph node, so the two stay independent.
- **A building is no longer a crossroads.** A `Unit` used to be forced onto a `Node` of a `Path`, which made the graph explode. It now has its own position plus an optional anchor, either on a `Node` or at an offset along a `Way`. That anchor is not decoration: `spawn ... at nearestWay` only grows a building on a cell a road runs through or fronts, because a building nobody can drive to could neither send nor receive an `Agent`. An `Agent` leaving a building anchored along a way drives out by the end its destination lies behind, which is not always the near one, and it only knocks at a door it has actually driven to.
- **Routing is pluggable and traffic-aware.** The original walked the graph with a dynamic A\* inside `Path::FindNextPoint`. Routing now goes through the `IRouter` interface, and the shipped implementation minimises BPR travel times rather than distances. The flow of each `Way` is an exponential moving average, **not** the MSA solver of CiudadSim; the traffic section below explains why that distinction matters.
- **There is a clock.** Rules can be given a period in game minutes rather than in ticks, and can be restricted to a range of hours, which is what lets a shop keep opening hours and a factory work a shift. The inspector reads those hours back and tells you whether a building is open.
- **The rules can be watched.** A rule reports every attempt, successful or not, along with the command that refused, which is what the Rule Log panel of the demo shows. Debugging a ruleset without that amounts to guessing.

### Files and tooling

- There are two file formats, not three: `.ogs` for a ruleset and `.ogc` for a save. There is no separate "world" file. A save carries a fingerprint of the ruleset it was written against and the list of the types it uses, so opening it against a ruleset that has drifted tells you so instead of failing obscurely later.
- The script parser sits behind `IScriptParser`, so another front end can be plugged in without touching the engine.
- One window shows one city, but a `World` can hold several. A road crossing a border is clipped against the region of each city, and each piece belongs to the city it was clipped to, once `World::Listener` has allowed it. Routing stays inside one `Path`, so an agent never wanders into the neighbouring city on its own.
- The demo has undo and redo, a script editor that reparses in place, and an inspector that shows what a building holds and which rules act on it.

### Speed

The original was written to run a few hundred cells in Unity. This one is meant to stay smooth on a real street network — the Chicago sample is 546 crossroads, 2176 segments and a region of 487 by 641 cells, which is a third of a million cells swept by every rule of every layer. Getting there took the following, and they are worth knowing about if you intend to grow the simulation further:

- **The grid is sparse and addressed by shifts.** Cells live in blocks of 16 by 16 allocated on first write, so an empty world costs nothing and a city can be founded anywhere. Finding the block a cell belongs to is a shift and a mask rather than a signed division, and the last block found is remembered, which answers fifteen lookups in sixteen without touching the hash table.
- **A rule sweeps its cells row by row**, which is the order a block stores them in, so sixteen consecutive cells are one cache line instead of sixteen.
- **The region a city administers is derived once**, when it is founded or moved. Deriving it per cell, which is what a rule reading a layer used to do three times over, meant two floating point floors on the hottest path in the library.
- **A rule that visits only a share of its cells draws them by selection sampling**: the region is scanned once in order and each cell is taken with the probability that makes the sample come out right. Every subset of the size asked for is still equally likely, but nothing is shuffled and nothing is allocated. The previous approach held two vectors as large as the map, copied one into the other on every run, and jumped randomly through both.
- **Names are interned.** Everything a script names — resources, types of building, what an agent is looking for — is a `Name`, which is an index into a table fixed when the ruleset is read. Asking a building whether it accepts what an agent carries compares integers rather than strings.
- **Travel times are cached** on each segment and recomputed only when its smoothed traffic actually moves, which spares the BPR power on every quiet street of the network on every tick. Agents re-examine their route on a period rather than on every tick, and only switch when the cost has drifted enough to be worth it.
- **The router works in flat arrays.** Dijkstra's scratch space is indexed by a dense node index kept by the `Path`, with a generation counter instead of clearing, rather than the `std::map` and `std::unordered_set` it started with.
- **A layer is drawn at the granularity the zoom deserves.** Squares of cells are aggregated so that neither their size on screen nor their number gets out of hand, which is what keeps a half million cell city from asking Dear ImGui for a hundred and sixty thousand rectangles per layer per frame.

Together these took the Chicago sample from 2.25 ms to 0.95 ms of simulation per tick, and cut the rectangles the renderer emits at the worst zoom by a factor of sixteen.



## Using the demo application

The map holds a SimCity-style vertical rail on its left. From top to bottom: **Play / Pause**, then the six tools — **Inspect**, **Roads**, **Zones**, **Buildings**, **Maps**, **Demolish** — then **Undo** and **Redo**. Keys `1` to `6` select a tool and the space bar toggles the pause.

The row to the right of the rail holds the settings of the selected tool, and only those:

- **Inspect** highlights whatever is under the cursor — a building, an agent, a road, a node, a zone, or the grid cell when there is nothing else — and a click sends it to the Inspector panel. A building standing on a junction hides its node, since that is what you pointed at. This is the only tool that highlights cells; the others show the footprint of what they are about to do instead.
- **Roads** drags a segment of the chosen way type. Both ends snap to a nearby node, and otherwise to the world grid.
- **Zones** and **Maps** paint a rectangle. Click for a single square of the brush size, or drag for a rectangle. Zones do not overlap: painting Commercial over part of a Residential rectangle re-zones exactly the cells you painted and leaves the rest residential.
- **Maps** also carries the layer list, since choosing which map to paint and which map to look at is the same decision. Each row toggles the visibility of a map, sets its opacity, and picks how it is drawn: filled cells, contours, or numeric values. Clicking a name makes it the main layer; Alt+clicking shows it alone. A simulation opens with one map shown, because half a dozen heatmaps stacked on the same cells cannot be read.
- **Buildings** drops a building on a road. The segment is cut in two and the building sits on the junction, so agents have somewhere to stop; clicking an existing node builds there instead. Buildings grown by a zone are anchored at an offset along a way and cut nothing, which is what keeps a street of forty houses a single segment.
- **Demolish** removes a building, a road or an orphan node, and holds **Clear city**, which throws away everything that was built but keeps the ruleset, so roads can be laid again straight away. That one cannot be undone; everything else can, with Ctrl+Z and Ctrl+Y.

**Recenter** and the **Zoom** slider sit on the row below, next to the display toggles. You can also pan by dragging with the middle or right button, zoom with the wheel, and frame the whole city with the Home key.

The **Simulation clock** panel holds the speed buttons, from x0.25 to x32, and the time of day. Two fields and **Set time** move the calendar where you want it, which matters because a rule that keeps office hours does nothing outside them: a save written at half past midnight looks dead until eight in the morning. While paused, **Step** runs a number of ticks one after the other, so a rule with a period of three ticks and one with a period of seven each land on their own tick; **1 min** and **1 h** queue the ticks of a game minute and of a game hour.

Files are handled through File → New city, Open ruleset (`.ogs`), Open city and Save city (`.ogc`). The Script panel edits the open `.ogs` and Apply reparses it, keeping the city as long as every type it still uses is defined by the new ruleset. Its **Checksum** section reads the fingerprint of the ruleset, of the text being edited and of the open save, and re-stamps the save with the current one. While a ruleset is being written that fingerprint changes at every save, so File → **Open saves with a stale checksum** waives the check; the types a save names are still required to exist. Simulation clock, Inspector, Rule Log, Charts, Traffic, History and Script are dockable panels. The canvas shows `Jour N  HH:MM` in its top left corner and tints the background through night, dawn, day and dusk; the charts use game hours on the X axis.

## The engine, class by class

Every class of the engine lives in `include/OpenGlassBox` and is documented there. This section is the map you need before reading any of them: what each one is for, and why it is a class of its own rather than a field of another.

### The two halves: recipes and things

Nothing in the engine is hard-coded, and that split runs through the whole design. A ruleset declares **recipes** — a kind of house, a kind of road, a kind of traveller — and a running city holds **things** built from them. `Types.hpp` holds the recipes (`UnitType`, `WayType`, `AgentType`, `MapType`, `AreaType`, `PathType`); `Unit`, `Way`, `Agent`, `Map`, `Area` and `Path` are the things.

A thing keeps a `const&` to its recipe rather than a copy of it. A thousand houses share one `UnitType`, so a house costs what it actually holds and nothing more. The price of that is a lifetime rule which is worth remembering, because breaking it is the one way to make the engine misbehave in a way that is hard to debug: **the ruleset has to outlive every city loaded from it**. `Simulation` declares its `Script` before its `World` for exactly that reason, so that destruction, which runs in reverse, takes the cities away first.

### The containers: Simulation, World, City


| Class        | What it is                                                        | What it owns                     |
| ------------ | ----------------------------------------------------------------- | -------------------------------- |
| `Simulation` | The whole game, and where a program starts                        | the ruleset and the world        |
| `World`      | The ground: one grid, one calendar, the layers of the environment | the layers, the clock, the towns |
| `City`       | One town: its roads, buildings, travellers and zones              | everything a town is made of     |


`Simulation` also owns the conversion from wall time to game time. You hand `update()` the seconds elapsed since the last frame; it scales them by the speed the player chose, accumulates them, and runs as many **fixed ticks** as fit. A slow machine therefore falls behind rather than simulating differently, and a save made on one machine replays on another.

`World` exists because two towns that touch have to share their environment: pollution does not stop at a boundary. The layers live there, not in the towns, and what a `City` holds on the grid is the rectangle of cells it administers — a `MapRegion`. That region is what bounds every rule run on its behalf, so a town never reads over its neighbour's shoulder.

`World` also owns the **order of a tick**, which is not arbitrary: the towns move their agents and run their buildings first, then the layers run their own rules. Agents move before buildings look around so that a building sees who has just arrived, and the layers come last so that a cell reflects everything that happened during the tick.

### The road network: Path, Node, Way

A `Path` is a graph, and the only thing an `Agent` may travel on. `Node` is a vertex — a crossroads, and an address a building may sit on. `Way` is an edge — a street, undirected, since one-way traffic is not modelled.

A city may hold several `Path` objects and they never meet: a road network and a rail network are two of them, and the router never leaves the one it started on. That is the whole of the separation, and it is enforced in one place rather than checked everywhere.

A `Way` knows its length, how many agents are on it, and from those two how long it takes to drive. That travel time, not the length, is what the router minimises — see the traffic section below.

Both containers are `std::deque` rather than `std::vector`, and that is deliberate: everything refers to a crossroads or a street **by address**, so adding one must not move the others.

### The actors: Unit, Agent, Area

`Unit` is a building. It holds resources, it runs its rules once every so many ticks, and those rules send agents out. A `Unit` is **not** a graph node — a distinction worth dwelling on, because it is the one place this port departs most from the original. A building has its own position and, separately, an *anchor* on the network: a `Node`, or a `Way` at an offset. Anchoring along a street is what keeps the graph small; a street of forty houses used to become forty crossroads and forty-one segments, and the router paid for all of them at every tick.

`Agent` is one trip: a load of resources leaving a building and looking for another that will take it. Agents run no rules — a city may have a thousand alive at once — and they do not know where they are going. The router finds the cheapest building that answers to the name they are looking for *and has room*, and the answer changes as the traffic does. An agent that finds nothing wanders, and after `agentGiveUpTicks` it hands its load back to the building that sent it out. Wandering means the city is short of somewhere to deliver, not that the router is broken.

`Area` is a zone the player painted, and the piece that turns an empty grid into a town. It owns its footprint and nothing else: the buildings it grew belong to the `City`, which is why it counts them by walking the city rather than keeping a list of pointers that would outlive what they point at.

`Unit` and `Agent` share a base, `Entity<TYPE>`, holding the four things both have — an identifier, a recipe, a colour and a position. `Node` and `Way` deliberately do not inherit from it: they have no colour of their own and no identity outside the `Path` numbering them.

### The environment: Map

A `Map` is one layer: coal, water, forest, but also pollution, land value or desirability. Every cell holds an amount of that one thing, capped by the recipe of the layer, so nothing piles up without bound in one place.

A `Map` is what lets a rule ask a question about a *place* rather than about a building: "is there water within three cells of here?" A building reads and writes its neighbourhood through its **footprint**, a radius around the cell it stands on, and that radius is a taxicab distance — a diamond, not a circle. `MapCoordinatesInsideRadius` walks that diamond, reusing its buffers because a layer runs this for every cell of every town on every tick. `MapRandomCoordinates` is the other walker: it hands out a share of the cells of a region, drawn so that every subset of that size is equally likely, but scanned in reading order so that the cells come out in the order the grid stores them.

The grid is unbounded in the four directions, so cell coordinates are signed, and storage is sparse: blocks of 16×16 cells allocated the first time something is written into them. An empty layer costs nothing and a town can be founded anywhere without deciding on a size beforehand. `allocatedChunks()` makes the cost visible in the debug panel.

### The rules: Rule, RuleCommand, RuleValue

This is the scripting language at runtime, and it is smaller than you would expect.

- `IRule` is a name, a period, and a list of commands. `RuleUnit`, `RuleMap` and `RuleArea` add what each kind needs: a fallback for a building, a random sample of the cells for a layer, nothing for a zone.
- `IRuleCommand` is one line of a rule. Every command is asked **twice**: `validate()`, then `execute()`. A rule asks all of its commands first and applies them only if every one agreed, which is what makes a rule atomic — "take a person out of the house and put them in a car" either does both or does neither, so nobody is ever lost between the two.
- `IRuleValue` is somewhere a command reads a number from and writes one back to: `local`, `global` or `map`. Having them behind one interface is what lets a single `add` command serve all three — the command knows *how much*, the value knows *where*.
- `RuleContext` is everything a rule may look at while it runs: the town, the building or the cell, the clock, the resources. Rules hold no state of their own — they are shared by every entity that lists them — so all of the "where" has to be handed to them. The context is held by the entity and reused from tick to tick, since a layer rule fires over thousands of cells.

`IRule::Listener` is a single global observer of every attempt to run a rule, reporting **which command refused**. That is the answer to "why does this rule, which looks right, never do anything?", and it is what the Rule Log panel shows. While nobody is listening the whole feature costs one null pointer test.

### Time: SimulationClock

A tick means nothing on its own. `SimulationClock` turns a tick count into an hour of a day, so a rule can say `hour between 8 18` instead of counting ticks, and a building can be open or shut. The conversion is a runtime setting, `ticksPerMinute`, which is why a rule written as `rate 30 minutes` rescales with the settings instead of being left behind.

`OpeningHours` reads the `hour between` conditions of the rules of a building and aggregates them into a timetable. That is what the inspector and the map tooltip show as *open* or *closed*, and it is derived rather than declared: a building has opening hours because its rules keep them, and one whose rules keep none is open around the clock.

### Routing: IRouter and Dijkstra

`IRouter` is the interface the agents ask for an itinerary; `Dijkstra` is the implementation, and a `City` owns one so that its scratch buffers are allocated once instead of on every search. Two things make it more than a textbook shortest path, both covered in the traffic section: the cost of a street is its travel time under the current traffic, and there is no goal to aim at.

### Resources

`Resource` is a named stock with an amount and a capacity; `Resources` is a bag of them. Two resources are the same resource when their names match, which is what lets a script declare a new one without touching the engine. Lookup is a linear scan over a small vector, which beats a hash map at the handful of resources a building actually holds.

### Saving: CitySave

The `.ogc` format holds the state of a game: roads, buildings and what they hold, agents and where they are, the layers, the clock, and the traffic averages of the streets — the last so that a loaded town does not start with every road looking empty. What it does not hold is the rules, which stay in the `.ogs` next to it.

That split is why every entry point in `CitySave` deals with the ruleset too. A save stores a **fingerprint** of the ruleset it was written against and the names of every type it uses. Loading it into a different ruleset would rebuild the same geometry out of different recipes, giving a town that looks right and behaves like something else, so the loader refuses rather than warns. `peekHeader()` reads the header alone, which is how the demo loads the right ruleset without asking.

## The ruleset language

A ruleset is a `.ogs` text file, and it is where the game lives. The full reference is in `[demo/data/Simulations/README.md](demo/data/Simulations/README.md)`; what follows is enough to read one.

The file is a handful of sections, each closed by `end`, and inside them one declaration per line. Whitespace does not matter, `#` runs to the end of the line, and the order of the sections does not matter either: the parser walks the file twice, declaring the names on the first pass and filling them in on the second, so a building may list a rule written further down.

```text
resources                    # what everything else is counted in
    resource People
    resource Goods
    resource Money
end

paths                        # kinds of network
    path Road color 0xAAAAAA
end

segments                     # kinds of street, with their traffic model
    segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
end

agents                       # kinds of traveller
    agent Worker color 0xFFFFFF speed 10
end

rules                        # the behaviour of everything
    unitRule SendPeopleToWork
        rate 45 minutes
        hour between 8 18
        local People greater 0
        local People remove 1
        agent Worker to Work add [ People 1 ]
    end
end

units                        # kinds of building
    unit Home color 0xFF00FF mapRadius 1 rules [ SendPeopleToWork ]
         targets [ Home ] caps [ People 8 ] resources [ People 8 ]
end

maps                         # layers of the environment
    map Water color 0x0000FF capacity 100 rules [ ]
end

areas                        # kinds of zone the player paints
    area Residential color 0x44AA44 rules [ GrowHomes ]
end
```

Three kinds of rule exist, and which one you write says what runs it:

- `unitRule` is run by a building, once every period. It may name another rule with `onFail`, which is how a script says "go to work, or else stay at home".
- `mapRule` is run by a layer, over the cells of a town. `randomTilesPercent` makes it visit a share of them, which reaches the same behaviour on average at a fraction of the cost.
- `areaRule` is run by a zone, once for the whole zone. Its commands `count`, `spawn`, `upgrade` and `destroy` deal in buildings rather than resources.

The body of a rule is a list of commands, and every one of them is either a **test** or an **action**. The distinction is not in the syntax but in what the command does when asked to validate, and it matters because a rule fires only if every one of its commands agrees:


| Command                                 | Kind   | What it does                                               |
| --------------------------------------- | ------ | ---------------------------------------------------------- |
| `local People greater 0`                | test   | reads the resources of the building or the cell            |
| `global Money greater 0`                | test   | reads the resources of the town                            |
| `map Water greater 300`                 | test   | reads the layer over the footprint                         |
| `hour between 8 18`                     | test   | reads the clock; wraps around midnight, so `22 6` is night |
| `count Home less 10`                    | test   | counts the buildings inside the zone                       |
| `local Goods add 1`                     | action | writes the resources of the building or the cell           |
| `map Pollution add 1`                   | action | writes the layer over the footprint                        |
| `agent Worker to Work add [ People 1 ]` | action | sends a traveller out                                      |
| `spawn Home at nearestWay`              | action | grows a building beside a road, or `at freeCell` anywhere  |
| `upgrade Home to Tower`                 | action | replaces a building by another                             |
| `destroy Home`                          | action | demolishes one                                             |


Two things about `rate` are worth knowing, because they are the usual source of a ruleset that does nothing. It may be written in ticks, `rate 7`, or in game time, `rate 45 minutes`, `rate 2 hours`, `rate 1 day` — the second form is the readable one and the one that survives a change of settings. And a period is counted on the clock of the entity running the rule, starting from a random phase, so that the whole city does not leave home on the same tick.

## How traffic is modelled



### Travel time on a road: the BPR function

Agents do not all take the shortest road: they take the *fastest* one, and a road gets slower as it fills up. The standard way to express that is the **BPR function**, named after the US **B**ureau of **P**ublic **R**oads that published it in 1964. It is the textbook formula of traffic assignment: the travel time of a road grows with the ratio of the traffic it carries to the traffic it was built for.

`Way::travelTime` implements it:

$$\displaystyle t(f) = t_0 \left(1 + 0.15 \left(\frac{f}{c}\right)^{\beta}\right)$$

- $t_0$ is the free flow travel time, the length of the road divided by its speed limit;
- $f$ is the flow, that is how many agents are on the road;
- $c$ is the capacity, the flow above which the road starts to jam;
- $\beta$ says how brutally it degrades past that point: with $\beta = 4$, twice the capacity costs about three and a half times the free flow time.

An empty road costs $t_0$. A road loaded exactly to its capacity costs 15 % more. `speed`, `capacity` and `beta` are properties of a `WayType`, so they are set in the `.ogs` ruleset.

A word on the units, because they are not the ones a traffic engineer would expect. Here $f$ is a **number of agents currently on the road**, not a number of vehicles per hour, and $c$ is the number of agents a street carries before it starts to feel it. Nothing forbids $f$ from exceeding $c$: a saturated street stays passable, it just becomes expensive, and that is precisely what makes the router send the next agent somewhere else instead of queueing everybody through the same place. Times are in seconds of game time, which the clock converts from ticks, so lengthening a tick does not make the city drive faster.

### Finding a destination: a search with no goal

The router is where those travel times are actually used, and it does two things a textbook shortest path does not.

The first is the cost: an edge costs its travel time under the current traffic, not its length. A short road carrying two hundred agents costs more than a long empty one.

The second is stranger, and it is the reason `Dijkstra` is not an A. **There is no goal.** An agent does not know which building it is going to; it knows the *name* of what it is looking for. The search walks the network outwards from the crossroads the agent stands at and stops at the first building that answers to that name **and has room for the load**. A building standing along a street is kept as a candidate rather than accepted at once, because a crossroads one hop away may hold a cheaper one.

Having no goal also means there is nothing for an A estimate to aim at. What is added to the cost of a crossroads is the free flow travel time back to the one the search started from, which biases the order of exploration towards the neighbourhood of the departure, where the nearest destination usually is. It is a speed-up, not an admissible heuristic: the answer is the cheapest building the search met first, which in an unusual geometry may not be the cheapest one there is. The `AStarRouter` alias is a leftover of the days when the search did have a goal.

Two consequences follow, and both are visible in the demo. Two agents leaving the same door a minute apart may be sent to different shops, because the traffic changed in between. And an agent whose itinerary was computed a while ago replaces it when the road it is on has become worse than the alternative.

Comparing the two costs a whole graph search, so it does not happen on every tick: `pathCheckTicks` in the Traffic panel is how often an agent bothers to look, and `cost deviation` is how much worse its road has to be before it switches. When the comparison does say the alternative is better, the itinerary that search produced is the one the agent takes — it is not searched for a second time. Lowering `pathCheckTicks` to one is the quickest way to bring a large city to its knees.

### Why the flow is smoothed

If routing used the instantaneous number of agents on each road, the whole population would swap between two parallel roads every tick: everyone sees road A empty, everyone moves to A, A is now jammed, everyone moves back to B. OpenGlassBox avoids that by feeding the BPR function an exponential moving average of the count $n$ of agents on the road, with a fixed weight $\alpha$ (0.05 by default, adjustable in the Traffic panel):

$$\displaystyle f \leftarrow (1-\alpha) f + \alpha n$$

This damps the oscillation, and that is all it does. It is deliberately **not** the solver used by traffic engineering tools such as CiudadSim, whose **MSA** (Method of Successive Averages) computes an all-or-nothing assignment $y^k$ onto the shortest paths at iteration $k$ and averages it in with a decreasing weight:

$$\displaystyle f^{k+1} = (1-\lambda_k) f^k + \lambda_k y^k,\qquad \lambda_k = \frac{1}{k}$$

Because $\lambda_k$ shrinks, MSA converges to a Wardrop equilibrium, where no agent can find a cheaper route. A fixed $\alpha$ does not converge to anything: it just keeps the picture of the network stable enough for the agents to make sensible decisions. This is a game, not an assignment solver.

### Reading the Traffic panel

The **relative gap** compares what the agents actually pay to what they would pay on the cheapest routes available at the current travel times:

$$\displaystyle \mathrm{Relgap} = \frac{\mathrm{TSTT} - \mathrm{SPTT}}{\mathrm{TSTT}}$$

TSTT is the total system travel time and SPTT the shortest path travel time. Near zero, the agents are already on their cheapest itineraries and the network has settled. Here it is a diagnostic you watch, not a stopping criterion of a solver.

Routing itself sits behind `IRouter` (`findRoute`, `shortestPathCost`), which a `City` owns, so another pathfinder can be dropped in without touching `Agent`.

## For authors who want to build their own city builder

The point of GlassBox (Willmott, GDC 2012) is that a city simulation can be **data** rather than a tree of objects with an `Update()` method. Everything is one of four things:

- **Maps** are 2D fields over the grid: water, pollution, desirability.
- **Units** are buildings holding bounded stocks of resources.
- **Agents** are mobiles that carry resources from one Unit to another.
- **Paths** are the network the Agents travel on.

Rules move resources between the four, and they are the gameplay. OpenGlassBox adds a fifth: **Areas**, the RCI zones that were in the talk but missing from MultiAgentSimulation. An Area is a rectangle of cells whose rules spawn, upgrade and demolish buildings. The class-by-class section above says how each of the five is put together.

The consequence for you is that the demo is only a **host**. The gameplay lives in the `.ogs` ruleset, and you should be able to build a different game without writing C++. What is in place today:

- the `.ogs` rule language, documented in `[demo/data/Simulations/README.md](demo/data/Simulations/README.md)`;
- the `.ogc` save, which stores a fingerprint of the ruleset it was made with and refuses to load against a different one;
- `IRouter`, to plug in another pathfinder;
- `World::Listener` (`allowWayAcross`, `allowWayRemoved`), so a city can refuse a road coming from its neighbour;
- `test_city.ogs`, a commented ruleset meant to be read as a tutorial.

What is missing, and where contributions would help most:

- registering your own `IRuleCommand` from C++ without forking the parser;
- a stable query API for the UI (map totals, agents by type, relative gap, budget);
- service networks as `Path` of their own for water and power, or the convention that one Map is one coverage area;
- queues on a `Way`: in the original GlassBox the agent *is* the traffic, and nothing here models a car waiting behind another;
- staged construction: `upgrade` exists, but there is no build timer and no cost in Money;
- a deterministic seed plus a replay, to debug a ruleset by reproducing a run.



## Future ideas

- Import [OpenStreetMap](https://www.openstreetmap.org) maps.
- Implement ideas from [Exploring SimCity: A Conscious Process of Discovery](https://youtu.be/eZfj7LEFT98).
- Parallelize the simulation: dispatch work across CPU cores (e.g. with OpenMP) or distribute it over the network (peer-to-peer). `World::Listener` is the hook; real IPC is out of scope here.
- Attach `Agent` objects directly to `Way` segments; for cars, track the distance to the next `Agent`.
- Display and edit a SimCity-like city as a spreadsheet: insert and edit cells to define simulation rules. This project could be merged with [SimTaDyn](https://github.com/Lecrapouille/SimTaDyn).



## References

- GDC talk slides: [http://www.andrewwillmott.com/talks/inside-glassbox](http://www.andrewwillmott.com/talks/inside-glassbox)
- Since the original conference video is no longer available, an alternative GDC talk can be found here: [https://youtu.be/eZfj7LEFT98](https://youtu.be/eZfj7LEFT98)
- A Scilab traffic assignment toolbox: [https://www.rocq.inria.fr/metalau/ciudadsim](https://www.rocq.inria.fr/metalau/ciudadsim) and [https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf](https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf). For more information, see other PDFs at [https://jpquadrat.github.io/](https://jpquadrat.github.io/) in the section *Modélisation du Trafic Routier*.
- A tutorial for building a city builder (focused on rendering with SFML): [https://www.binpress.com/creating-city-building-game-with-sfml/](https://www.binpress.com/creating-city-building-game-with-sfml/)
- Moving cars: [http://lo-th.github.io/root/traffic/](http://lo-th.github.io/root/traffic/) (source code: [https://github.com/lo-th/root/tree/gh-pages/traffic](https://github.com/lo-th/root/tree/gh-pages/traffic), a fork of [https://github.com/volkhin/RoadTrafficSimulator](https://github.com/volkhin/RoadTrafficSimulator))
- A work-in-progress, open-source, multiplayer city simulation game: [https://github.com/citybound/citybound](https://github.com/citybound/citybound)
- An open-source version of Transport Tycoon: [https://github.com/OpenTTD/OpenTTD](https://github.com/OpenTTD/OpenTTD)

