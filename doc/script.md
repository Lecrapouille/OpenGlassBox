# Scripts and the `.ogs` language

**Scripts are the heart of OpenGlassBox.** The C++ engine is a host: it advances time, routes agents, and executes rules. The **gameplay lives in `.ogs` files**. Change a script and you change the game—new building types, resources, traffic behaviour, zone growth—without recompiling the engine.

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

The [bundled simulations](../demo/data/Simulations/README.md) include ready-made examples; start with `sandbox.ogs`.

## `.ogs` language reference

An `.ogs` file consists of named blocks closed by `end`. A `#` starts a comment, identifiers are bare words, and block order does not matter.

### `resources`

```text
resources
    resource Water
    resource People
end
```

Resources are named quantities shared by the rest of the ruleset. Units, layers, agents, and city-wide globals all refer to these names.

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
    layer Water color 0x0000FF capacity 100 rules [ ]
    layer Pollution color 0x555555 capacity 80 rules [ SpreadPollution ]
end
```

Layers are shared 2D fields on the world grid. Units read and modify nearby layer cells within their `layerRadius`.

### `buildings`

```text
buildings
    building Home color 0xFF00FF layerRadius 1
        rules [ SendPeopleToWork ]
        targets [ Home ]
        caps [ People 4 ]
        resources [ People 4 ]
end
```

Units are buildings. `caps` sets local resource capacities, `resources` sets initial amounts, and `targets` lists the names agents use to find the building. A unit is **not** a graph node: it may be anchored to a segment or node, or stand freely in the world.

### `zones`

```text
zones
    zone Residential color 0x44AA44 rules [ GrowHomes AbandonHomes ]
end
```

Zones are RCI-style zones. Their rules operate on the painted rectangle with commands such as `spawn`, `upgrade`, `destroy`, and `count`.

### `rules`

```text
rules
    layerRule CreateGrass
        rate 7
        layer Water remove 10 randomTilesPercent 90
        layer Grass add 1
    end

    buildingRule SendPeopleToWork
        rate 20
        hour between 8 18
        local People remove 1
        agent Worker to Work add [ People 1 ]
    end

    zoneRule GrowHomes
        rate 80
        count Home less 10
        spawn Home at nearestSegment
    end
end
```

Every command in a rule must validate before any command is executed. If one refuses, the whole rule is skipped for that tick.

- `layerRule` uses layer commands and can select cells with `randomTilesPercent`.
- `buildingRule` uses `local`, `global`, `layer`, `agent Type to Target add [ Res N ]`, and `hour between A B`.
- `zoneRule` uses `count`, `spawn`, `upgrade`, and `destroy` to manage buildings in a zone.

`hour between` uses the simulation clock and wraps around midnight when `A > B`.

### Periods

`rate` says how often a rule is attempted. A bare number counts simulation ticks; add a unit to express the period in readable game time:

```text
    buildingRule ProduceGoods
        rate 30 minutes
    end

    zoneRule AbandonHomes
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
