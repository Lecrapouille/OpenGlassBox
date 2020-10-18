# OpenGlassBox

[![License: MIT](https://img.shields.io/github/license/Lecrapouille/OpenGlassBox?style=plastic)](https://github.com/Lecrapouille/OpenGlassBox/blob/master/LICENSE)
[![codecov](https://codecov.io/gh/Lecrapouille/OpenGlassBox/branch/master/graph/badge.svg)](https://codecov.io/gh/Lecrapouille/OpenGlassBox)
[![coveralls](https://coveralls.io/repos/github/Lecrapouille/OpenGlassBox/badge.svg?branch=master)](https://coveralls.io/github/Lecrapouille/OpenGlassBox?branch=master)

|Branch     | **`Linux/Mac OS`** | **`Windows`** |
|-----------|------------------|-------------|
|master     |[![Build Status](https://travis-ci.org/Lecrapouille/OpenGlassBox.svg?branch=master)](https://travis-ci.org/Lecrapouille/OpenGlassBox)|[![Build status](https://ci.appveyor.com/api/projects/status/github/lecrapouille/OpenGlassBox?svg=true)](https://ci.appveyor.com/project/Lecrapouille/OpenGlassBox)|
|development|[![Build Status](https://travis-ci.org/Lecrapouille/OpenGlassBox.svg?branch=dev-refacto)](https://travis-ci.org/Lecrapouille/OpenGlassBox)||

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a personal implementation from the slides of Maxis SimCity 2013 simulation engine named GlassBox and shown in at this [GDC conference](http://www.andrewwillmott.com/talks/inside-glassbox). Note this project is neither Maxis's released code source nor a project affiliated with them. This project is simply a portage from C#/Unity to C++11/SDL of https://github.com/federicodangelo/MultiAgentSimulation.
This current project is a sandbox: a preliminary work for its integration for another work-in-progress project of mine.

## Prerequisite

- Compilation tools: C++11 (g++ or clang++), Gnu Makefile and bc (basic calculator needed for my Makefile).
- Renderer: SDL2, SDL2_image (sudo apt-get install libsdl2-dev libsdl2-image-dev)
- GUI: [DearImGui](https://github.com/ocornut/imgui), [imgui_sdl](https://github.com/Tyyppi77/imgui_sdl) (automatically downloaded but not installed).
- Debug: [backward-cpp](https://github.com/bombela/backward-cpp) (automatically downloaded but not installed if and only if you compile the project in debug mode).
- Non regression tests (optional and only for developers): [googletest](https://github.com/google/googletest), gcov.
- [MyMakefile](https://github.com/Lecrapouille/MyMakefile) used instead of CMake and installed when git cloning recursively.

## Download, compilation and run

- Download source:
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

- (Optional) Unit test, code coverage:
```sh
cd OpenGlassBox/test
make coverage
```

## Notes concerning the portage

- Dependencies to the game engine Unity has been removed.
- Since I'm more interested by the simulation than the game renderer, I'm not using game engine but, for the moment, SDL2 which is enough to draw lines and dots. SDL2 may be replaced by OpenGL or by SFML in the futur.
- The original project did not come with unit tests or comments. Added !
- The original project was using the same names than the GDC conference. I renamed classes:
  - Box is now named City.
  - Point is now named Node.
  - Segment is now named Way.
  - ResourceBinCollection is simply named Resources (collection of Resource).
  - Unit to is now named ??? (to be defined).
  - SimulationDefinitionLoader is now renamed ScriptParser.
  - I added class Dijkstra instead of the method Path::FindNextPoint implementing a dynamic A* algorithm.
- The original project did not implement Area class (aka Zone). This is also has to be added for this project.
- Currently an Unit shall be coupled to a Node of the Path.
- (work in progress) Class coupling simplification from the initial code. Still in progress but Agent in the original code known by Node but not by Ways this can more difficult to compute flow. Still in progress D* algorithm is made inside the Node class will be made by a separated class. Unit are linked to a Node on a Path this is not great since that force splitting Ways to add Node and makes the graph grows up too much.
- Currently I made a quick & dirty script straightforward parser. I wish to replace it by Forth parser.
- Wish to show SimCity as a spreadsheet: insert/edit cells to add simulation rules.

## Screenshot

![alt tag](https://github.com/Lecrapouille/OpenGlassBox/blob/master/doc/OpenGlassBox.png)
- In pink: houses.
- In cyan: works.
- In red: People travelling.
- In grey: path (nodes and ways).

## References

- The GDC conference http://www.andrewwillmott.com/talks/inside-glassbox with some explanations here https://www.gamasutra.com/view/news/164870/GDC_2012_Breaking_down_SimCitys_Glassbox_engine.php or https://www.reddit.com/r/SimCity/comments/1a9fz6/how_the_glassbox_engine_works_based_on_the_gdc/
- A Scilab traffic assignment toolbox: https://www.rocq.inria.fr/metalau/ciudadsim and https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf For more information on this work, you can find other pdf at https://jpquadrat.github.io/ in section *Modélisation du Trafic Routier*
- A tutorial to make a city builder (more focused on rendering with the library SFML) https://www.binpress.com/creating-city-building-game-with-sfml/
- Moving cars: http://lo-th.github.io/root/traffic/ (code source https://github.com/lo-th/root/tree/gh-pages/traffic a fork based on https://github.com/volkhin/RoadTrafficSimulator)
- A work-in-progress, open-source, multi-player city simulation game: https://github.com/citybound/citybound
- An open source version of the game Transport Tycoon: https://github.com/OpenTTD/OpenTTD
