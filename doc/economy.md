# The economy

There is no economic model in OpenGlassBox. This document says what exists instead, why the gap matters, and what the missing pieces would look like if they were written. Each section says plainly whether it describes the code or proposes something: production and prices are **proposals**, the choice of a destination and the smoothing of the curves **describe what is there**.

It is a companion to the [traffic documentation](traffic.md), and the parallel is not decorative: the two systems have the same shape. Both spread a demand over a set of candidates whose attractiveness depends on how many others already chose them, and both go wrong in the same way when they ignore that feedback. Traffic answers it with the BPR curve, which makes a street dearer as it fills; the choice of a destination answers it with a claim on the building, which is the section below.

## What exists today

Production is entirely scripted. A building runs its rules once every so many ticks, and a rule that produces something is a rule that adds to a stock:

```text
unitRule ProduceGoods
    rate 30 minutes
    hour between 8 18
    local People greater 0
    local Goods add 1
    layer Pollution add 1
    global Money add 1
end
```

`Unit::executeRules` fires it on its period, and `RuleUnit::execute` applies it atomically: every command is asked to `validate()` first, and none is applied unless all agreed. So the factory either produces a good, pollutes its neighbourhood and pays the treasury, or does none of the three.

That is the whole economy, and its properties are worth naming:

- **Production is a constant.** One good per thirty minutes, whatever the factory holds beyond the one person the rule tests for. Twelve workers produce exactly what one produces.
- **There is no price.** `Money` is a resource like `Goods` or `People`, declared in the script and summed into `City::globals()`. The two Money a shop earns per sale is a literal in the ruleset.
- **There is no capital, no wage, no tax.** "Work" is an `Agent` of type `Worker` carrying one `People` to a building that answers to the name `Work`.
- **Nothing arbitrates between buildings.** A shop that has been out of stock for an hour is no more attractive to the next truck than the full shop next door.

None of this is an oversight for its own sake. The GlassBox approach puts gameplay in data, and a constant rate is legible in a way a production function is not. The sections below are about the places where that legibility costs too much.

## Production: `ProductionModel`

**Not implemented.**

A constant rate breaks down as soon as a building is allowed to grow. Doubling the workforce should not double the output for ever, or a city converges on one enormous factory. The standard shape with diminishing returns is the **Cobb-Douglas** production function:

$$\displaystyle Q = A \cdot L^{\alpha} \cdot K^{1-\alpha}$$

- $Q$ is the quantity produced;
- $L$ is the labour available;
- $K$ is the capital, or the raw material;
- $A$ is total factor productivity, which may depend on technology or on urban density;
- $\alpha \in [0,1]$ weighs labour against capital.

The exponents summing to one is what gives **constant returns to scale**: doubling both $L$ and $K$ doubles $Q$ exactly, which keeps the behaviour of the economy predictable as the city grows.

A first implementation does not need the power. A linear form bounded by the scarcest input reads better on screen and is easier to tune:

$$\displaystyle Q = \min\!\left(L, \frac{K}{k_{req}}\right) \cdot \text{rate}$$

where $k_{req}$ is the raw material needed per worker. The player sees immediately which input is missing, with no exponent to interpret.

`ProductionModel` would be the class holding that choice, consulted by a rule command rather than replacing one: the script would keep saying what a factory turns into what, and the model would decide how much. Its inputs are the stocks the `Unit` already holds, so it needs no new data on the entity.

## Prices: local supply and demand

**Not implemented.**

Once output varies, a fixed sale price stops making sense. The price of a resource in a zone would follow the imbalance between demand and available stock:

$$\displaystyle p = p_0 \cdot \left(\frac{D}{S + \epsilon}\right)^{\gamma}$$

- $D$ is the local demand and $S$ the available stock;
- $p_0$ is the reference price, the one at equilibrium $D = S$;
- $\gamma$ is an elasticity exponent, typically between 0.5 and 1: the larger it is, the more violently the price reacts to an imbalance;
- $\epsilon$ is a small constant keeping the division finite when $S = 0$.

A `Layer` is the natural home for $D$ and $S$, since both are questions about a place rather than about a building, and the neighbourhood sum a layer already performs is exactly the aggregation needed.

## Choosing a destination

This was the gap that mattered most, because unlike the others it degraded behaviour the player could see. It is the one thing on this page that has been fixed.

An agent looking for somewhere to deliver stops at the first building that will take its load. `Dijkstra::findRoute` walks the network outwards and returns as soon as `Unit::accepts` says yes, which asks two questions: does the building answer to this name, and does it have room.

