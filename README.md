# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an open source implementation of the Maxis' SimCity 2013 simulation engine named GlassBox. Note: this project is not affiliated to Maxis Software but is a portage of
https://github.com/federicodangelo/MultiAgentSimulation from C#/Unity to C++11/SDL. The original
project was inspired by the [GDC presentation given by Maxis](http://www.andrewwillmott.com/talks/inside-glassbox).
This current project is a sandbox, a preliminary work for an integration in my project SimTaDyn.

## Axe of Developement

Done / wishes / work in progress:
- No dependency with the game enine Unity. Is currently replaced by SDL but ca be replaced by OpenGL or something else.
- Original project did not come with unit tests or comments.
- Rename Class with less generic name: Box is now City, Segment is now Way, Unit to xxx.
- Class coupling simplification from the initial code. Still in progress but Agent in the original code known by Node but not by Ways this can more difficult to compute flow. Still in progress D* algorithm is made inside the Node class will be made by a separated class. Unit are linked to a Node on a Path this is not great since that force splitting Ways to add Node and makes the graph grows up too much.
- Todo: Area class (aka Zone) is not made.
- Currently I made a quick & dirty script parser. I wish to replace it by Forth parser.
- Wish to Show SimCity as a spreadsheet: insert/edit cells to add simulation rules.

## Download, compilation and run

- Prerequisite: g++ or clang++, makefile, c++11, sdl2. Optional: bc, googletest.

- Download source:
```sh
git clone https://github.com/Lecrapouille/OpenGlassBox.git --recursive
```

- Compile the project:
```sh
cd OpenGlassBox
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

## References

- http://www.andrewwillmott.com/talks/inside-glassbox with some explanations here https://www.gamasutra.com/view/news/164870/GDC_2012_Breaking_down_SimCitys_Glassbox_engine.php or https://www.reddit.com/r/SimCity/comments/1a9fz6/how_the_glassbox_engine_works_based_on_the_gdc/
- https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf
- https://jpquadrat.github.io/ See section *Modélisation du Trafic Routier*
- https://www.binpress.com/creating-city-building-game-with-sfml/
- http://lo-th.github.io/root/traffic/ (code source https://github.com/lo-th/root/tree/gh-pages/traffic a fork based on https://github.com/volkhin/RoadTrafficSimulator)
- https://github.com/OpenTTD/OpenTTD
