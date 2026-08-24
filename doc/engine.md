# The engine, class by class

The GlassBox approach describes a city simulation as data rather than as a tree of objects with an `Update()` method. OpenGlassBox models the following class living in `include/OpenGlassBox` and is described there. The following sections explain what each class represents and why it exists separately.

- **Areas** are zones whose rules spawn, upgrade, remove units, and demolish buildings.
- **Maps** are 2D fields over the grid, such as water, pollution, desirability ...
- **Units** are buildings that hold bounded stocks of resources.
- **Agents** carry resources from one unit to another.
- **Resources** are money, goods, happiness ...
- **Paths** are networks made of nodes and segments on which agents travel.
- **Rules** move resources between them and define the gameplay.

## The two halves: recipes and things

Nothing in the engine is hard-coded, and that split runs through the whole design. A ruleset declares **recipes** : a kind of house, a kind of road, a kind of traveller and a running city holds **things** built from them. `Types.hpp` holds the recipes (`UnitType`, `WayType`, `AgentType`, `MapType`, `AreaType`, `PathType`); `Unit`, `Way`, `Agent`, `Map`, `Area` and `Path` are the things.

A thing keeps a `const&` to its recipe rather than a copy of it. A thousand houses share one `UnitType`, so a house costs what it actually holds and nothing more. The price of that is a lifetime rule which is worth remembering, because breaking it is the one way to make the engine misbehave in a way that is hard to debug: **the ruleset has to outlive every city loaded from it**. `Simulation` declares its `Script` before its `World` for exactly that reason, so that destruction, which runs in reverse, takes the cities away first.

### The containers: Simulation, World, City

| Class        | What it is                                                        | What it owns                     |
| ------------ | ----------------------------------------------------------------- | -------------------------------- |
| `Simulation` | The whole game, and where a program starts                        | the ruleset and the world        |
| `World`      | The ground: one grid, one calendar, the layers of the environment | the layers, the clock, the towns |
| `City`       | One town: its roads, buildings, travellers and zones              | everything a town is made of     |

`Simulation` also owns the conversion from wall time to game time. You hand `update()` the seconds elapsed since the last frame; it scales them by the speed the player chose, accumulates them, and runs as many **fixed ticks** as fit. A slow machine therefore falls behind rather than simulating differently, and a save made on one machine replays on another.

`World` exists because two towns that touch have to share their environment: pollution does not stop at a boundary. The layers live there, not in the towns, and what a `City` holds on the grid is the rectangle of cells it administers : a `MapRegion`. That region is what bounds every rule run on its behalf, so a town never reads over its neighbour's shoulder.

`World` also owns the **order of a tick**, which is not arbitrary: the towns move their agents and run their buildings first, then the layers run their own rules. Agents move before buildings look around so that a building sees who has just arrived, and the layers come last so that a cell reflects everything that happened during the tick.

## The road network: Path, Node, Way

A `Path` is a graph, and the only thing an `Agent` may travel on. `Node` is a vertex : a crossroads, and an address a building may sit on. `Way` is an edge : a street, undirected, since one-way traffic is not modelled.

A city may hold several `Path` objects and they never meet: a road network and a rail network are two of them, and the router never leaves the one it started on. That is the whole of the separation, and it is enforced in one place rather than checked everywhere.

A `Way` knows its length, how many agents are on it, and from those two how long it takes to drive. That travel time, not the length, is what the router minimises : see the traffic section below.

Both containers are `std::deque` rather than `std::vector`, and that is deliberate: everything refers to a crossroads or a street **by address**, so adding one must not move the others.

## The actors: Unit, Agent, Area

`Unit` is a building. It holds resources, it runs its rules once every so many ticks, and those rules send agents out. A `Unit` is **not** a graph node : a distinction worth dwelling on, because it is the one place this port departs most from the original. A building has its own position and, separately, an *anchor* on the network: a `Node`, or a `Way` at an offset. Anchoring along a street is what keeps the graph small; a street of forty houses used to become forty crossroads and forty-one segments, and the router paid for all of them at every tick.

