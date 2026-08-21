# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an implementation of the GlassBox simulation engine from Maxis's SimCity (2013), based on the [2012 GDC talk slides](http://www.andrewwillmott.com/talks/inside-glassbox). This project is neither Maxis's official source code nor affiliated with Maxis. It is a C++14 port of the well-written [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation) project — originally written in C# for the Unity game engine more than 8 years ago.

This project builds:

- static and shared libraries for the simulation engine;
- a standalone demo application rendered with SDL2 and [Dear ImGui](https://github.com/ocornut/imgui).

The simulation engine and the demo renderer are kept separate: I was more interested in the simulation logic than in rendering. SDL2 and Dear ImGui were chosen for the demo because they were the quickest way to visualize the simulation — feel free to plug in your own rendering engine instead :) I'm also looking for a game dev / artist ables to make a more interesting demo game.

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
- **Renderer libraries**: SDL2 and SDL2_image (must be installed on your system; only needed for the demo — see below).
- **GUI libraries**: [Dear ImGui](https://github.com/ocornut/imgui) and [imgui_sdl](https://github.com/Tyyppi77/imgui_sdl) — automatically downloaded and built by the Makefile (not installed system-wide; only needed for the demo).
- **Debug library** (debug builds only): [backward-cpp](https://github.com/bombela/backward-cpp) — automatically downloaded and built by the Makefile (not installed system-wide).
- **Unit tests** (optional, for developers): [Google Test](https://github.com/google/googletest) (must be downloaded, built, and installed manually), plus coverage tools (see below).
- **Makefile helper** [MyMakefile](https://github.com/Lecrapouille/MyMakefile): automatically fetched when cloning with `--recursive`.

The simulation logic and the rendering layer are separated in the code. SDL2 and Dear ImGui are only used to display simulation states in the demo — use your own rendering engine for integration.

### Installing system packages

**Debian / Ubuntu**

```sh
# Required to build the demo
sudo apt-get install build-essential git pkg-config libsdl2-dev libsdl2-image-dev

# Optional: debug builds (backward-cpp) and code coverage
sudo apt-get install libdw-dev lcov
```

**Fedora**

```sh
# Required to build the demo
sudo dnf install gcc-c++ make git pkgconf-pkg-config sdl2-compat-devel SDL2_image-devel

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

Run the demo:

```sh
./demo/build-release/Demo/Demo
```

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
- The original project did not implement the `Area` class (a.k.a. `Zone`). `Area` manages `Unit` creation, upgrade, and destruction. This still needs to be added.
- A `Unit` is coupled to a `Node` on a `Path`. This is not ideal, as it creates many unnecessary graph nodes.
- The original project implemented a dynamic A* algorithm in `Path::FindNextPoint`. I replaced it with a `Dijkstra` class, but a proper traffic-aware routing algorithm is still needed.
- I implemented a quick-and-dirty script parser that is less elegant than the original. This was acceptable because I planned to replace the script syntax with [Forth](https://esp32.arduino-forth.com/) (which has a smaller footprint than Lua).
- The original project had no unit tests or comments. I added both.
- Since I was more interested in the simulation than in rendering:
  - dependencies on the Unity engine and its decorator classes were not ported;
  - the library was separated from the demo application;
  - the demo uses SDL2 to draw lines and dots instead of a full game engine such as Unity.

## Using the demo application

- You cannot build a city interactively yet. A sample simulation is defined in `demo/Demo.cpp` inside `bool GlassBox::initSimulation()` — edit this function to create your own map.
- The simulation script is located at `data/Simulations/TestCity.txt`.
- Once the demo has started, press the `d` key to toggle the debug window showing the internal state of the simulation.

## Future ideas

- Import [OpenStreetMap](https://www.openstreetmap.org) maps.
- Implement ideas from [Exploring SimCity: A Conscious Process of Discovery](https://youtu.be/eZfj7LEFT98).
- Improve pathfinding to account for traffic flow instead of simply choosing the shortest path.
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
