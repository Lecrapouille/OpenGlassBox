# Scripts and the `.ogs` language

**Scripts are the heart of OpenGlassBox.** The C++ engine is a host: it advances time, routes agents, and executes rules. The **gameplay lives in `.ogs` files**. Change a script and you change the game:new building types, resources, traffic behaviour, zone growth:without recompiling the engine.

That is the idea behind GlassBox (Willmott, GDC 2012): a city simulation can be **data** rather than a tree of objects with an `Update()` method. OpenGlassBox adds **Zones** (RCI zones) on top of the original MultiAgentSimulation model. For most users and contributors, the scripts are the most important part of the project; the demo is only one way to edit and watch them run.

A ruleset combines five concepts, connected by **rules**:

| Concept | Role |
| ------- | ---- |
| **Layers** | 2D fields (water, pollution, desirability, …). |
| **Buildings** | Buildings with bounded resource stocks. |
| **Agents** | Travellers that carry resources between buildings. |
| **Paths** | Road (or rail) networks made of nodes and segments. |
| **Zones** | Zones whose rules spawn, upgrade, and remove buildings. |

Rules are atomic: every command in a rule must validate before any command runs. Buildings use `buildingRule`, layers use `layerRule`, and zones use `zoneRule`.

Two file formats exist:

- **`.ogs`**: the ruleset: resources, types, and rules.
- **`.ogc`**: a city save: geometry and live state, tied to the ruleset it was created with.

The [bundled simulations](../demo/data/simulations/README.md) include ready-made examples; start with `sandbox.ogs`.

## `.ogs` language reference

An `.ogs` file consists of named blocks closed by `end`. A `#` starts a comment, identifiers are bare words, and block order does not matter.

### `resources`

```text
resources
    resource Water
    resource People
end
```

Resources are named quantities shared by the rest of the ruleset. Buildings, layers, agents, and city-wide globals all refer to these names.

#### Settings the player turns: the `Budget` and `Tax` convention

Two things a rule cannot do: set a value, and read the intent of a player. So a ruleset that wants a dial declares a resource for it and lets the interface be its only author. The demo picks the dials out by name:

| Name | Panel | Range | Meaning |
| --- | --- | --- | --- |
| ends in `Budget` | Budget | 0 to 100 | Share of its asking price a service is granted. |
| starts with `Tax` | Budget | 0 to 20 | Rate a kind of building pays. |

`PoliceBudget` appears as a *Police* slider and `TaxResidential` as a *Residential* one. Nothing in the engine knows these names: the panel walks the resources the script declared and keeps the ones that match, so a ruleset that invents `LibraryBudget` gets its slider without a line of C++. A dial missing from a city, which is the case of a new city and of a save written before the service existed, is created at a sensible value on the first draw rather than read as zero.

This is what separates the treasury from the budget. `Money` is a stock the rules move; a dial is a setting the player moves. A rich city that grants nothing has no service at all, and the two causes of an idle service, an empty treasury and a closed dial, block different commands and read differently in the Rule Log. Use an `onFail` chain to grade the effect.

### `paths` and `segments`

```text
paths
    path Road color 0xAAAAAA
    path Water color 0x0000FF crossings false
end

segments
    segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
end
```

