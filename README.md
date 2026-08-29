# OpenGlassBox

**Note: I am also looking for a game developer or an artist able to turn this library into a real game.**

[OpenGlassBox](https://github.com/Lecrapouille/OpenGlassBox) is a C++17 city-simulation engine inspired by **GlassBox**, the engine behind Maxis's SimCity (2013). It is a port and extension of Federico D'Angelo's C#/Unity project, [MultiAgentSimulation](https://github.com/federicodangelo/MultiAgentSimulation), itself based on the [2012 GDC presentation](http://www.andrewwillmott.com/talks/inside-glassbox).

OpenGlassBox is an independent project: neither OpenGlassBox nor MultiAgentSimulation contains Maxis source code or is affiliated with Maxis.

The project provides:

- static and shared libraries for the simulation engine: they parse rulesets files, let you define gameplay through simple rules, and drive a dynamic city simulation;
- a standalone, SimCity-like 2D demo built on that engine.

## Documentation

| Document | Contents |
| -------- | -------- |
| [Improvements](doc/improvements.md) | Changes over MultiAgentSimulation. |
| [Installation](doc/installation.md) | Compile and install  OpenGlassBox. |
| [Engine](doc/engine.md) | Classes and tick order. |
| [Traffic](doc/traffic.md) | Travel times, congestion, and destination search. |
| [Economy](doc/economy.md) | What the ruleset does, and what a real economic model would add. |
| [Scripts](doc/script.md) | The `.ogs` language: the core of the project. |
| [Demo application](doc/demo.md) | Tools, panels, and keyboard shortcuts. |
| [Bundled simulations](demo/data/Simulations/README.md) | Sample rulesets and the test city walk through. |
| [Integration guide](doc/integration.md) | Link the library and drive a simulation from your own code. |

## Simulation engine and rulesets

The GlassBox approach describes a city simulation as data rather than as a tree of objects with an `Update()` method. OpenGlassBox models layers, buildings, agents, paths, zones, and the rules that connect them.

- **Zones** are zones whose rules spawn, upgrade, and demolish buildings.
- **Layers** are 2D grid fields with values, such as water, pollution,  desirability ...
- **Buildings** are buildings that hold bounded stocks of resources.
- **Agents** carry resources from one building to another.
- **Resources** are money, goods, happiness ...
- **Paths** are networks on which agents travel.

Gameplay is defined in rulesets files: resources, building types, agents, layers, zones, and the rules that move resources and grow cities. The C++ engine parse them and executes those rules; therefore, scripts and their parsing are the most important part of the project.

## Demo application

The engine library does not depend on the demo renderer, so it can be embedded in another application or connected to a different rendering engine.

This screenshot may not match the current state of the code for the demo application; what you see also depends on the loaded ruleset and city. The demo includes several bundled simulations, from the introductory `sandbox` city to traffic-focused road networks.

![OpenGlassBox](doc/OpenGlassBox.png)

One window shows one city. The simulation starts paused: press **Play** on the city toolbar or the space bar to start it. The **Simulation clock** panel steps the simulation tick by tick while paused and changes the speed from x0.25 to x16. Opening a ruleset (`.ogs`) starts from an empty city; a city save (`.ogc`) holds geometry, live state, and a hash of the ruleset it was built with. The **Inspector** panel shows details about any selected element; the left toolbar lets you edit the city.

## References

- GDC talk slides: [http://www.andrewwillmott.com/talks/inside-glassbox](http://www.andrewwillmott.com/talks/inside-glassbox)
- Since the original conference video is no longer available, an alternative GDC talk can be found here: [https://youtu.be/eZfj7LEFT98](https://youtu.be/eZfj7LEFT98)
- A Scilab traffic assignment toolbox: [https://www.rocq.inria.fr/metalau/ciudadsim](https://www.rocq.inria.fr/metalau/ciudadsim) and [https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf](https://www.rocq.inria.fr/metalau/ciudadsim/ftp/CS5/manual/manual.pdf). For more information, see other PDFs at [https://jpquadrat.github.io/](https://jpquadrat.github.io/) in the section *Modélisation du Trafic Routier*.
- A tutorial for building a city builder (focused on rendering with SFML): [https://www.binpress.com/creating-city-building-game-with-sfml/](https://www.binpress.com/creating-city-building-game-with-sfml/)
- Moving cars: [http://lo-th.github.io/root/traffic/](http://lo-th.github.io/root/traffic/) (source code: [https://github.com/lo-th/root/tree/gh-pages/traffic](https://github.com/lo-th/root/tree/gh-pages/traffic), a fork of [https://github.com/volkhin/RoadTrafficSimulator](https://github.com/volkhin/RoadTrafficSimulator))
- A work-in-progress, open-source, multiplayer city simulation game: [https://github.com/citybound/citybound](https://github.com/citybound/citybound)
- An open-source version of Transport Tycoon: [https://github.com/OpenTTD/OpenTTD](https://github.com/OpenTTD/OpenTTD)