Room used to be a plain boolean on the stock. That made twenty agents dispatched on the same tick all see the same single free slot and all be sent to claim it, and nineteen of them arrive to find it taken. It is the economic twin of the all-or-nothing assignment described in the [traffic documentation](traffic.md#aon-all-or-nothing), with the same cause: a choice made without regard to how many others are making it at the same moment.

### What was done: reserving the place

A `Unit` now counts the agents heading towards it, and `accepts` measures the room against the stock **plus** that count. An agent claims its place when it is routed and gives it back when it delivers, when it gives up, when it is destroyed, or when it merely recomputes its itinerary and picks somewhere else. `Agent::route` is the single point all of that goes through, which is what makes the claim impossible to leak: a building whose count never came back down would be invisible to every agent for the rest of the game, and that would be worse than the crowding it prevents. A test asserts the invariant over a whole city, that the claims outstanding equal the agents that have somewhere to go.

Two details are worth knowing. The claim is against the other agents and not against oneself, so an agent lifts its own for the length of its own tick; otherwise it would find its own destination full and never be let in at the door it was sent to. And nothing of this is saved: itineraries are recomputed on loading, so the counts rebuild themselves and the `.ogc` format does not move.

The cost is one integer per building and one comparison per acceptance test.

### What was not done: scoring need against distance

The refinement, still unwritten, is to arbitrate rather than take the first acceptable building. Replace the boolean with a **weighted score of need against distance**:

$$\displaystyle \text{score}(u) = \frac{\text{need}(u)}{1 + d(u)^{\delta}}$$

- $\text{need}(u)$ is how much the building is short of, so an empty shop outbids a nearly full one;
- $d(u)$ is the distance, or better the travel time the router already computed;
- $\delta$ tunes how much proximity outweighs need. A large $\delta$ recovers roughly the current behaviour.

It is deliberately postponed, and the reason is the cost. The search **terminates** on the first accepting building, which is correct only because anything further out costs strictly more. A score that can rise with distance removes that guarantee: the search would have to carry on through a tolerance window, and widening the radius by a quarter is about half as many nodes again, on every itinerary of every agent. Against that, once the place is reserved what is left to arbitrate is the choice between a nearly full building close by and an empty one a little further, which is a preference rather than a defect.

There is also a boundary to respect whenever it is written: `Unit::accepts` lives in the engine while `Dijkstra` lives in the demo, so the criterion has to sit on the engine side of that line to be usable by another router.

## Smoothing the indicators

**Not in the engine, and on purpose.**

Population and money move in steps, a rule firing at a time, and a curve of steps is hard to read a trend off. So the demo keeps an exponential moving average of every series it samples and draws it next to the raw one, behind the *Trend* toggle of the Charts panel:

$$\displaystyle I_t = I_{t-1} + \eta \left(I_{\text{observed}} - I_{t-1}\right)$$

That lives in `demo/src/Game/TimeSeries.hpp`. It is the **same** filter `Segment::smoothFlow` applies to traffic, with the same fixed step, but it is not the same thing: the traffic average is read back by the router and changes what the agents do, whereas this one is read by nobody but the plotting code.

That distinction is why it stayed out of the engine. A smoothed indicator earns its place there when something reads it and acts on it, which is the classical argument for smoothing: a tax rise empties a district, the district emptying collapses the revenue, the collapse triggers another adjustment. There is no tax and no rule that reads an indicator, so today the benefit is legibility and nothing more. The day the first consumer appears, the average moves into the engine along with it.

The historical parallel is still worth keeping in view. The SimCity (2013) launch bug was not only about traffic: agents had no lasting attachment to their home or their job, so the smallest local disturbance set off a cascade of moves. The correction was the same principle as MSA, stabilising by inertia rather than recomputing an optimum every tick.

## What goes where

| Component | Kind of computation | Where it belongs |
| --- | --- | --- |
| Shortest route per tick | Local, cheap | C++, exists: `IRouter` and `Dijkstra` |
| Congestion smoothing | Global, persistent state | C++, exists: `Segment::smoothFlow`; see [why there is no solver](traffic.md#why-there-is-no-assignment-solver) |
| Reserving a place at the destination | Local, one counter per `Unit` | C++, exists: `Unit::reserve` and `Agent::route` |
| Production function | Local, per `Unit` | C++, proposed: `ProductionModel` |
| Scoring need against distance | Widens every search | Postponed, see above |
| Smoothing of the plotted curves | Per series, in the panel | C++, exists: `game::TimeSeries`, demo only |
| Simple growth rules: thresholds, timers | Local, no complex state | Script: `zoneRule`, as at Maxis |

## Where the coefficients would live

The exponents $\alpha$, $\gamma$ and $\delta$ belong with the coefficients that already exist, not in new script keywords. `ogb::Config` in `include/OpenGlassBox/Config.hpp` holds the ones that tune the whole simulation, next to `TrafficConfig::smoothing`; the recipe structures in `include/OpenGlassBox/Types.hpp` hold the ones that vary per kind of building, next to the `capacity` and `beta` of a `SegmentType`. Recipes and rates stay in the `.ogs` ruleset, where they already are.

The step $\eta$ of the smoothing is not in that list, because the smoothing is not in the engine: it is `TimeSeries::SMOOTHING` in the demo. Should an indicator ever be read by a rule, its step moves to `ogb::Config` at the same time as the indicator itself.

The `.ogs` grammar does not need to grow for any of this. See the [script language reference](script.md) and the [engine documentation](engine.md).