A `path` defines a network family, such as roads or rails. A `segment` (`SegmentType`) defines one kind of connection: its free-flow `speed`, `capacity` at which congestion becomes significant, and BPR exponent `beta` (4 by default). These three are the parameters of the travel time the router minimises; see the [traffic documentation](traffic.md#travel-time-on-a-road-the-bpr-function) for what they do and how to tune them.

`crossings` says whether two lines of the network that cross make a junction agents can turn at. It is true unless written otherwise, which is what a road network wants: a street drawn over another is a crossroads, and both are cut so that the graph says what the picture shows. Write `crossings false` for the networks where one line running over another means nothing, such as a water main under a power line; a junction there is then made by hand, with the Nodes tool of the demo. `Path::findCrossings` is what an editor of your own would call.

### `agents`

```text
agents
    agent Worker color 0xFFFFFF speed 10
end
```

Agents are mobile entities. Their `speed` is expressed in world units per second. They carry resources and search for a building matching a `target`.

### `layers`

```text
layers
    layer Water color 0x0000FF capacity 20 decay 10 rate 2 hours rules [ ]
    layer Pollution color 0x555555 capacity 80 diffusion 24 decay 10 rate 30 minutes rules [ ]
end
```

Layers are shared 2D fields on the world grid. Buildings read and write the cells within their `layerRadius`.

`capacity` is the most one cell holds. It is also the unit a building writes in: `layer Water add 400` hands the 400 out cell by cell, in random order, filling each one to `capacity` before moving to the next. It does not give 400 to every cell in the radius, and it does not spread the 400 evenly over them. So a supply is a flow and the capacity is the size of one cell: a tower that writes 400 into a layer of capacity 20 serves twenty cells per period.

`diffusion` and `decay` are the transport of the layer, and they are properties rather than rules because a rule only ever reads and writes the one cell it stands on.

| Field | Meaning |
| --- | --- |
| `diffusion` | Percent of a cell that moves to its four neighbours each period. The four share it equally and the remainder stays, so the total over the map is unchanged. |
| `decay` | Percent of a cell that disappears each period. A cell that holds something always loses at least one unit, so it empties instead of settling just above zero. |
| `rate` | The period of diffusion and decay, in the same syntax as a rule. Not the period of the rules of the layer: each rule carries its own. |

The two shares are taken from the same amount, so `diffusion + decay` may not exceed 100 and the parser refuses a layer where it does. Both default to zero, which is a field that only rules move.

Choose the three together to say how far something travels and how long it lasts. Noise travels far and stops the moment the lorry has passed, so it takes a high diffusion, a high decay and a short period. What lands in the soil hardly moves and hardly leaves, so it takes a low diffusion, a low decay and a period of a day.

### `buildings`

```text
buildings
    building Shack color 0x8B6F5A layerRadius 1
        rules [ SendPeopleToWork ]
        targets [ Home ]
        caps [ People 4 ]
        resources [ People 3 ]
end
```

`caps` sets local resource capacities, `resources` sets initial amounts, and `targets` lists the names agents use to find the building. A resource absent from `caps` is one the building cannot hold at all. A building is **not** a graph node: it may be anchored to a segment or node, or stand freely in the world.

The type name and the targets are two different things. `Shack` is what the ruleset and the saves call this building; `Home` is the role it fills for an agent. Several types answer to one role, which is how a ladder of poor, ordinary and rich houses receives the same commute.

### `zones`

```text
zones
    zone ResidentialLow color 0x66CC66
        rules [ GrowShack UpgradeShackToHouse DowngradeHouseToShack AbandonShack ]
end
```

Zones are RCI-style zones. Their rules operate on the painted rectangle with commands such as `spawn`, `upgrade`, `destroy`, and `count`. A zone holds no building of its own: the buildings inside it belong to the city, and the rectangle is only what decides which rules apply to them.

Density belongs to the zone, so a ruleset carries one zone per density and lets the wealth ladder run inside each.

### `rules`

```text
rules
    layerRule CreateGrass
        rate 20 minutes
        layer Water remove 1 randomTilesPercent 10
        layer Grass add 1
    end

    buildingRule SendPeopleToWork
        rate 45 minutes
        hour between 8 18
        local People greater 0
        local People remove 1
        agent Worker to Work add [ People 1 ]
    end

    zoneRule GrowShack
        rate 2 hours
        layer Water greater 10
        global ResidentialDemand remove 1
        count Shack less 4
        spawn Shack at nearestSegment
    end
end
```

Every command in a rule must validate before any command is executed. If one refuses, the whole rule is skipped for that tick.

- `layerRule` uses layer commands and can select cells with `randomTilesPercent`. It runs on one cell at a time and reads no other, which is why `diffusion` and `decay` are recipes of the layer and not rules. It owns no resources either, so `local` is refused in a layer rule: write `layer` to reach the cell, or `global` to reach the city.
- `buildingRule` uses `local`, `global`, `layer`, `agent Type to Target add [ Res N ]`, and `hour between A B`.
- `zoneRule` uses `count`, `spawn`, `upgrade`, and `destroy` to manage buildings in a zone.

`hour between` uses the simulation clock and wraps around midnight when `A > B`.

#### `onFail`: a fallback, and how to grade an effect

A `buildingRule` may end with `onFail OtherRule`. When the rule refuses, the engine runs that one instead, and so on down the chain.

```text
    buildingRule PatrolFull
        rate 2 hours
        global PoliceBudget greater 66
        global Money remove 3
        layer Crime remove 6
        onFail PatrolHalf
    end

    buildingRule PatrolHalf
        rate 2 hours
        global PoliceBudget greater 33
        global Money remove 2
        layer Crime remove 3
    end
```

Only the first rule of a chain belongs in the `rules [ ]` of a building; the engine walks the rest. Three points follow from that:

- Only the period of the first rule counts. The fallbacks are tried in the same pass, so give them the same `rate` for a reader and know that the engine ignores theirs.
- A chain has to go one way. The parser refuses a rule that names itself, but two rules naming each other would loop for ever.
- Since a rule is all or nothing, a chain is the only way to write a graded effect. Three separate rules would each refuse on their own conditions and the building would simply stop.

The same shape covers a production line that would rather use ore, falls back on oil, and ends with plain labour when no mine and no well supply it.

#### Which cell a zone rule reads

A `layer` line inside a `zoneRule` reads the plot the next building would stand on: a free cell of the rectangle that fronts a road. A rectangle with no plot left, because it is full, reads its centre instead. The reach is that one cell, so `layer Water greater 10` means the same amount whatever the size the player painted.

#### `upgrade` and `destroy`

`upgrade A to B` replaces one building in place. It keeps the plot on the street and carries the stock over, clipped by the `caps` of the new type: a resource the new type cannot hold is a resource the upgrade drops. `destroy A` removes the emptiest one first, so a district loses the building nobody lives in before the one that is full.

Together these are how a wealth ladder is written. Density belongs to the zone the player paints and wealth to the ladder: a zone spawns its poorest type, upgrades it as the ground around it improves, and downgrades it on the opposite conditions. An `upgrade` in both directions is what lets a district recover instead of only decaying.

An agent looks for a name in `targets`, never for a type name, which is what makes the ladder work: nine different houses all answering to `Home` receive the same `agent Worker to Home` without a single rule naming any of them.

### Periods

`rate` says how often a rule is attempted. A bare number counts simulation ticks; add a unit to express the period in readable game time:

```text
    buildingRule ProduceGoods
        rate 30 minutes
    end

    zoneRule AbandonShack
        rate 1 day
    end
```

At the default 20 ticks per game minute, `rate 1 minute` is 20 ticks, `rate 30 minutes` is 600 ticks, `rate 2 hours` is 2,400 ticks, and `rate 1 day` is 28,800 ticks.

Write the time unit on the same line as the number, because `hour` is also a command. For example, `rate 1` followed by `hour between 8 18` means one tick, not one hour.

Durations are converted to ticks when rules run. Changing `TimeConfig::ticksPerMinute` therefore rescales the whole ruleset. A zero period is invalid.

## `.ogc` save structure

```text
save
    ruleset sandbox.ogs
    hash <sha256 of the .ogs bytes>
    types [ Road Dirt Home Work Shop Residential ]
end
```

The header is followed by the clock, `city Name size U V`, globals, paths, nodes, segments, buildings, zones, layer cells, and agents. Traffic flow is also saved so a loaded city does not treat every road as empty.

A save identifies the ruleset it was created with. Loading fails if a required type is missing or the ruleset hash differs.

A save names its ruleset by file name and is looked for beside itself first, so the saves a ruleset can invalidate are the `.ogc` files in its own directory. Editing a script breaks all of them at once, which is why **Apply** in the demo stamps them with the new fingerprint: only that header line is rewritten, and the types a save names are still required to exist. `CitySave::savesUsingRuleset` and `CitySave::restamp` are what a tool of your own would call. Failing that, **Open stale saves** in the Script panel waives the check.

See [engine documentation](engine.md#saving-citysave) for how the loader uses the fingerprint.
