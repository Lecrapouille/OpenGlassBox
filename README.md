# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a C++14 city-simulation engine inspired by **GlassBox**, the engine behind Maxis's SimCity (2013). It is a port and extension of Federico D'Angelo's C#/Unity project, [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation), itself based on the [2012 GDC presentation](http://www.andrewwillmott.com/talks/inside-glassbox).

OpenGlassBox is an independent project: neither OpenGlassBox nor MultiAgentSimulation contains Maxis source code or is affiliated with Maxis.

The project provides:

- static and shared libraries containing the simulation engine;
- a standalone SimCity-like 2D demo built on top of that engine.

The engine does not depend on the demo renderer, so it can be embedded in another application or connected to a different rendering engine. **Note: I am also looking for a game developer or an artist able to turn this library into a real game.**

## Documentation

| Document | Contents |
| -------- | -------- |
| [Integration guide](doc/integration.md) | Link the library and drive a simulation from your own code. |
| [Demo application](doc/demo.md) | Tools, panels, and keyboard shortcuts. |
| [Engine](doc/engine.md) | Classes, tick order, traffic model. |
| [Scripts](doc/script.md) | The `.ogs` language: the core of the project. |
| [Improvements](doc/improvements.md) | Changes over MultiAgentSimulation. |
| [Bundled simulations](demo/data/Simulations/README.md) | Sample rulesets and the `test_city` walkthrough. |

## Differences from MultiAgentSimulation

- The original project uses C# for Unity. This is C++14, with no game engine underneath.
- The simulation is a library that knows nothing about drawing; the demo is a separate program on top of it.
- Confusing names from the GDC talk were renamed: `Box` → `City`, `Point`/`Segment` → `Node`/`Way`, `ResourceBinCollection` → `Resources`, `SimulationDefinitionLoader` → `ScriptParser`. The term `Unit` was kept (it means building). New classes include `Area` (zone).
- The original project had neither unit tests nor comments. This one has both.
- Several parts of the code were optimized for larger cities.

See [improvements](doc/improvements.md) for details.

## Installing system packages

### Prerequisites

- **Operating systems**: Linux, macOS. Should compile on Windows as well.
- **Build tools**: C++14 compiler (`g++` or `clang++`), GNU Make, Git. C++14 is required for `std::make_unique`; otherwise, the code is largely C++11-compatible.
- **Debug library** (debug builds only): [backward-cpp](https://github.com/bombela/backward-cpp): automatically downloaded and built by the Makefile (not installed system-wide).
- **Unit tests** (optional): [Google Test](https://github.com/google/googletest) (must be downloaded, built, and installed manually), plus coverage tools (see below).
- **Makefile helper** [MyMakefile](https://github.com/Lecrapouille/MyMakefile): automatically fetched when cloning with `--recursive`.

GLFW and Dear ImGui were chosen for the demo because they were the quickest way to see the simulation run; feel free to plug in your own renderer.

- **Renderer libraries**: GLFW 3, GLEW, and OpenGL 3.3 (system packages; demo only).
- **GUI libraries**: [Dear ImGui](https://github.com/ocornut/imgui) (docking), [ImPlot](https://github.com/epezent/implot), and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog): downloaded and built by the Makefile (demo only).

### Debian / Ubuntu

```sh
# Required to build the demo
sudo apt-get install build-essential git pkg-config libglfw3-dev libglew-dev

# Optional: debug builds (backward-cpp) and code coverage
sudo apt-get install libdw-dev lcov
```

### Fedora

```sh
# Required to build the demo
sudo dnf install gcc-c++ make git pkgconf-pkg-config glfw-devel glew-devel

# Optional: debug builds (backward-cpp) and code coverage
sudo dnf install elfutils-devel lcov cmake
```

`cmake` is only needed if you build and install Google Test from source (see the CI workflow for an example).

## Download and compile

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

This creates a `build` folder with executables and libraries. On macOS, a bundle application is also created inside the build folder.

Adjust `-j8` to the number of cores on your machine, or pick a compiler with `make CXX=clang++ -j8`. Builds are optimized with debug symbols by default (`COMPILATION_MODE := normal` in `Makefile.common`). Use `make COMPILATION_MODE=debug -j8` to step through the code, and `make COMPILATION_MODE=release -j8` to ship. The mode matters: a map rule runs over every cell of a city, so the Chicago save with its three hundred thousand cells is an order of magnitude slower to simulate when compiled without optimizations.

(Optional) Install OpenGlassBox on your system:

```sh
sudo make install
```

(Optional) Run unit tests with code coverage:

```sh
cd OpenGlassBox/tests
make coverage -j8
```

## Integrating OpenGlassBox into your project

The engine is meant to be linked from another application. A minimal workflow: parse a `.ogs` ruleset, create a city, attach a router, and call `simulation.update()` from your game loop.

See the [integration guide](doc/integration.md) for pkg-config flags, a code example, and extension points. A working sample also lives in [LinkAgainstMyLibs](https://github.com/Lecrapouille/LinkAgainstMyLibs):

```sh
git clone https://github.com/Lecrapouille/LinkAgainstMyLibs.git --recursive
cd LinkAgainstMyLibs/OpenGlassBox
make -j8
./build/OpenGlassBox
```

## Demo application

Run the demo:

```sh
./build/OpenGlassBox-demo
./build/OpenGlassBox-demo demo/data/Simulations/chicago.ogc
```

This screenshot may not match the current state of the code; what you see also depends on the loaded ruleset.

![OpenGlassBox](doc/OpenGlassBox.png)

One window shows one city. The simulation starts paused: press **Play** on the map toolbar or the space bar to start it. The **Simulation clock** panel steps the simulation tick by tick while paused and changes the speed from x0.25 to x16. Opening a ruleset (`.ogs`) starts from an empty city; a city save (`.ogc`) holds geometry, live state, and a hash of the ruleset it was built with. The **Inspector** panel shows details about any selected element; the left toolbar lets you edit the city.

The demo includes several bundled simulations, from the introductory `test_city` sandbox to traffic-focused road networks. See [bundled simulations](demo/data/Simulations/README.md) and [demo documentation](doc/demo.md).

## Simulation engine

The GlassBox approach describes a city simulation as data rather than as a tree of objects with an `Update()` method. OpenGlassBox models maps, units, agents, paths, areas, and the rules that connect them.

- **Areas** are zones whose rules spawn, upgrade, and demolish buildings.
- **Maps** are 2D fields over the grid, such as water, pollution, and desirability.
- **Units** are buildings that hold bounded stocks of resources.
- **Agents** carry resources from one unit to another.
- **Resources** are money, goods, happiness ...
- **Paths** are networks on which agents travel.

See [engine documentation](doc/engine.md) for the class-by-class reference and the traffic model.

## Scripts and rulesets

**Scripts are the most important part of the project.** Gameplay is defined in `.ogs` rulesets: resources, building types, agents, maps, zones, and the rules that move resources and grow cities. The C++ engine executes those rules; the demo is one host for editing and watching them.

- [Script language reference](doc/script.md): syntax, commands, and save format.
- [Bundled simulations](demo/data/Simulations/README.md): sample files and a worked `test_city` example.

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
