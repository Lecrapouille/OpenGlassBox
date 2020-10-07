# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a personal implementation from the slides of Maxis' SimCity 2013 simulation engine named GlassBox and shown in at this [GDC conference](http://www.andrewwillmott.com/talks/inside-glassbox). Note: this project is neither Maxis's released code source nor an affiliated project. This project is simply a portage from C#/Unity to C++11/SDL of https://github.com/federicodangelo/MultiAgentSimulation.
This current project is a sandbox: a preliminary work for its integration in my work-in-progress project SimTaDyn.

## Download, compilation and run

- Prerequisite: g++ or clang++, makefile, c++11, sdl2, bc (sudo apt-gety install).
Optionaly: googletest and gcov, if you wish to run unit tests and generate code coverage.

- Download source:
```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
```

- Compile the project:
```sh
cd OpenGlassBox/
make CXX=g++
```

- Run OpenGlassBox:
```sh
./build/OpenGlassBox
```

- Unit test, code coverage:
```sh
cd OpenGlassBox/test
make coverage
```

## Notes concerning the portage

- Dependencies to the game engine Unity has been removed. I also removed Listener classes (ie SimBoxListener).
- Since I'm more interested by the simulation than the game renderer, I'm not using game engine but for the moment SDL2 which is enough to draw lines and dots. SDL2 may be replaced by OpenGL or by SFML.
- The original project did not come with unit tests or comments. Added !
- The original project did not implement Area class (aka Zone). This is also has to be added. Currently Units is linked with a Node of the Path.
- From original project, using the same class names than the GDC conference, I renamed classes:
  - Box is now named City.
  - Point is now named Node.
  - Segment is now named Way.
  - ResourceBinCollection is now named Resources.
  - Unit to is now named ???.
  - SimulationDefinitionLoader is now renamed ScriptParser.
  - I added class Dstar (for Dynamic A*) instead of the method Path::FindNextPoint.
- Class coupling simplification from the initial code. Still in progress but Agent in the original code known by Node but not by Ways this can more difficult to compute flow. Still in progress D* algorithm is made inside the Node class will be made by a separated class. Unit are linked to a Node on a Path this is not great since that force splitting Ways to add Node and makes the graph grows up too much.
- Currently I made a quick & dirty script straightforward parser. I wish to replace it by Forth parser.
- Wish to Show SimCity as a spreadsheet: insert/edit cells to add simulation rules.

## References

- http://www.andrewwillmott.com/talks/inside-glassbox with some explanations here https://www.gamasutra.com/view/news/164870/GDC_2012_Breaking_down_SimCitys_Glassbox_engine.php or https://www.reddit.com/r/SimCity/comments/1a9fz6/how_the_glassbox_engine_works_based_on_the_gdc/
- https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf
- https://jpquadrat.github.io/ See section *Modélisation du Trafic Routier*
- https://www.binpress.com/creating-city-building-game-with-sfml/
- http://lo-th.github.io/root/traffic/ (code source https://github.com/lo-th/root/tree/gh-pages/traffic a fork based on https://github.com/volkhin/RoadTrafficSimulator)
- https://github.com/OpenTTD/OpenTTD
