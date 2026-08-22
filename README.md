# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an implementation of the GlassBox simulation engine from Maxis's SimCity (2013), based on the [2012 GDC talk slides](http://www.andrewwillmott.com/talks/inside-glassbox). This project is neither Maxis's official source code nor affiliated with Maxis. It is a C++14 port of the well-written [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation) project — originally written in C# for the Unity game engine more than 8 years ago.

This project builds:

- static and shared libraries for the simulation engine;
- a standalone demo application rendered with GLFW, OpenGL 3.3 and [Dear ImGui](https://github.com/ocornut/imgui) (docking branch), with [ImPlot](https://github.com/epezent/implot) charts and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog).

The simulation engine and the demo renderer are kept separate: I was more interested in the simulation logic than in rendering. GLFW and Dear ImGui were chosen for the demo because they were the quickest way to visualize the simulation — feel free to plug in your own rendering engine instead :) I'm also looking for a game dev / artist ables to make a more interesting demo game.

## Screenshot of the standalone demo application

Note: this screenshot may not reflect the latest development state, which also depends on the loaded simulation script.
Click on the image to watch a video of the simulation.
[![OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox/blob/master/doc/OpenGlassBox.png)](https://youtu.be/zyLO9Ls_hME?feature=shared).

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

The simulation logic and the rendering layer are separated in the code. GLFW and Dear ImGui are only used to display simulation states in the demo — use your own rendering engine for integration.

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

Adjust `-j8` to match the number of CPU cores on your machine. You can also change your compiler: `make CXX=g++ -j8` if wanted.

Run the demo (from the project root, after `make`):

```sh
./build/OpenGlassBox-demo
./build/OpenGlassBox-demo braess.ogc
```

The simulation starts paused. Use the Time panel to play, step ticks, and change the speed from x0.25 to x16. One window is one city. File → Open ruleset (`.ogs`) starts an empty city; File → Open city / Save city uses a `.ogc` (geometry + live state + hash of the ruleset). The Script panel can Apply a reparse without dropping the city when the types still placed still exist.

- Default: `demo/data/Simulations/test_city.ogs` + `test_city.ogc` (Home / Work / Shop, areas, BPR traffic).
- `braess.ogs` / `braess.ogc`: four-node Braess paradox. Watch the Traffic panel for Relgap and total travel time.
- `regular.ogs` / `regular.ogc`: CiudadSim-style `Regular(6,6)` grid, bidirectional ways.
- `chicago.ogs` / `chicago.ogc`: simplified arteries (not the Scilab 546-node network).

The language of `.ogs` is specified in [`demo/data/Simulations/README.md`](demo/data/Simulations/README.md).

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

Changes made compared to the original source code:

- The original code was written in C# for the Unity engine. Since I'm never developing in C#, I ported it to C++.
- The original project reused names from the GDC talk. I renamed classes whose names I found confusing:
  - `Box` is now `City`.
  - `Point` and `Segment` are now `Node` and `Way` (better aligned with graph theory terminology).
  - `ResourceBinCollection` is now `Resources`.
  - `SimulationDefinitionLoader` is now `ScriptParser`.
- The original project did not implement the `Area` class (a.k.a. `Zone`). OpenGlassBox now has `Area` / `AreaRule` with `spawn`, `upgrade`, `destroy` and `count` commands. Areas and Units are decoupled: a zone is a rectangle of cells; a building is not a graph node.
- A `Unit` used to be forced onto a `Path` `Node`, which exploded the graph. A Unit now has its own position and an optional anchor on a `Way` (offset) or a `Node`.
- The original project implemented a dynamic A* algorithm in `Path::FindNextPoint`. Routing implements `IRouter` (`Dijkstra` / `AStarRouter`) and minimises BPR travel times. Flow on each Way is an exponential moving average, **not** the MSA solver of CiudadSim (see below).
- Two files: `.ogs` (ruleset) and `.ogc` (one save: header with ruleset hash + types, geometry, live state). There is no third “world” file.
- One application window holds one city. A `World` may still contain several cities; a road that crosses a border is split, each piece owned by the city that contains its midpoint, after `World::Listener` authorises it. Dijkstra stays intra-Path: an agent does not walk into the neighbouring city by itself.
- I implemented a script parser behind `IScriptParser`, so a Forth backend can be plugged in later without touching the engine.
- The original project had no unit tests or comments. I added both.
- Since I was more interested in the simulation than in rendering:
  - dependencies on the Unity engine and its decorator classes were not ported;
  - the library was separated from the demo application;
  - the demo uses GLFW + OpenGL 3.3 + Dear ImGui (docking) instead of a full game engine such as Unity. There is no SDL.

## Using the demo application

- A SimCity-style rail on the left of the map: Inspect, Roads, Zones, Buildings, Maps, Demolish. The palette under the rail lists the types from the open `.ogs`. Undo / Redo sit on the rail. Demolish can Clear city (ruleset kept).
- Roads snap to the world grid. Zones and map paint drag as soon as the canvas is hovered. Inspect can pick a Way. Bulldoze removes orphan nodes.
- File → New city / Open ruleset (`.ogs`), Open city / Save city (`.ogc`). The Script panel edits the `.ogs` and Apply reparses it.
- Time, Layers, Inspector, Rule Log, Charts, Traffic and Script are dockable. The canvas HUD shows `Jour N  HH:MM` and tints night / dawn / day / dusk. Charts use game hours on the X axis.

## Traffic: BPR, MSA, and what OpenGlassBox actually does

**BPR** (Bureau of Public Roads), already the cost of a `Way` (`Way::travelTime`):

$$\displaystyle t(f) = t_0 \left(1 + 0.15 \left(\frac{f}{c}\right)^{\beta}\right)$$

**MSA** as in CiudadSim (assignment solver, **not implemented**). At iteration \(k\), \(y^k\) is an all-or-nothing assignment onto shortest paths, then

$$\displaystyle f^{k+1} = (1-\lambda_k)\, f^k + \lambda_k\, y^k,\qquad \lambda_k = \frac{1}{k}$$

**OpenGlassBox** instead smooths the live count of agents \(n\) on the Way with a fixed \(\alpha\) (default \(0{,}05\)):

$$\displaystyle f \leftarrow (1-\alpha)\, f + \alpha\, n$$

This is not a \(1/k\) average and it does not converge to a Wardrop equilibrium: it **damps** the A↔B oscillation.

**Relgap** in the Traffic panel is a diagnostic, not a solver stopping rule:

$$\displaystyle \mathrm{Relgap} = \frac{\mathrm{TSTT} - \mathrm{SPTT}}{\mathrm{TSTT}}$$

`City` owns an `IRouter` (`findRoute`, `shortestPathCost`). Swap the implementation without touching Agents.

## For authors who want their own SimCity

GlassBox (Willmott, GDC 2012) is **data**, not a tree of `Update()` objects. Four families: Maps (2D fields), Units (buildings with bounded bins), Agents (mobiles that carry resources), Paths (the pipe). Areas (RCI zones) were in the talk and missing from MultiAgentSimulation; they are here. The demo is a **host**: gameplay lives in the `.ogs`. The more stable the language and the `.ogc` save, the less C++ you need to touch.

Already in this tree:

- `.ogs` language (see `demo/data/Simulations/README.md`)
- `.ogc` save with a ruleset fingerprint
- `IRouter` for another pathfinder
- `World::Listener` (`allowWayAcross` / `allowWayRemoved`) for road diplomacy
- TestCity as a documented recipe

Later, without doing it in this lot:

- Register custom `IRuleCommand` implementations from C++ without forking the parser
- Freeze a query API for the UI (map totals, agents by type, Relgap, budget)
- Service networks as their own `Path` (water, power), or the convention “one Map = one coverage”
- Occupancy / queues on a Way (original GlassBox: the agent *is* the traffic)
- Staged construction (`upgrade` exists; a timer / Money cost does not)
- Deterministic seed + replay for rule debugging

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
