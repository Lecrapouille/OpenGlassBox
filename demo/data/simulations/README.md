# Bundled simulations

This folder holds the rulesets and city saves shipped with the demo. For the `.ogs` language itself, see the [script language reference](../../../doc/script.md).

## File formats

- **`.ogs`**: ruleset (or mod): resources, rules, layers, paths, segments, agents, buildings, and zones.
- **`.ogc`**: city save: header (ruleset name, SHA-256 of the `.ogs`, types in use), geometry, and live state (clock, globals, layer cells, buildings, agents, segment flows).

A separate world file is not needed. A save identifies the ruleset it was created with. Loading fails if a required type is missing or the ruleset hash differs. **Apply** in the Script panel stamps the saves sitting beside the ruleset with its new fingerprint, so editing a script here does not leave the bundled cities unopenable. Failing that, File → **Open saves with a stale checksum** bypasses the check; required types must still exist.

**New city** selects a `.ogs` ruleset and starts with an empty city. **Apply** in the Script panel reparses the current ruleset and keeps the geometry if every placed type remains defined.

## Bundled scenarios

Three rulesets are shipped. Each save file points to one of them.

- `simple.ogs`: a small RCI ruleset with four building types (Home, Work, Shop, Restaurant). Every block is explained line by line. **Start here** if you want to learn the script language. One city comes with it:
    - `simple.ogc`: a small 12×12 hand-drawn town on a dirt road loop.
- `sandbox.ogs`: the full ruleset. Wealth ladders, services with a budget, utilities, waste, health, transport, tourism, trade, and a route to high technology. Open this once you know the basics. Four cities are built on it:
    - `sandbox.ogc`: the small town the ruleset was written for, with its utilities and services already placed.
    - `simple2.ogc`: a larger hand-drawn town.
    - `regular.ogc`: CiudadSim-style `Regular(6,6)` grid, **bidirectional** segments.
    - `chicago.ogc`: simplified downtown arteries (not the 546-node Scilab `chisincen.net`). Two orders of magnitude larger than the others, which is what the router is measured on.
- `braess.ogs` + `braess.ogc`: four-node Braess paradox. A ruleset of its own, since the point of it is three segment types whose speed and capacity are chosen so that the shortcut makes everybody slower.

## Worked example: `simple`

`simple.ogs` is the best starting point. It is short, and it is commented from top to bottom.

**A rule is all or nothing.** The engine reads every line of a rule first. If one line fails, nothing runs, and the rule is tried again later. A house cannot send a worker it does not have, and a shop cannot sell goods it never received.

**The day follows the clock.** At 08:00 houses send workers to the factories. Factories turn workers into goods, pollute the neighbourhood, and pay the treasury. Trucks carry goods to the shops and restaurants. Between 10:00 and 20:00 residents left at home go shopping. Between 12:00 and 14:00 workers eat lunch. Between 18:00 and 22:00 everybody goes home.

**Pollution closes the loop.** Factories raise pollution on the ground. High pollution lowers desirability. Low desirability makes houses leave the district. Fewer houses mean less traffic and less pollution over time.

**Zones grow buildings.** The player paints Residential, Commercial, or Industrial zones. Zone rules count the buildings inside and spawn new ones next to the nearest road. A zone with no road in reach grows nothing.

**Layers and buildings are different things.** Layers are numbers on the ground (water, grass, pollution, desirability). Buildings sit on the map and hold resources. Rules on a building read and write its own stock; rules on a layer read and write the cells around the building, up to its `layerRadius`.

### When nothing seems to happen

A zone grows nothing without a road in reach. A building that finds nowhere to deliver wanders for two game hours (`SimulationConfig::agentGiveUpTicks`) and then hands its load back. Agents searching without ever finding usually mean the city is short of a building type: a single factory holds twelve workers, so a district of houses needs an Industrial zone, or more factories, before everybody has somewhere to go.

## Worked example: `sandbox`

`sandbox.ogs` is the full game. It is also commented line by line, but much longer. Three ideas hold it together.

**Wealth and density are two different things.** Density comes from the zone the player paints: `ResidentialLow` grows small buildings and `ResidentialHigh` grows tall ones. Wealth comes from an `upgrade`: every zone spawns its poorest building and improves it, or ruins it, according to the ground around it. Three densities times three levels of wealth gives nine residential types and nine commercial ones. All nine houses answer to `Home`, so a worker sent home at six in the evening reaches whichever house is nearest, whatever it is called and however rich the district became.

