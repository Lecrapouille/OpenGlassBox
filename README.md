# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a personal implementation of Maxis SimCity 2013 simulation engine named GlassBox presented in this GDC conference 2012 [slides](http://www.andrewwillmott.com/talks/inside-glassbox).
Note: this project is neither a Maxis's released code source nor an affiliated project to Maxis but simply a portage from C#/Unity to C++14/SDL of the excellent job https://github.com/federicodangelo/MultiAgentSimulation but aged more than 8 years.

## Screenshot

Note: this figure may not refer to the latest development state, which also depends on the loaded simulation script.
![OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox/blob/master/doc/OpenGlassBox.png). Since I was more interested in the simulation than the rendering they are separated, and I used SDL2 and DearImGui were used to display the game states for the demonstration. Use your own rendering engine instead.

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

- Operating Systems: Linux, Mac OS X.
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

- Compile the project:
```sh
cd OpenGlassBox/
make download-external-libs
make CXX=g++ -j8
```
Where `-j8` is to adapt to match the number of CPU cores.

- Run OpenGlassBox:
```sh
./demo/build/OpenGlassBox
```

- (Optional) To install OpenGlassBox on your operating system:
```sh
sudo make install
```

- (Optional) Unit test with code coverage:
```sh
cd OpenGlassBox/test
make coverage -j8
```

## Notes concerning the portage

Here are the current changes made from the original code source:
- The original code was made in C#. Since I'm never developing in C# I made a portage into C++14.- Since I was more interested in the simulation than the rendering, dependencies to the game engine Unity and Decorator classes for using it have not been ported. For rendering the simulation, I'm not using the game engine and SDL2 is enough to draw lines and dots. Feel free to use your own rendering engine instead.- The original project did not come with unit tests or comments. Added!- The original project was using the same names as the GDC conference. I renamed classes whose name confused me:
  - `Box` is now named `City`.
  - `Point` and `Segment` are now named `Node` and `Way` (since will match more graph theory terms).
  - `ResourceBinCollection` is simply named `Resources`.
  - `SimulationDefinitionLoader` is now renamed `ScriptParser`.
- The original project did not implement `Area` class (aka `Zone`). `Area` manages `Units` (creation, upgrade, destruction). This also has to be added to this project.- A `Unit` shall be coupled to a `Node` of the `Path`. This is not particularly nice since this will create a lot of unnecessary graph nodes.- The original project implemented a dynamic A* algorithm in `Path::FindNextPoint`. I have created a `Dijkstra` class instead but a real traffic algorithm should have to be developed.
- Currently, I made a quick & dirty script straightforward parser. My code is less good than the original one. It was ok because I wished to replace the script syntax by [Forth](https://esp32.arduino-forth.com/) (which has less footprint than Lua).
## How to play?

- For the moment, you cannot construct your game interactively. A prebuild game is made in `demo/main.cpp` inside `bool GlassBox::initSimulation()` you can adapt it to create your own map.
- Simulation script is located at `OpenGlassBox/data/Simulations/TestCity.txt`.
- During the simulation, you can type the `d` key to show the debug window showing the internal states of the simulation.

## Ideas for the next?

- Import [OpenStreetMap](https://www.openstreetmap.org) maps.
- Implement some ideas explained in this video [Exploring SimCity: A Conscious Process of Discovery](https://youtu.be/eZfj7LEFT98).
- Update the dynamic A* algorithm to take into account the traffic flow instead of a shorter path.
- Parallelize algorithm: dispatch over all CPU cores (i.e. using OpenMP) or distribute computations (i.e. network, peer to peer).
- `Agent` should be directly attached to `Ways` and know the distance to the next `Agent`.- I wished to show and manipulate a SimCity city as a spreadsheet: insert/edit cells to add simulation rules.
## References

- Slides from the GDC conference can be downloaded here http://www.andrewwillmott.com/talks/inside-glassbox
- Since the video of this conference is no longer available, an alternative GDC conference video can be found here: https://youtu.be/eZfj7LEFT98
- A Scilab traffic assignment toolbox: https://www.rocq.inria.fr/metalau/ciudadsim and https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf For more information on this work, you can find other PDF at https://jpquadrat.github.io/ in section *Modélisation du Trafic Routier*
- A tutorial to make a city builder (more focused on rendering with the library SFML) https://www.binpress.com/creating-city-building-game-with-sfml/
- Moving cars: http://lo-th.github.io/root/traffic/ (code source https://github.com/lo-th/root/tree/gh-pages/traffic a fork based on https://github.com/volkhin/RoadTrafficSimulator)
- A work-in-progress, open-source, multi-player city simulation game: https://github.com/citybound/citybound
- An open-source version of the game Transport Tycoon: https://github.com/OpenTTD/OpenTTD