`Agent` is one trip: a load of resources leaving a building and looking for another that will take it. Agents run no rules : a city may have a thousand alive at once : and they do not know where they are going. The router finds the cheapest building that answers to the name they are looking for *and has room*, and the answer changes as the traffic does. An agent that finds nothing wanders, and after `agentGiveUpTicks` it hands its load back to the building that sent it out. Wandering means the city is short of somewhere to deliver, not that the router is broken.

`Area` is a zone the player painted, and the piece that turns an empty grid into a town. It owns its footprint and nothing else: the buildings it grew belong to the `City`, which is why it counts them by walking the city rather than keeping a list of pointers that would outlive what they point at.

`Unit` and `Agent` share a base, `Entity<TYPE>`, holding the four things both have : an identifier, a recipe, a colour and a position. `Node` and `Way` deliberately do not inherit from it: they have no colour of their own and no identity outside the `Path` numbering them.

## The environment: Map

A `Map` is one layer: coal, water, forest, but also pollution, land value or desirability. Every cell holds an amount of that one thing, capped by the recipe of the layer, so nothing piles up without bound in one place.

A `Map` is what lets a rule ask a question about a *place* rather than about a building: "is there water within three cells of here?" A building reads and writes its neighbourhood through its **footprint**, a radius around the cell it stands on, and that radius is a taxicab distance : a diamond, not a circle. `MapCoordinatesInsideRadius` walks that diamond, reusing its buffers because a layer runs this for every cell of every town on every tick. `MapRandomCoordinates` is the other walker: it hands out a share of the cells of a region, drawn so that every subset of that size is equally likely, but scanned in reading order so that the cells come out in the order the grid stores them.

The grid is unbounded in the four directions, so cell coordinates are signed, and storage is sparse: blocks of 16×16 cells allocated the first time something is written into them. An empty layer costs nothing and a town can be founded anywhere without deciding on a size beforehand. `allocatedChunks()` makes the cost visible in the debug panel.

## The rules: Rule, RuleCommand, RuleValue

This is the scripting language at runtime, and it is smaller than you would expect.

- `IRule` is a name, a period, and a list of commands. `RuleUnit`, `RuleMap` and `RuleArea` add what each kind needs: a fallback for a building, a random sample of the cells for a layer, nothing for a zone.
- `IRuleCommand` is one line of a rule. Every command is asked **twice**: `validate()`, then `execute()`. A rule asks all of its commands first and applies them only if every one agreed, which is what makes a rule atomic : "take a person out of the house and put them in a car" either does both or does neither, so nobody is ever lost between the two.
- `IRuleValue` is somewhere a command reads a number from and writes one back to: `local`, `global` or `map`. Having them behind one interface is what lets a single `add` command serve all three : the command knows *how much*, the value knows *where*.
- `RuleContext` is everything a rule may look at while it runs: the town, the building or the cell, the clock, the resources. Rules hold no state of their own : they are shared by every entity that lists them : so all of the "where" has to be handed to them. The context is held by the entity and reused from tick to tick, since a layer rule fires over thousands of cells.

`IRule::Listener` is a single global observer of every attempt to run a rule, reporting **which command refused**. That is the answer to "why does this rule, which looks right, never do anything?", and it is what the Rule Log panel shows. While nobody is listening the whole feature costs one null pointer test.

## Time: SimulationClock

A tick means nothing on its own. `SimulationClock` turns a tick count into an hour of a day, so a rule can say `hour between 8 18` instead of counting ticks, and a building can be open or shut. The conversion is a runtime setting, `ticksPerMinute`, which is why a rule written as `rate 30 minutes` rescales with the settings instead of being left behind.

`OpeningHours` reads the `hour between` conditions of the rules of a building and aggregates them into a timetable. That is what the inspector and the map tooltip show as *open* or *closed*, and it is derived rather than declared: a building has opening hours because its rules keep them, and one whose rules keep none is open around the clock.

