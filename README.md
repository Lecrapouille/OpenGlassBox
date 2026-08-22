# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an implementation of the GlassBox simulation engine from Maxis's SimCity (2013), based on the [2012 GDC talk slides](http://www.andrewwillmott.com/talks/inside-glassbox). This project is neither Maxis's official source code nor affiliated with Maxis. It is a C++14 port of the well-written [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation) project — originally written in C# for the Unity game engine more than 8 years ago.

This project builds:

- static and shared libraries for the simulation engine;
- a standalone demo application rendered with GLFW, OpenGL 3.3 and [Dear ImGui](https://github.com/ocornut/imgui) (docking branch), with [ImPlot](https://github.com/epezent/implot) charts and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog).

The simulation engine and the demo renderer are kept separate, because I was more interested in the simulation logic than in the rendering. GLFW and Dear ImGui were chosen for the demo simply because they were the quickest way to see the simulation run, so feel free to plug in your own rendering engine instead. I am also looking for a game developer or an artist able to turn this into a more interesting game.

## Screenshot of the standalone demo application

This screenshot may not match the current state of the code, and what you see also depends on the loaded ruleset. Click on the image to watch a video of the simulation running.

[![OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox/blob/master/doc/OpenGlassBox.png)](https://youtu.be/zyLO9Ls_hME?feature=shared)

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
- **Build tools**: C++14 compiler (`g++` or `clang++`), GNU Make, Git. C++14 is required for `std::make_unique`; otherwise, the code is largely C++11-compatible.
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

The `.ogs` language is specified in [`demo/data/Simulations/README.md`](demo/data/Simulations/README.md).

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

## Notes on the port

What changed compared to the original source code:

- The original code was written in C# for the Unity engine. Since I do not develop in C#, I ported it to C++.
- The original project reused the names of the GDC talk. I renamed the classes whose names I found confusing:
  - `Box` is now `City`.
  - `Point` and `Segment` are now `Node` and `Way` (better aligned with graph theory terminology).
  - `ResourceBinCollection` is now `Resources`.
  - `SimulationDefinitionLoader` is now `ScriptParser`.
- The original project never implemented the `Area` class, also called `Zone`. OpenGlassBox has `Area` and `AreaRule`, with `spawn`, `upgrade`, `destroy` and `count` commands. Areas and Units are kept independent: a zone is a rectangle of cells, and a building is not a graph node.
- A `Unit` used to be forced onto a `Node` of a `Path`, which made the graph explode. A Unit now has its own position, plus an optional anchor either at an offset along a `Way` or on a `Node`. That anchor is not decoration: `spawn ... at nearestWay` only grows a building on a cell a road runs through or fronts, and the demo draws the link. A building with no way to reach the network could neither send nor receive an `Agent`, so an `Area` refuses to grow one at all rather than scatter houses in a field.
- The original project implemented a dynamic A\* in `Path::FindNextPoint`. Routing now goes through `IRouter` (`Dijkstra` or `AStarRouter`) and minimises BPR travel times. The flow of each `Way` is an exponential moving average and **not** the MSA solver of CiudadSim; the traffic section below explains the difference.
- There are only two file formats: `.ogs` for the ruleset and `.ogc` for a save, whose header holds a hash of the ruleset and the list of the types used. There is no third "world" file.
- One application window shows one city. A `World` can still hold several of them: a road crossing a border is split, each half belonging to the city containing its midpoint, once `World::Listener` has allowed it. Routing stays inside one `Path`, so an agent never wanders into the neighbouring city on its own.
- The script parser sits behind `IScriptParser`, so another front end, a Forth one for instance, can be plugged in without touching the engine.
- The original project had neither unit tests nor comments. I added both.
- Since I cared about the simulation rather than the rendering, the dependencies on the Unity engine and its decorator classes were not ported, the library was separated from the demo, and the demo uses GLFW, OpenGL 3.3 and Dear ImGui (docking branch) rather than a full game engine. There is no SDL.

## Using the demo application

The map holds a SimCity-style vertical rail on its left. From top to bottom: **Play / Pause**, then the six tools — **Inspect**, **Roads**, **Zones**, **Buildings**, **Maps**, **Demolish** — then **Undo** and **Redo**. Keys `1` to `6` select a tool and the space bar toggles the pause.

The row to the right of the rail holds the settings of the selected tool, and only those:

- **Inspect** highlights whatever is under the cursor — a building, an agent, a road, a node, a zone, or the grid cell when there is nothing else — and a click sends it to the Inspector panel. This is the only tool that highlights cells; the others show the footprint of what they are about to do instead.
- **Roads** drags a segment of the chosen way type. Both ends snap to a nearby node, and otherwise to the world grid.
- **Zones** and **Maps** paint a rectangle. Click for a single square of the brush size, or drag for a rectangle. Zones do not overlap: painting Commercial over part of a Residential rectangle re-zones exactly the cells you painted and leaves the rest residential.
- **Maps** also carries the layer list, since choosing which map to paint and which map to look at is the same decision. Each row toggles the visibility of a map, sets its opacity, and picks how it is drawn: filled cells, contours, or numeric values. Clicking a name makes it the main layer; Alt+clicking shows it alone. A simulation opens with one map shown, because half a dozen heatmaps stacked on the same cells cannot be read.
- **Buildings** drops a building on a road without splitting it.
- **Demolish** removes a building, a road or an orphan node, and holds **Clear city**, which throws away everything that was built but keeps the ruleset, so roads can be laid again straight away. That one cannot be undone; everything else can, with Ctrl+Z and Ctrl+Y.

**Recenter** and the **Zoom** slider sit on the row below, next to the display toggles. You can also pan by dragging with the middle or right button, zoom with the wheel, and frame the whole city with the Home key.

Files are handled through File → New city, Open ruleset (`.ogs`), Open city and Save city (`.ogc`). The Script panel edits the open `.ogs` and Apply reparses it, keeping the city as long as every type it still uses is defined by the new ruleset. Simulation clock, Inspector, Rule Log, Charts, Traffic, History and Script are dockable panels. The canvas shows `Jour N  HH:MM` in its top left corner and tints the background through night, dawn, day and dusk; the charts use game hours on the X axis.

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

### Why the flow is smoothed

If routing used the instantaneous number of agents on each road, the whole population would swap between two parallel roads every tick: everyone sees road A empty, everyone moves to A, A is now jammed, everyone moves back to B. OpenGlassBox avoids that by feeding the BPR function an exponential moving average of the count $n$ of agents on the road, with a fixed weight $\alpha$ (0.05 by default, adjustable in the Traffic panel):

$$\displaystyle f \leftarrow (1-\alpha)\, f + \alpha\, n$$

This damps the oscillation, and that is all it does. It is deliberately **not** the solver used by traffic engineering tools such as CiudadSim, whose **MSA** (Method of Successive Averages) computes an all-or-nothing assignment $y^k$ onto the shortest paths at iteration $k$ and averages it in with a decreasing weight:

$$\displaystyle f^{k+1} = (1-\lambda_k)\, f^k + \lambda_k\, y^k,\qquad \lambda_k = \frac{1}{k}$$

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

Rules move resources between the four, and they are the gameplay. OpenGlassBox adds a fifth: **Areas**, the RCI zones that were in the talk but missing from MultiAgentSimulation. An Area is a rectangle of cells whose rules spawn, upgrade and demolish buildings.

The consequence for you is that the demo is only a **host**. The gameplay lives in the `.ogs` ruleset, and you should be able to build a different game without writing C++. What is in place today:

- the `.ogs` rule language, documented in [`demo/data/Simulations/README.md`](demo/data/Simulations/README.md);
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

- GDC talk slides: http://www.andrewwillmott.com/talks/inside-glassbox
- Since the original conference video is no longer available, an alternative GDC talk can be found here: https://youtu.be/eZfj7LEFT98
- A Scilab traffic assignment toolbox: https://www.rocq.inria.fr/metalau/ciudadsim and https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf. For more information, see other PDFs at https://jpquadrat.github.io/ in the section *Modélisation du Trafic Routier*.
- A tutorial for building a city builder (focused on rendering with SFML): https://www.binpress.com/creating-city-building-game-with-sfml/
- Moving cars: http://lo-th.github.io/root/traffic/ (source code: https://github.com/lo-th/root/tree/gh-pages/traffic, a fork of https://github.com/volkhin/RoadTrafficSimulator)
- A work-in-progress, open-source, multiplayer city simulation game: https://github.com/citybound/citybound
- An open-source version of Transport Tycoon: https://github.com/OpenTTD/OpenTTD
