# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a C++14 city-simulation engine inspired by **GlassBox**, the engine behind Maxis's SimCity (2013). It is a port and extension of Federico D'Angelo's C#/Unity project, [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation), itself based on the [2012 GDC presentation](http://www.andrewwillmott.com/talks/inside-glassbox).

OpenGlassBox is an independent project: neither OpenGlassBox nor MultiAgentSimulation contains Maxis source code or is affiliated with Maxis.

The OpenGlassBox project provides:

- static and shared libraries containing the simulation engine;
- a standalone "SimCity-like" 2D demo built using the simulation engine.

The engine does not depend on the demo renderer, so it can be embedded in another application or connected to a different rendering engine. **Note: I am also looking for a game developer or an artist able to turn this library into a real game.**

Differences with MultiAgentSimulation:

- The original project uses C# for Unity. This is C++14, with no game engine underneath.
- The simulation is a library that knows nothing about drawing, and the demo is a separate program on top of it, so you can plug in your own renderer.
- The original reused the names of the GDC talk. The ones I found confusing were renamed:
  - `Box` became `City`,
  - `Point` and `Segment` became `Node` and `Way` (the vocabulary of graph theory),
  - `ResourceBinCollection` became `Resources`,
  - `SimulationDefinitionLoader` became `ScriptParser`.
  - I kept the term `Unit`, which means `Building` (but more generic).
  - Some classes have been added: `Area` (a.k.a. `Zone`).
- The original project had neither unit tests nor comments. This project has both: every header carries its documentation and a small example, and the test suite covers the engine down to individual rule commands.
- Some parts of the code have been optimized from the original project.

See [this document](doc/improvements.md) for more information.

## Installing system packages

### Prerequisites

- **Operating systems**: Linux, macOS. Should compile on Windows as well.
- **Build tools**: C++14 compiler (`g++` or `clang++`), GNU Make, Git. C++14 is required for `std::make_unique`; otherwise, the code is largely C++11-compatible.
- **Debug library** (debug builds only): [backward-cpp](https://github.com/bombela/backward-cpp) : automatically downloaded and built by the Makefile (not installed system-wide).
- **Unit tests** (optional, for developers): [Google Test](https://github.com/google/googletest) (must be downloaded, built, and installed manually), plus coverage tools (see below).
- **Makefile helper** [MyMakefile](https://github.com/Lecrapouille/MyMakefile): automatically fetched when cloning with `--recursive`.

GLFW and Dear ImGui were chosen for the demo simply because they were the quickest way to see the simulation run, so feel free to plug in your own rendering engine instead.

- **Renderer libraries**: GLFW 3, GLEW and OpenGL 3.3 (must be installed on your system; only needed for the demo; see below).
- **GUI libraries**: [Dear ImGui](https://github.com/ocornut/imgui) (docking), [ImPlot](https://github.com/epezent/implot) and [ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog) : automatically downloaded and built by the Makefile (not installed system-wide; only needed for the demo).

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

Note: `cmake` is only needed if you build and install Google Test from source (see the CI workflow for an example).

## Download and compile OpenGlassBox

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

A `build` folder shall have been created with executables and libraries. On macOS, a bundle application is also created inside the build folder.

Adjust `-j8` to the number of cores of your machine, and pick a compiler with `make CXX=clang++ -j8` if you prefer. Builds are optimized with debug symbols by default (`COMPILATION_MODE := normal` in `Makefile.common`). Use `make COMPILATION_MODE=debug -j8` to step through the code, and `make COMPILATION_MODE=release -j8` to ship. The mode matters: a map rule runs over every cell of a city, so the Chicago save with its three hundred thousand cells is an order of magnitude slower to simulate when compiled without optimisations.

(Optional) Install OpenGlassBox on your system:

```sh
sudo make install
```

(Optional) Run unit tests with code coverage:

```sh
cd OpenGlassBox/tests
make coverage -j8
```

## How to integrate OpenGlassBox in your project?

Here is a basic example on how to link your project against OpenGlassBox:

```sh
git clone https://github.com/Lecrapouille/LinkAgainstMyLibs.git --recursive

cd LinkAgainstMyLibs/OpenGlassBox
make -j8
./build/OpenGlassBox
```

TODO

## OpenGlassBox demo application

Run the demo:

```sh
./build/OpenGlassBox-demo
./build/OpenGlassBox-demo demo/data/Simulations/chicago.ogc
```

This screenshot may not match the current state of the code, and what you see also depends on the loaded ruleset.

![OpenGlassBox](doc/OpenGlassBox.png)

One window shows Chicago city. The simulation starts paused: press `Play` on the map toolbar or the space bar to start it. The `Simulation clock` panel steps it tick by tick while paused and changes the speed from `x0.25` to `x16`. Opening a ruleset (`.ogs`) starts from an empty city, while a city save (`.ogc`) holds the geometry, the live state and a hash of the ruleset it was made with. The `inspector` panel allow you to show information on any element of the game. The left panel allows you to edit your city.

(Work in progress) The demo includes several simulations. Their purpose, files, and `.ogs` language are documented in the [simulation file reference](demo/data/Simulations/README.md).

See [this document](doc/demo.md) for more information.

## The simulation engine, class by class

The GlassBox approach describes a city simulation as data rather than as a tree of objects with an `Update()` method. OpenGlassBox models five kinds of data:

- **Areas** are zones whose rules spawn, upgrade, and demolish buildings.
- **Maps** are 2D fields over the grid, such as water, pollution, and desirability.
- **Units** are buildings that hold bounded stocks of resources.
- **Agents** carry resources from one unit to another.
- **Resources** are money, goods, happiness ...
- **Paths** are networks on which agents travel.

See [this document](doc/engine.md) for more information.

## The script language reference

The file format for storing a ruleset: is `.ogs`. The rule language, documented in the [simulation file reference](demo/data/Simulations/README.md);

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
