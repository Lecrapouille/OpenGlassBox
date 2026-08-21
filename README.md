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
./build/OpenGlassBox-demo Simulations/Braess.txt
```

The simulation starts paused. Use the Time panel to play, step ticks, and change the speed from x0.25 to x16. File → Open loads another script; the current file is also watched on disk and hot-reloaded.

- Default script: `demo/data/Simulations/TestCity.txt` (Home / Work / Shop, areas, BPR traffic).
- `demo/data/Simulations/Braess.txt`: four-node network that exhibits the Braess paradox. Watch the Traffic panel for the relative gap and the total travel time.

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
- The original project did not implement the `Area` class (a.k.a. `Zone`). OpenGlassBox now has `Area` / `AreaRule` with `spawn`, `upgrade`, `destroy` and `count` commands.
- A `Unit` used to be forced onto a `Path` `Node`, which exploded the graph. A Unit now has its own position and an optional anchor on a `Way` (offset) or a `Node`.
- The original project implemented a dynamic A* algorithm in `Path::FindNextPoint`. Routing now minimises BPR travel times (inspired by [CiudadSim](https://www.rocq.inria.fr/metalau/ciudadsim)), with MSA flow smoothing and a cached itinerary per Agent.
- I implemented a script parser behind `IScriptParser`, so a Forth backend can be plugged in later without touching the engine.
- The original project had no unit tests or comments. I added both.
- Since I was more interested in the simulation than in rendering:
  - dependencies on the Unity engine and its decorator classes were not ported;
  - the library was separated from the demo application;
  - the demo uses GLFW + OpenGL 3.3 + Dear ImGui (docking) instead of a full game engine such as Unity.

## Using the demo application

- Tools in the map toolbar let you lay roads, place buildings along a Way (without splitting it), paint resource maps, paint Areas and bulldoze, all with undo/redo.
- File → Open loads a simulation script. The current script is also watched on disk and hot-reloaded.
- The simulation script is located at `demo/data/Simulations/TestCity.txt`. `Braess.txt` is a four-node network that exhibits the Braess paradox.
- Time, Layers, Inspector, Rule Log, Charts and Traffic panels are dockable. Pause, step ticks, and change the speed from x0.25 to x16.

## Future ideas

- Import [OpenStreetMap](https://www.openstreetmap.org) maps.
- Implement ideas from [Exploring SimCity: A Conscious Process of Discovery](https://youtu.be/eZfj7LEFT98).
- Parallelize the simulation: dispatch work across CPU cores (e.g. with OpenMP) or distribute it over the network (peer-to-peer).
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