## Routing: IRouter and Dijkstra

`IRouter` is the interface the agents ask for an itinerary; `Dijkstra` is the implementation, and a `City` owns one so that its scratch buffers are allocated once instead of on every search. Two things make it more than a textbook shortest path, both covered in the traffic section: the cost of a street is its travel time under the current traffic, and there is no goal to aim at.

## Resources

`Resource` is a named stock with an amount and a capacity; `Resources` is a bag of them. Two resources are the same resource when their names match, which is what lets a script declare a new one without touching the engine. Lookup is a linear scan over a small vector, which beats a hash map at the handful of resources a building actually holds.

## Files and tooling

- There are two file formats, not three:
  - the `.ogs` for a ruleset: rule language, documented in the [simulation file reference](script.md);
  - the `.ogc` for a save: which stores a fingerprint of the ruleset it was made with and refuses to load against a different one; There is no separate "world" file. A save carries a fingerprint of the ruleset it was written against and the list of the types it uses, so opening it against a ruleset that has drifted tells you so instead of failing obscurely later.
- The script parser sits behind `IScriptParser`, so another front end can be plugged in without touching the engine.
- One window shows one city, but a `World` can hold several. A road crossing a border is clipped against the region of each city, and each piece belongs to the city it was clipped to, once `World::Listener` has allowed it. Routing stays inside one `Path`, so an agent never wanders into the neighbouring city on its own.
- The demo has undo and redo, a script editor that reparses in place, and an inspector that shows what a building holds and which rules act on it.

### Saving: CitySave

The `.ogc` format holds the state of a game: roads, buildings and what they hold, agents and where they are, the layers, the clock, and the traffic averages of the streets : the last so that a loaded town does not start with every road looking empty. What it does not hold is the rules, which stay in the `.ogs` next to it.

That split is why every entry point in `CitySave` deals with the ruleset too. A save stores a **fingerprint** of the ruleset it was written against and the names of every type it uses. Loading it into a different ruleset would rebuild the same geometry out of different recipes, giving a town that looks right and behaves like something else, so the loader refuses rather than warns. `peekHeader()` reads the header alone, which is how the demo loads the right ruleset without asking.

## Rulesets and saves

The gameplay is data-driven:

- a `.ogs` **ruleset** declares resources, networks, agents, maps, buildings, zones, and the rules that connect them;
- a `.ogc` **city save** stores geometry and live simulation state, while referring back to its ruleset.

Rules are atomic lists of tests and actions: either every command validates and the rule runs, or nothing changes. Buildings use `unitRule`, map layers use `mapRule`, and zones use `areaRule`.

For the complete syntax, command list, time periods, save format, and worked `test_city` example, read the [simulation file reference](demo/data/Simulations/README.md).

## How traffic is modelled

### Travel time on a road: the BPR function

Agents do not all take the shortest road: they take the *fastest* one, and a road gets slower as it fills up. The standard way to express that is the **BPR function**, named after the US **B**ureau of **P**ublic **R**oads that published it in 1964. It is the textbook formula of traffic assignment: the travel time of a road grows with the ratio of the traffic it carries to the traffic it was built for.

`Way::travelTime` implements it:

$$\displaystyle t(f) = t_0 \left(1 + 0.15 \left(\frac{f}{c}\right)^{\beta}\right)$$

- $t_0$ is the free flow travel time, the length of the road divided by its speed limit;
- $f$ is the flow, that is how many agents are on the road;
- $c$ is the capacity, the flow above which the road starts to jam;
- $\beta$ says how brutally it degrades past that point: with $\beta = 4$, twice the capacity costs about three and a half times the free flow time.

An empty road costs $t_0$. A road loaded exactly to its capacity costs 15 % more. `speed`, `capacity` and `beta` are properties of a `WayType`, so they are set in the `.ogs` ruleset.

