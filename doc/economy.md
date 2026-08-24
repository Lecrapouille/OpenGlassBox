# The economy

There is no economic model in OpenGlassBox. This document says what exists instead, why the gap matters, and what the missing pieces would look like if they were written. Everything past the first section is a **proposal, not a description of the code**.

It is a companion to the [traffic documentation](traffic.md), and the parallel is not decorative: the two systems have the same shape. Both spread a demand over a set of candidates whose attractiveness depends on how many others already chose them, and both are wrong in the same way when they ignore that feedback.

## What exists today

Production is entirely scripted. A building runs its rules once every so many ticks, and a rule that produces something is a rule that adds to a stock:

```text
unitRule ProduceGoods
    rate 30 minutes
    hour between 8 18
    local People greater 0
    local Goods add 1
    map Pollution add 1
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

Once output varies, a fixed sale price stops making sense. The price of a resource in an area would follow the imbalance between demand and available stock:

$$\displaystyle p = p_0 \cdot \left(\frac{D}{S + \epsilon}\right)^{\gamma}$$

- $D$ is the local demand and $S$ the available stock;
- $p_0$ is the reference price, the one at equilibrium $D = S$;
- $\gamma$ is an elasticity exponent, typically between 0.5 and 1: the larger it is, the more violently the price reacts to an imbalance;
- $\epsilon$ is a small constant keeping the division finite when $S = 0$.

A `Map` layer is the natural home for $D$ and $S$, since both are questions about a place rather than about a building, and the neighbourhood sum a layer already performs is exactly the aggregation needed.

## Choosing a destination: `AssignmentPolicy`

**Not implemented.** This is the gap that matters most, because unlike the others it degrades behaviour the player can see.

An agent looking for somewhere to deliver stops at the first building that will take its load. `Dijkstra::findRoute` walks the network outwards and returns as soon as `Unit::accepts` says yes, which asks two questions: does the building answer to this name, and does it have room.

Room is a boolean. A shop missing one good and a shop missing twenty are indistinguishable, so the near one is filled first and the far one waits, however badly it needs stock. First come, first saturated.

That is the economic twin of the all-or-nothing assignment described in the [traffic documentation](traffic.md#all-or-nothing), and it has the same cause: a choice made without regard to how many others are making it. The fix has the same shape too. Replace the "first acceptable" test with a **weighted score of need against distance**:

$$\displaystyle \text{score}(u) = \frac{\text{need}(u)}{1 + d(u)^{\delta}}$$

- $\text{need}(u)$ is how much the building is short of, so an empty shop outbids a nearly full one;
- $d(u)$ is the distance, or better the travel time the router already computed;
- $\delta$ tunes how much proximity outweighs need. A large $\delta$ recovers roughly the current behaviour.

The agent takes the highest-scoring reachable building instead of the first one found, which spreads the load over equivalent destinations on its own.

`AssignmentPolicy` would be that criterion, extracted so that the router asks it rather than hard-coding the test. Two things make this more delicate than it looks, and both should be settled before writing it:

- the search currently **terminates** on the first accepting building at a crossroads, which is correct only because any further building costs strictly more. A score that can rise with distance removes that guarantee, so the search has to be bounded some other way, by a radius or by a candidate budget.
- `Unit::accepts` lives in the engine while `Dijkstra` lives in the demo, so the policy has to sit on the engine side of that line to be usable by another router.

## Smoothing the indicators

**Not implemented.**

Economic feedback loops run away in a few ticks when they act on raw measurements: a tax rise empties a district, the district emptying collapses the revenue, the collapse triggers another adjustment. Player-facing indicators, whether treasury, employment or satisfaction, want to be smoothed:

$$\displaystyle I_t = I_{t-1} + \eta \left(I_{\text{observed}} - I_{t-1}\right)$$

This is the **same** exponential moving average that `Way::smoothFlow` already applies to traffic, with the same fixed step and for the same reason. In both cases a noisy signal, an instantaneous vehicle count or a raw economic reading, is turned into a stable trend that decisions can be based on, without recomputing a global optimum at every tick.

That is worth stating plainly because it is an argument for writing it once. The engine has one smoothing concept, not two.

The historical parallel holds here as well. The SimCity (2013) launch bug was not only about traffic: agents had no lasting attachment to their home or their job, so the smallest local disturbance set off a cascade of moves. The correction was the same principle as MSA, stabilising by inertia rather than recomputing an optimum every tick.

## What goes where

| Component | Kind of computation | Where it belongs |
| --- | --- | --- |
| Shortest route per tick | Local, cheap | C++, exists: `IRouter` and `Dijkstra` |
| Congestion smoothing | Global, persistent state | C++, exists: `Way::smoothFlow`; see `TrafficAssignmentSolver` in the [traffic documentation](traffic.md#if-we-wanted-a-real-solver-trafficassignmentsolver) |
| Production function | Local, per `Unit` | C++, proposed: `ProductionModel` |
| Agent to building assignment | Local, cheap | C++, proposed: `AssignmentPolicy` |
| Smoothing of macro indicators | Global, persistent state | C++, proposed, reusing the traffic moving average |
| Simple growth rules: thresholds, timers | Local, no complex state | Script: `areaRule`, as at Maxis |

## Where the coefficients would live

The exponents $\alpha$, $\gamma$, $\delta$ and the step $\eta$ belong with the coefficients that already exist, not in new script keywords. `SimulationConfig` in `include/OpenGlassBox/Config.hpp` holds the ones that tune the whole simulation, next to `trafficSmoothing`; the recipe structures in `include/OpenGlassBox/Types.hpp` hold the ones that vary per kind of building, next to the `capacity` and `beta` of a `WayType`. Recipes and rates stay in the `.ogs` ruleset, where they already are.

The `.ogs` grammar does not need to grow for any of this. See the [script language reference](script.md) and the [engine documentation](engine.md).
