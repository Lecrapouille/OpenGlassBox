# OpenGlassBox

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is an open source implementation of the Maxis' SimCity 2013 simulation engine named GlassBox. Note: this project is not affiliated to Maxis Software but is a portage of
https://github.com/federicodangelo/MultiAgentSimulation from C#/Unity to C++11/SDL. The original
project was inspired by the [GDC presentation given by Maxis](http://www.andrewwillmott.com/talks/inside-glassbox).
This current project is a sandbox, a preliminary work for an integration in my project SimTaDyn.

## Axe of Developement

Done / wishes / work in progress:
- No dependency with the game enine Unity.
- Class coupling simplification from the initial code.
- Replacement of the home-made script by Forth script.
- SimCity spreadsheet: insert/edit cells to add simulation rules.

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
