# How traffic is modelled

Traffic assignment is an old field with a settled vocabulary, and OpenGlassBox borrows from it without pretending to be one of its tools. This document has two halves. The first states the classical theory: how a road gets slower, and the family of algorithms that decide who drives where. The second says what the engine actually does, which formula lives in which method, and where it departs from the textbook on purpose.

The reference implementation of the theory is [CiudadSim](https://www.rocq.inria.fr/metalau/ciudadsim), a Scilab toolbox for traffic assignment; the notation below follows it.

## Notation

The road network is a directed graph $G = (N, A)$, where $N$ is the set of nodes (crossroads) and $A$ the set of links $a$ (street segments).

Travel demand is an **origin-destination matrix**: $d_{ij}$ is the number of trips wanting to go from node $i$ to node $j$ during the period considered.

For a pair $(i,j)$ there is a set of possible routes $K_{ij}$. Writing $f_k$ for the flow assigned to route $k$, the flow on a link is the sum of the flows of every route using it:

$$\displaystyle x_a = \sum_{(i,j)} \sum_{k \in K_{ij}} f_k \, \delta_{a,k}, \qquad \delta_{a,k} = \begin{cases} 1 & \text{if route } k \text{ uses link } a \\ 0 & \text{otherwise} \end{cases}$$

Every algorithm below answers the same question: how to spread $d_{ij}$ over the routes of $K_{ij}$, knowing that the cost of a route depends on the flow it carries. That circularity is the whole difficulty. The flow determines the cost, the cost determines the flow, and what is wanted is a fixed point of the two.

## Travel time on a road: the BPR function

Each link $a$ has a **performance function** $t_a(x_a)$ giving its travel time as a function of the flow it carries. The one used in practice comes from the US **B**ureau of **P**ublic **R**oads, which published it in 1964, and is known as the **BPR function**:

$$\displaystyle t_a(x_a) = t_a^0 \left(1 + \alpha \left(\frac{x_a}{c_a}\right)^{\beta}\right)$$

- $t_a^0$ is the free flow travel time, on an empty road;
- $c_a$ is the practical capacity of the link, the flow above which it starts to jam;
- $\alpha, \beta$ are shape parameters, standard values being $\alpha = 0.15$ and $\beta = 4$;
- $x_a$ is the current flow.

While $x_a \ll c_a$ the penalty term is near zero and $t_a \approx t_a^0$. As $x_a$ approaches and passes $c_a$, the exponent $\beta = 4$ makes the term explode. That brutality is the point: a jam does not build up linearly, it appears.

### What OpenGlassBox does

`Way::updateTravelTime`, in `src/Path.cpp`, is the BPR function, with $\alpha$ fixed at 0.15 in the code:

```cpp
void Way::updateTravelTime()
{
    if (m_type.capacity <= 0.0f)
    {
        m_travelTime = m_t0;
        return;
    }

    float const x = m_flow / m_type.capacity;

    m_travelTime = m_t0 * (1.0f + 0.15f * std::pow(x, m_type.beta));
}
```

$t_a^0$ is computed once per segment by `Way::updateMagnitude`, as the length of the road divided by the speed limit of its type. $c_a$ and $\beta$ are properties of a `WayType`, so a ruleset sets them per kind of road:

```text
segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
```

A word on the units, because they are not the ones a traffic engineer would expect. Here the flow is a **number of agents currently on the road**, not a number of vehicles per hour, and the capacity is the number of agents a street carries before it starts to feel it. Nothing forbids the flow from exceeding the capacity: a saturated street stays passable, it just becomes expensive, and that is precisely what makes the router send the next agent somewhere else instead of queueing everybody through the same place. Times are in seconds of game time, which the clock converts from ticks, so lengthening a tick does not make the city drive faster.

An empty road costs $t_a^0$. A road loaded exactly to its capacity costs 15 % more. Twice its capacity costs about three and a half times the free flow time.

## AON: All-or-nothing

The simplest assignment, and the one a textbook Dijkstra or A\* performs without knowing it. For each pair $(i,j)$, compute the shortest route under the **current** costs $t_a$, and put **all** of the demand on it:

$$\displaystyle x_a = \sum_{(i,j)} d_{ij} \cdot \mathbb{1}\!\left[a \in \text{shortest route}(i,j)\right]$$

The structural flaw is that the cost used to find the route ignores the flow that is about to use it. Everybody runs the same computation against the same costs, so everybody converges on the same road, which saturates, while a slightly longer and completely empty alternative sits next to it.

This is the historical bug of SimCity (2013). Stone Librande of Maxis confirmed it publicly at the time: the agents insisted on the shortest possible route to each destination, causing enormous jams even when other roads were free. It was all-or-nothing, run once per agent, with no feedback from the flow.

## Frank-Wolfe and the Wardrop equilibrium

Wardrop's first principle (1952) defines the stable state of a traffic system: **no user can improve by unilaterally changing route**. Every used route between a pair $(i,j)$ has the same cost, and that cost is no greater than the cost of any unused route.

This is equivalent to the convex program known as the Beckmann formulation:

$$\displaystyle \min_{x} \; Z(x) = \sum_{a \in A} \int_0^{x_a} t_a(s)\, ds$$

subject to flow conservation, meaning the demand $d_{ij}$ is entirely carried, and $x_a \geq 0$.

Why an integral rather than the total cost $\sum_a t_a(x_a) \, x_a$? Because the objective is not the current total cost but a function whose gradient is exactly $t_a(x_a)$, and it is that property which makes the minimum of $Z$ coincide with the Wardrop equilibrium. Each user minimising their own trip happens to minimise this collective integral. It is a classical result, not a sum of costs.

Frank-Wolfe solves it by iterating, each iteration reusing an all-or-nothing:

1. Initialise $x^0$ with an all-or-nothing on the free flow costs $t_a^0$.
2. At iteration $n$: compute the current costs $t_a(x_a^n)$; run an all-or-nothing against those costs to get a target direction $y^n$; find the step $\lambda_n \in [0,1]$ minimising $Z(x^n + \lambda(y^n - x^n))$, a one-dimensional line search since $Z$ is convex along that segment; and set $x^{n+1} = x^n + \lambda_n (y^n - x^n)$.
3. Repeat until the iterates stop moving.

Geometrically, each iteration nudges the current solution towards the best all-or-nothing direction available at that moment. Because $Z$ is convex, the sequence converges to the unique Wardrop equilibrium.

The line search assumes the demand $d_{ij}$ stays fixed for the whole computation, which makes Frank-Wolfe an **offline** method: precomputing a map, planning an infrastructure. It is not something to run inside a simulation whose demand changes at every tick.

## MSA: method of successive averages

The **method of successive averages** keeps the structure of Frank-Wolfe, an all-or-nothing per iteration blended into the previous solution, and drops the line search. The step is decided in advance, from the iteration number alone:

$$\displaystyle \lambda_n = \frac{1}{n}, \qquad x^{n+1} = x^n + \frac{1}{n}\left(y^n - x^n\right)$$

Expanding the recurrence shows that $x^n$ is the running arithmetic mean of every all-or-nothing computed so far:

$$\displaystyle x^{n} = \frac{1}{n}\sum_{k=1}^{n} y^k$$

Hence the name. As $n$ grows, an individual all-or-nothing carries less weight and the solution settles. MSA reaches the same Wardrop equilibrium as Frank-Wolfe, generally more slowly since $1/n$ is not the optimal step, but it is far simpler and, more importantly, it never assumes the demand is frozen.

## What OpenGlassBox does instead

The key simplification is that OpenGlassBox does not have to simulate a flow, because the flow is already there. In CiudadSim, $x_a$ is an aggregate quantity the solver computes. Here it is the `Agent` objects physically crossing the `Way` objects one by one; `Way::addAgent` and `Way::removeAgent` merely count them.

So there is no assignment loop. Each agent, when it routes, runs its own all-or-nothing against the travel times of the moment, and the flow that results is observed rather than solved for.

### Smoothing, and why it is not MSA

Routing on the instantaneous count would make the whole population swap between two parallel roads every tick: everyone sees road A empty, everyone moves to A, A is now jammed, everyone moves back to B. The BPR function is therefore fed an **exponential moving average** of the count $n$ of agents on the road, with a fixed weight $\alpha$:

$$\displaystyle f \leftarrow (1-\alpha) f + \alpha \, n$$

That is `Way::smoothFlow`, called once per tick for every segment from `City::update` through `Path::smoothFlows`, before any agent moves, so that all of them route on the same picture of the network during the tick. The weight is `SimulationConfig::trafficSmoothing`, 0.05 by default and adjustable in the Traffic panel of the demo. Segments whose flow barely moved are skipped, which spares the BPR power on every quiet street of the city on every tick.

The resemblance to MSA is real but superficial, and worth being precise about, because the two differ in exactly one term. MSA averages with a **decreasing** weight $\lambda_k = 1/k$:

$$\displaystyle f^{k+1} = (1-\lambda_k) f^k + \lambda_k y^k, \qquad \lambda_k = \frac{1}{k}$$

Because $\lambda_k$ shrinks, MSA converges to a Wardrop equilibrium. A fixed $\alpha$ converges to nothing: it is a low-pass filter, not a solver. It damps the oscillation and that is all it does.

That is a choice, not an approximation. A vanishing step is right for a **static** demand, where the system is supposed to freeze onto the exact equilibrium. A city is not static: a district appears, a road is demolished, a rush hour starts. A fixed step keeps the network responsive for ever, at the price of never settling on an equilibrium it would have been wrong to settle on. This is a game, not an assignment solver.

The same exponential moving average is the mechanism proposed for smoothing macroeconomic indicators; see the [economy documentation](economy.md).

### Finding a destination: a search with no goal

The router is where those travel times are used, and it does two things a textbook shortest path does not.

The first is the cost: an edge costs its travel time under the current traffic, not its length. A short road carrying two hundred agents costs more than a long empty one.

The second is stranger, and it is the reason `Dijkstra` is not an A\*. **There is no goal.** An agent does not know which building it is going to; it knows the *name* of what it is looking for. `Dijkstra::findRoute` walks the network outwards from the crossroads the agent stands at and stops at the first building that answers to that name **and has room for the load**, which `Unit::accepts` decides. A building standing along a street is kept as a candidate rather than accepted at once, because a crossroads one hop away may hold a cheaper one.

Having no goal also means there is nothing for an A\* estimate to aim at. What is added to the cost of a crossroads is the free flow travel time back to the one the search started from, which biases the order of exploration towards the neighbourhood of the departure, where the nearest destination usually is. It is a speed-up, not an admissible heuristic: the answer is the cheapest building the search met first, which in an unusual geometry may not be the cheapest one there is. The `AStarRouter` alias is a leftover of the days when the search did have a goal.

Two consequences follow, and both are visible in the demo. Two agents leaving the same door a minute apart may be sent to different shops, because the traffic changed in between. And an agent whose itinerary was computed a while ago replaces it when the road it is on has become worse than the alternative.

Comparing the two costs a whole graph search, so it does not happen on every tick: `pathCheckTicks` is how often an agent bothers to look, and `pathCostDeviation` is how much worse its road has to be before it switches. When the comparison does say the alternative is better, the itinerary that search produced is the one the agent takes: it is not searched for a second time. Lowering `pathCheckTicks` to one is the quickest way to bring a large city to its knees.

Note that this "first acceptable building" criterion is the destination-choice counterpart of all-or-nothing, and it has the same weakness. It is discussed, with a proposed fix, in the [economy documentation](economy.md#choosing-a-destination-assignmentpolicy).

### Reading the relative gap

Since the engine never solves for an equilibrium, it measures how far it is from one. The **relative gap** compares what the agents actually pay against what they would pay on the cheapest routes available at the current travel times:

$$\displaystyle \mathrm{Relgap} = \frac{\mathrm{TSTT} - \mathrm{SPTT}}{\mathrm{TSTT}}$$

TSTT is the total system travel time and SPTT the shortest path travel time. `Simulation::relativeGap` sums the remaining cost of every agent of every city for the first, and asks the router for `shortestPathCost` from where each agent stands for the second. Near zero, the agents are already on their cheapest itineraries and the network has settled. It is a diagnostic to watch in the Traffic panel, not the stopping criterion of a solver.

## What we deliberately do not do

All of the above, all-or-nothing included, assumes that every user of a pair $(i,j)$ knows the network perfectly and picks the minimum-cost route. Real drivers have an imperfect and heterogeneous perception of costs: habit, ignorance, personal preference.

The **logit** model spreads the demand over the routes of $K_{ij}$ with a probability decreasing in their cost, rather than putting all of it on the cheapest:

$$\displaystyle P_k = \frac{e^{-\theta t_k}}{\displaystyle\sum_{k' \in K_{ij}} e^{-\theta t_{k'}}}$$

where $\theta > 0$ is a sensitivity parameter. As $\theta \to \infty$ the logit degenerates into all-or-nothing; as $\theta \to 0$ the split becomes uniform.

The **probit** model instead assumes each user perceives a cost $\tilde{t}_k = t_k + \varepsilon_k$ with $\varepsilon_k \sim \mathcal{N}(0, \sigma^2)$, so that $P_k = \Pr(\tilde{t}_k \leq \tilde{t}_{k'}, \forall k' \neq k)$. It has no closed form and needs numerical integration or a Monte-Carlo simulation.

**Dial** (1971) and **Bell** (1995) are what make a logit assignment tractable on a graph without enumerating the routes, which can be exponentially many: Dial restricts the computation to "efficient" routes, those never moving away from the destination, and propagates probabilities in a forward and a backward pass; Bell reformulates the logit assignment as a flow problem solved by linear equations on the graph.

These capture the real dispersion of route choices, and they cost considerably more in computation and in code. OpenGlassBox gets a comparable dispersion for free by other means: agents route at different moments, against travel times that have moved in between, and towards destinations chosen by availability. Adding a stochastic model on top of that would buy realism no player would see.

## If we wanted a real solver: `TrafficAssignmentSolver`

Should the fixed-step filter ever prove insufficient, the natural shape of the change is a class rather than a scattering of methods. `TrafficAssignmentSolver` would own the three things the current design does not have anywhere to put:

- the **flow buffer**, currently one `m_flow` per `Way` updated in place, which a real MSA needs to keep separate from the observed count $y^k$ of the current window;
- the **iteration counter** $k$, which does not exist because there is no iteration: smoothing happens once per tick, unconditionally;
- the **step policy**, the choice between $\lambda_k = 1/k$ and a fixed $\eta$, which is currently not a choice but a hard-coded formula.

The observed count of a window of $N$ ticks would play the part of the all-or-nothing $y^k$: the agents, all following the shortest route under the current weights, already perform that computation by walking. The solver would then update the average, recompute the travel times, and republish the weights the router reads.

The hook would be `City::update`, in the place `Path::smoothFlows` occupies today, or `World::update` if the averaging is ever to span several cities. Nothing else would need to change: `Way::travelTime` is already the single point every router reads.

A cheaper calibration reference, if the continuous BPR curve turns out to be hard to tune, is the official SimCity (2013) patch. Maxis weighted roads by **capacity tiers at 25 %, 50 % and 75 %**: as a road crosses each threshold it becomes less attractive to the following agents. That is the BPR curve of the first section discretised into three steps, and it is easier to reason about than $\alpha$ and $\beta$.

This is not implemented, and the current behaviour is not a placeholder waiting for it. Read the section above on why a fixed step is the right default for a living city.

## Where the coefficients live

| Coefficient | Where |
| --- | --- |
| $\alpha = 0.15$ of the BPR function | hard-coded in `Way::updateTravelTime`, `src/Path.cpp` |
| $\beta$, capacity $c_a$, speed limit | `WayType` in `include/OpenGlassBox/Types.hpp`, set per segment type in the `.ogs` ruleset |
| Smoothing weight $\alpha$ of the moving average | `SimulationConfig::trafficSmoothing`, `include/OpenGlassBox/Config.hpp` |
| Route re-examination period and threshold | `SimulationConfig::pathCheckTicks`, `pathRecalcTicks`, `pathCostDeviation` |

Nothing here is a script keyword beyond what `segment` already accepts. See the [script language reference](script.md) and the [engine documentation](engine.md).