A word on the units, because they are not the ones a traffic engineer would expect. Here $f$ is a **number of agents currently on the road**, not a number of vehicles per hour, and $c$ is the number of agents a street carries before it starts to feel it. Nothing forbids $f$ from exceeding $c$: a saturated street stays passable, it just becomes expensive, and that is precisely what makes the router send the next agent somewhere else instead of queueing everybody through the same place. Times are in seconds of game time, which the clock converts from ticks, so lengthening a tick does not make the city drive faster.

### Finding a destination: a search with no goal

The router is where those travel times are actually used, and it does two things a textbook shortest path does not.

The first is the cost: an edge costs its travel time under the current traffic, not its length. A short road carrying two hundred agents costs more than a long empty one.

The second is stranger, and it is the reason `Dijkstra` is not an A. **There is no goal.** An agent does not know which building it is going to; it knows the *name* of what it is looking for. The search walks the network outwards from the crossroads the agent stands at and stops at the first building that answers to that name **and has room for the load**. A building standing along a street is kept as a candidate rather than accepted at once, because a crossroads one hop away may hold a cheaper one.

Having no goal also means there is nothing for an A estimate to aim at. What is added to the cost of a crossroads is the free flow travel time back to the one the search started from, which biases the order of exploration towards the neighbourhood of the departure, where the nearest destination usually is. It is a speed-up, not an admissible heuristic: the answer is the cheapest building the search met first, which in an unusual geometry may not be the cheapest one there is. The `AStarRouter` alias is a leftover of the days when the search did have a goal.

Two consequences follow, and both are visible in the demo. Two agents leaving the same door a minute apart may be sent to different shops, because the traffic changed in between. And an agent whose itinerary was computed a while ago replaces it when the road it is on has become worse than the alternative.

Comparing the two costs a whole graph search, so it does not happen on every tick: `pathCheckTicks` in the Traffic panel is how often an agent bothers to look, and `cost deviation` is how much worse its road has to be before it switches. When the comparison does say the alternative is better, the itinerary that search produced is the one the agent takes : it is not searched for a second time. Lowering `pathCheckTicks` to one is the quickest way to bring a large city to its knees.

### Why the flow is smoothed

If routing used the instantaneous number of agents on each road, the whole population would swap between two parallel roads every tick: everyone sees road A empty, everyone moves to A, A is now jammed, everyone moves back to B. OpenGlassBox avoids that by feeding the BPR function an exponential moving average of the count $n$ of agents on the road, with a fixed weight $\alpha$ (0.05 by default, adjustable in the Traffic panel):

$$\displaystyle f \leftarrow (1-\alpha) f + \alpha n$$

This damps the oscillation, and that is all it does. It is deliberately **not** the solver used by traffic engineering tools such as CiudadSim, whose **MSA** (Method of Successive Averages) computes an all-or-nothing assignment $y^k$ onto the shortest paths at iteration $k$ and averages it in with a decreasing weight:

$$\displaystyle f^{k+1} = (1-\lambda_k) f^k + \lambda_k y^k,\qquad \lambda_k = \frac{1}{k}$$

Because $\lambda_k$ shrinks, MSA converges to a Wardrop equilibrium, where no agent can find a cheaper route. A fixed $\alpha$ does not converge to anything: it just keeps the picture of the network stable enough for the agents to make sensible decisions. This is a game, not an assignment solver.

### Reading the Traffic panel

The **relative gap** compares what the agents actually pay to what they would pay on the cheapest routes available at the current travel times:

$$\displaystyle \mathrm{Relgap} = \frac{\mathrm{TSTT} - \mathrm{SPTT}}{\mathrm{TSTT}}$$

TSTT is the total system travel time and SPTT the shortest path travel time. Near zero, the agents are already on their cheapest itineraries and the network has settled. Here it is a diagnostic you watch, not a stopping criterion of a solver.

Routing itself sits behind `IRouter` (`findRoute`, `shortestPathCost`), which a `City` owns, so another pathfinder can be dropped in without touching `Agent`.
