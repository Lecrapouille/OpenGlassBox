# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an implementation of Maxis SimCity 2013 simulation engine named GlassBox, based on the GDC conference 2012 [slides](http://www.andrewwillmott.com/talks/inside-glassbox). This project is neither a Maxis's released code source nor an affiliated project to Maxis but a portage to C++14 of the nicely written code [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation), now aged more than 8 years and written in C# for the Unity game engine.

This current project is compiled as:
- a static/shared libraries of the simulation engine.
- a standalone demonstration application displayed with SDL2 and DearImgui libraries.

I separated things because I was more interested in the simulation engine than the rendering.
For rendering the demo application states,  I choosed SDL2 and DearImGui because this was
the easier way for me but please, use your own personal/prefered rendering engine instead :)

## Screenshot of the standalone demo application

Note: this screeenshot may not refer to the latest development state, which also depends on the loaded simulation script.
Click on the image to watch the video of the simulation.
[![OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox/blob/master/doc/OpenGlassBox.png)](https://youtu.be/zyLO9Ls_hME?feature=shared).

In this screenshot:
- In pink: houses (static).
- In cyan: factories (static).
- In yellow: People going from houses to factories (dynamic).
- In white: People going from factories to houses (dynamic).
- In grey: nodes (crossroads) and ways (roads) (static).
- In blue: water produced by factories (dynamic).
- In green: grass consuming water (dynamic).
- Grid: city holding maps (grass, water), paths (ways, nodes), and units (producing agents moving along paths and carrying resources from one unit to another unit).

## Compilation Prerequisite

- Operating Systems: Linux, Mac OS X. Should compile for Windows.
- Compilation tools: C++14 (g++ or clang++), Gnu Makefile. The C++14 was used to use `std::make_unique`, else the whole code is C++11 compatible.
- Renderer libraries: SDL2, SDL2_image (You have to install them on your system `sudo apt-get install libsdl2-dev libsdl2-image-dev`).
- GUI libraries: [DearImGui](https://github.com/ocornut/imgui), [imgui_sdl](https://github.com/Tyyppi77/imgui_sdl) (automatically downloaded and compiled by the Makefile, but not installed).
- Debug library (if and only if you compile the project in debug mode): [backward-cpp](https://github.com/bombela/backward-cpp) (automatically downloaded and compiled by the Makefile, but not installed).
- Non-regression tests (optional and only for developers): [googletest](https://github.com/google/googletest) (you have to manually download the code source, compile it and install it on your system), gcov (you have to install on your system: `sudo apt-get install`).
- Makefile helper [MyMakefile](https://github.com/Lecrapouille/MyMakefile): (automatically downloaded when git cloning recursively).
Again: the logic and the rendering are separated in the code. SDL2 and DearImGui were used to display the game states for the demonstration. Use your own rendering engine instead.

## Download, compile and run

- Download git source recursively:
```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
```

- Compile the project (libs + demo):
```sh
cd OpenGlassBox/
make download-external-libs
make CXX=g++ -j8
```
Where `-j8` is to adapt to match the number of CPU cores.

- Run the OpenGlassBox demo:
```sh
./demo/build-release/Demo/Demo
```

- (Optional) Unit test with code coverage:
```sh
cd OpenGlassBox/test
make coverage -j8
```

- (Optional) To install OpenGlassBox on your operating system:
```sh
sudo make install
```

- Example of how to link OpenGlassBox in your project (once OpenGlassBox has been installed on your operating system):
```sh
git clone https://github.com/Lecrapouille/LinkAgainstMyLibs.git --recursive
cd LinkAgainstMyLibs/OpenGlassBox
make -j8
./build/OpenGlassBox
```

For Mac OS X users a bundle application is also created inside the build folder.

## Notes concerning the portage

Here are the current changes made from the original code source:
- The original code was made in C# for Unity engine. Since I'm never developing in C# I made a portage into C++14.
- The original project was using the same names as the GDC conference. I renamed classes whose name confused me:
  - `Box` is now named `City`.
  - `Point` and `Segment` are now named `Node` and `Way` (since will match more graph theory terms).
  - `ResourceBinCollection` is simply named `Resources`.
  - `SimulationDefinitionLoader` is now renamed `ScriptParser`.
- The original project did not implement `Area` class (aka `Zone`). `Area` manages `Units` (creation, upgrade, destruction). This also has to be added to this project.
- A `Unit` shall be coupled to a `Node` of the `Path`. This is not particularly nice since this will create a lot of unnecessary graph nodes.
- The original project implemented a dynamic A* algorithm in `Path::FindNextPoint`. I have created a `Dijkstra` class instead but a real traffic algorithm should have to be developed.
- Currently, I made a quick & dirty script straightforward parser. My code is less good than the original one. It was ok because I wished to replace the script syntax by [Forth](https://esp32.arduino-forth.com/) (which has less footprint than Lua).
- The original project did not come with unit tests or comments. Added!
- Since I was more interested in the simulation than the rendering:
  - dependencies to the game engine Unity and Decorator classes for using it have not been ported.
  - I have separated the library from the demo application.
  - For rendering the demo application, I'm not using a game engine such Unity. SDL2 is enough to draw lines and dots.

## How to play to demo application?

- For the moment, you cannot construct your game interactively. A prebuild game is made in `demo/Demo.cpp` inside `bool GlassBox::initSimulation()` you can adapt it to create your own map.
- Simulation script is located at `data/Simulations/TestCity.txt`.
- Once the demo started, press the `d` key to see the simulation.
- During the simulation, you can type the `d` key to show/hide the debug window showing the internal states of the simulation.

## Ideas for the next?

- Import [OpenStreetMap](https://www.openstreetmap.org) maps.
- Implement some ideas explained in this video [Exploring SimCity: A Conscious Process of Discovery](https://youtu.be/eZfj7LEFT98).
- Update the dynamic A* algorithm to take into account the traffic flow instead of a shorter path.
- Parallelize algorithm: dispatch over all CPU cores (i.e. using OpenMP) or distribute computations (i.e. network, peer to peer).
- `Agent` should be directly attached to `Ways` and for cars know the distance to the next `Agent`.
- I wished to show and manipulate a SimCity city as a spreadsheet: insert/edit cells to add simulation rules. This project could be merged
  with [SimTaDyn](https://github.com/Lecrapouille/SimTaDyn).

## References

- Slides from the GDC conference can be downloaded here http://www.andrewwillmott.com/talks/inside-glassbox
- Since the video of this conference is no longer available, an alternative GDC conference video can be found here: https://youtu.be/eZfj7LEFT98
- A Scilab traffic assignment toolbox: https://www.rocq.inria.fr/metalau/ciudadsim and https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf For more information on this work, you can find other PDF at https://jpquadrat.github.io/ in section *Modélisation du Trafic Routier*
- A tutorial to make a city builder (more focused on rendering with the library SFML) https://www.binpress.com/creating-city-building-game-with-sfml/
- Moving cars: http://lo-th.github.io/root/traffic/ (code source https://github.com/lo-th/root/tree/gh-pages/traffic a fork based on https://github.com/volkhin/RoadTrafficSimulator)
- A work-in-progress, open-source, multi-player city simulation game: https://github.com/citybound/citybound
- An open-source version of the game Transport Tycoon: https://github.com/OpenTTD/OpenTTD
