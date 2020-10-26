# OpenGlassBox

[![License: MIT](https://img.shields.io/github/license/Lecrapouille/OpenGlassBox?style=plastic)](https://github.com/Lecrapouille/OpenGlassBox/blob/master/LICENSE)
[![codecov](https://codecov.io/gh/Lecrapouille/OpenGlassBox/branch/master/graph/badge.svg)](https://codecov.io/gh/Lecrapouille/OpenGlassBox)
[![coveralls](https://coveralls.io/repos/github/Lecrapouille/OpenGlassBox/badge.svg?branch=master)](https://coveralls.io/github/Lecrapouille/OpenGlassBox?branch=master)

|Branch     | **`Linux/Mac OS`** | **`Windows`** |
|-----------|------------------|-------------|
|master     |[![Build Status](https://travis-ci.org/Lecrapouille/OpenGlassBox.svg?branch=master)](https://travis-ci.org/Lecrapouille/OpenGlassBox)|[![Build status](https://ci.appveyor.com/api/projects/status/github/lecrapouille/OpenGlassBox?svg=true)](https://ci.appveyor.com/project/Lecrapouille/OpenGlassBox)|
|development|[![Build Status](https://travis-ci.org/Lecrapouille/OpenGlassBox.svg?branch=dev-refacto)](https://travis-ci.org/Lecrapouille/OpenGlassBox)||

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a personal implementation of Maxis SimCity 2013 simulation engine named GlassBox presented in these GDC conference 2012 [slides](http://www.andrewwillmott.com/talks/inside-glassbox).
Note this project is neither a Maxis's released code source nor an affiliated project to Maxis but simply a portage from C#/Unity to C++11/SDL of https://github.com/federicodangelo/MultiAgentSimulation that is already aged of more than 8 years.

## Screenshot

Note: this figure may not refer to the latest developement state which also depends on the loaded smulation script.
![alt tag](https://github.com/Lecrapouille/OpenGlassBox/blob/master/doc/OpenGlassBox.png)

In this screenshot:
- In pink: houses (static).
- In cyan: factories (static).
- In yellow: People going from houses to factories (dynamic).
- In white: People going from factories to houses (dynamic).
- In grey: nodes (crossroads) and ways (roads) (static).
- In blue: water produced by factories (dynamic).
- In green: grass consuming water (dynamic).
- Grid: city holding maps (grass, water), paths (ways, nodes) and units (producing agents moving along paths and carrying resources from an unit to another unit).

## Compilation Prerequisite

- Operating Sytems: Linux, Mac OS X.
- Compilation tools: C++11 (g++ or clang++), Gnu Makefile and bc (basic calculator needed for my Makefile).
- Renderer libraries: SDL2, SDL2_image (sudo apt-get install libsdl2-dev libsdl2-image-dev)
- GUI libraries: [DearImGui](https://github.com/ocornut/imgui), [imgui_sdl](https://github.com/Tyyppi77/imgui_sdl) (automatically downloaded but not installed).
- Debug library (if and only if you compile the project in debug mode): [backward-cpp](https://github.com/bombela/backward-cpp) (automatically downloaded but not installed).
- Non regression tests (optional and only for developers): [googletest](https://github.com/google/googletest) (download, compile and install manually), gcov (sudo apt-get install).
- Makefile helper [MyMakefile](https://github.com/Lecrapouille/MyMakefile): (automatically downloaded when git cloning recursively).

## Download, compilation and run

- Download git source recursively:
```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
```

- Compile the project:
```sh
cd OpenGlassBox/
make download-external-libs
make CXX=g++
```

- Run OpenGlassBox:
```sh
./build/OpenGlassBox
```

- (Optional) Unit test with code coverage:
```sh
cd OpenGlassBox/test
make coverage
```

## Notes concerning the portage

Current changes from original code source:
- The original code was made in C#. Since I'm never developping in C# I made a portage into C++11.
- Since I'm more interested by the simulation than the rendering, dependencies to the game engine Unity and Decorator classes for using it have not been ported. For rendering the simulation, I'm not using game engine and SDL2 is enough to draw lines and dots.
- The original project was using the same names than the GDC conference. I renamed classes:
  - Box is now named City.
  - Point is now named Node.
  - Segment is now named Way.
  - ResourceBinCollection is simply named Resources.
  - SimulationDefinitionLoader is now renamed ScriptParser. Currently I made a quick & dirty script straightforward parser. I wish to replace it by Forth parser. Wish to show SimCity as a spreadsheet: insert/edit cells to add simulation rules.
- The original project did not implement Area class (aka Zone). Area manages Units (creation, upgrade, destruction). This is also has to be added for this project. Currently an Unit shall be coupled to a Node of the Path. This is not particurlaly nice
since it creates lot of graph nodes.
- The original project implemented a dynamic A* algorithm in Path::FindNextPoint. I have created a Dijkstra class instead.
- The original project did not come with unit tests or comments. Added !

## How to play?

- For the moment, you cannot construct you game interactively. A prebuild game is made in `main.cpp` inside `bool GlassBox::initSimulation()` you can adapt it to create your own map.
- Simulation script is located at `OpenGlassBox/data/Simulations/TestCity.txt`.
- During the simulation, you can type the `d` key to show the debug window showing internal states of the simulation.

## Ideas for the next ?

- Import [OpenStreetMap](https://www.openstreetmap.org) maps.
- Implement some ideas explained in this video [Exploring SimCity: A Conscious Process of Discovery](https://youtu.be/eZfj7LEFT98).
- Update the dynamic A* algorithm to take into account of the traffic flow instead of shorter path.
- Parallelize algorithm: dispatch over all CPU cores (ie using OpenMP) or distribute computations (ie network, peer to peer).
- Agent should be directly attached to Ways and know the distance to next Agent.

## References

- Slides from the GDC conference can be downloaded here http://www.andrewwillmott.com/talks/inside-glassbox
- Since the video of this conference is no longer available, an alternative GDC conference video can be found here: https://youtu.be/eZfj7LEFT98
- A Scilab traffic assignment toolbox: https://www.rocq.inria.fr/metalau/ciudadsim and https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf For more information on this work, you can find other pdf at https://jpquadrat.github.io/ in section *Modélisation du Trafic Routier*
- A tutorial to make a city builder (more focused on rendering with the library SFML) https://www.binpress.com/creating-city-building-game-with-sfml/
- Moving cars: http://lo-th.github.io/root/traffic/ (code source https://github.com/lo-th/root/tree/gh-pages/traffic a fork based on https://github.com/volkhin/RoadTrafficSimulator)
- A work-in-progress, open-source, multi-player city simulation game: https://github.com/citybound/citybound
- An open source version of the game Transport Tycoon: https://github.com/OpenTTD/OpenTTD