**The treasury and the budget are two different things.** `Money` is what the city owns; `PoliceBudget` is the share the player grants the police. A rich city that grants nothing has no patrol at all. The **Budget** panel of the demo turns those dials, and each service grades its effect with an `onFail` chain: a large share buys a large effect at a large cost, and a share of nothing buys nothing. So an idle service has two possible causes, and the Rule Log names which command blocked.

**Every system is a closed loop.** A cause, an effect, and a way back to the cause. Pollution reduces the value of the land, a low value ruins the houses, and fewer houses produce less pollution. A system with no way back is a number on a screen rather than a game.

### The day

The clock and `hour between` were not in the C# port; they drive the day.

- **08:00**: houses send workers out. A new simulation opens at that hour rather than at midnight (`SimulationConfig::startHour`; the clock panel sets it). A household lets one resident leave for work every three quarters of an hour, so the commute never empties it.
- **Factories** turn people into goods, pollute their neighbourhood, and pay the treasury. A factory would rather build from ore or from oil and falls back on plain labour, which is another `onFail` chain.
- **Goods** reach the shops on a `Truck`, since workers carry people and not stock. Without that freight rule the shops have nothing to sell.
- **10:00–20:00**: residents left at home go shopping. A shop only sells to customers standing in it (`local People greater 0` in `SellGoods`), so money follows traffic instead of appearing on its own.
- **12:00–14:00**: workers eat: a `Diner` heads for the nearest `Restaurant`, and `SendDinersBack` returns them before 15:00. A restaurant answers to `Restaurant` only and is supplied by its own freight rule.
- **18:00–22:00**: everybody goes home.
- **21:00–03:00**: the rich districts have a night life. That range crosses midnight, which `hour between` reads by itself when the first hour is the larger one.

### The route to high technology

This is the longest loop in the file and the one worth following. A district becomes desirable, its houses upgrade to `Villa`, `Townhouse` or `Condo`, and only those three carry `SendStudentsToUniversity`. The university consumes the student, raises `Skills` over its whole district, and sends the graduate home. A district with enough skills turns its `FactoryClean` into a `TechCampus`, which produces more and fouls nothing. So a city reaches high technology through its rich neighbourhoods, and there is no other road.

Every step of that chain is reversible. A district that loses its university loses its skills, and `HighTechLeavesForLackOfSkills` downgrades the campus back.

### Reading the ground

The layers say what an address is worth. `LandValue` is what a surveyor would measure: clean air, quiet, no jams, a park, a school, public transport within reach. `Desirability` is what a resident feels, and it follows the land value plus safety and health. Both are slow on purpose, in hours and days: a district that is built and abandoned within one afternoon reads as a bug, not as a city.

Pollution, noise, crime and the rest travel and fade through the `diffusion` and `decay` of their layer rather than through rules, which is what carries the smoke of a factory to the street beside it. See the [script language reference](../../../doc/script.md).

### When nothing seems to happen

A zone grows nothing without a road in reach, without water and power on the plot, and without the matching demand in the city accounts. The city hall is what produces those demands, so a map needs exactly one.

A building that finds nowhere to deliver wanders for two game hours (`SimulationConfig::agentGiveUpTicks`) and then hands its load back to the building that sent it out. Agents searching without ever finding usually mean the city is short of a building type: a single factory holds twelve workers, so a district of houses needs an Industrial zone, or more factories, before everybody has somewhere to go.

## Worked example: `braess`

This ruleset exists to show **Braess's paradox**. In 1968 Dietrich Braess proved something that feels wrong: if every driver picks the fastest route for himself, adding a new road can make *everyone* slower. The new link looks like a shortcut, so too many drivers take it, it jams, and the average trip takes longer than before the road was built.

`braess.ogs` is not a city. It is a small network built for that demonstration: four nodes, two parallel routes of equal cost, and one extra link in the middle that plays the role of the tempting shortcut. Agents leave `Start` and are absorbed at `End`. The router sends each one on what looks fastest *right now*, the same way the demo routes traffic in a real city.

**What to look for.** Open `braess.ogc` and watch the **Traffic** panel. At low traffic both main routes share the load and trips stay short. Raise the number of agents until the shortcut fills up: total travel time goes up even though a "better" road exists. The heatmap shows the shortcut turning red first; the relative gap and the total travel time tell you when the assignment has settled.

Three segment types carry the paradox:

- **Fast**: high speed, low capacity : good when it is empty, bad when it is full.
- **Slow**: low speed, high capacity : never fast, but it absorbs a crowd.
- **Shortcut**: very high speed, very low capacity : the road that should help and ends up hurting once enough agents use it.

The speeds and capacities were chosen on purpose so the effect is easy to see. The point is not to build a town, but to watch selfish routing produce a result no planner would want.
