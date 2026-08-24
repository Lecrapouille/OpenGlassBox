# Scripts and the `.ogs` language

**Scripts are the heart of OpenGlassBox.** The C++ engine is a host: it advances time, routes agents, and executes rules. The **gameplay lives in `.ogs` files**. Change a script and you change the game—new building types, resources, traffic behaviour, zone growth—without recompiling the engine.

That is the idea behind GlassBox (Willmott, GDC 2012): a city simulation can be **data** rather than a tree of objects with an `Update()` method. OpenGlassBox adds **Areas** (RCI zones) on top of the original MultiAgentSimulation model. For most users and contributors, the scripts are the most important part of the project; the demo is only one way to edit and watch them run.

A ruleset combines five concepts, connected by **rules**:

| Concept | Role |
| ------- | ---- |
| **Maps** | 2D fields (water, pollution, desirability, …). |
| **Units** | Buildings with bounded resource stocks. |
| **Agents** | Travellers that carry resources between units. |
| **Paths** | Road (or rail) networks made of nodes and segments. |
| **Areas** | Zones whose rules spawn, upgrade, and remove buildings. |

Rules are atomic: every command in a rule must validate before any command runs. Buildings use `unitRule`, map layers use `mapRule`, and zones use `areaRule`.

Two file formats exist:

- **`.ogs`**: the ruleset: resources, types, and rules.
- **`.ogc`**: a city save: geometry and live state, tied to the ruleset it was created with.

The [bundled simulations](../demo/data/Simulations/README.md) include ready-made examples; start with `test_city.ogs`.

## `.ogs` language reference

An `.ogs` file consists of named blocks closed by `end`. A `#` starts a comment, identifiers are bare words, and block order does not matter.

### `resources`

```text
resources
    resource Water
    resource People
end
```

Resources are named quantities shared by the rest of the ruleset. Units, maps, agents, and city-wide globals all refer to these names.

### `paths` and `segments`

```text
paths
    path Road color 0xAAAAAA
end

segments
    segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
end
```

A `path` defines a network family, such as roads or rails. A `segment` (`WayType`) defines one kind of connection: its free-flow `speed`, `capacity` at which congestion becomes significant, and BPR exponent `beta` (4 by default).

### `agents`

```text
agents
    agent Worker color 0xFFFFFF speed 10
end
```

Agents are mobile entities. Their `speed` is expressed in world units per second. They carry resources and search for a unit matching a `target`.

### `maps`

```text
maps
    map Water color 0x0000FF capacity 100 rules [ ]
    map Pollution color 0x555555 capacity 80 rules [ SpreadPollution ]
end
```

Maps are shared 2D fields on the world grid. Units read and modify nearby map cells within their `mapRadius`.

### `units`

```text
units
    unit Home color 0xFF00FF mapRadius 1
        rules [ SendPeopleToWork ]
        targets [ Home ]
        caps [ People 4 ]
        resources [ People 4 ]
end
```

Units are buildings. `caps` sets local resource capacities, `resources` sets initial amounts, and `targets` lists the names agents use to find the building. A unit is **not** a graph node: it may be anchored to a segment or node, or stand freely in the world.

### `areas`

```text
areas
    area Residential color 0x44AA44 rules [ GrowHomes AbandonHomes ]
end
```

Areas are RCI-style zones. Their rules operate on the painted rectangle with commands such as `spawn`, `upgrade`, `destroy`, and `count`.

### `rules`

```text
rules
    mapRule CreateGrass
        rate 7
        map Water remove 10 randomTilesPercent 90
        map Grass add 1
    end

    unitRule SendPeopleToWork
        rate 20
        hour between 8 18
        local People remove 1
        agent Worker to Work add [ People 1 ]
    end

    areaRule GrowHomes
        rate 80
        count Home less 10
        spawn Home at nearestWay
    end
end
```

Every command in a rule must validate before any command is executed. If one refuses, the whole rule is skipped for that tick.

- `mapRule` uses map commands and can select cells with `randomTilesPercent`.
- `unitRule` uses `local`, `global`, `map`, `agent Type to Target add [ Res N ]`, and `hour between A B`.
- `areaRule` uses `count`, `spawn`, `upgrade`, and `destroy` to manage buildings in a zone.

`hour between` uses the simulation clock and wraps around midnight when `A > B`.

### Periods

`rate` says how often a rule is attempted. A bare number counts simulation ticks; add a unit to express the period in readable game time:

```text
    unitRule ProduceGoods
        rate 30 minutes
    end

    areaRule AbandonHomes
        rate 1 day
    end
```

At the default 20 ticks per game minute, `rate 1 minute` is 20 ticks, `rate 30 minutes` is 600 ticks, `rate 2 hours` is 2,400 ticks, and `rate 1 day` is 28,800 ticks.

Write the time unit on the same line as the number, because `hour` is also a command. For example, `rate 1` followed by `hour between 8 18` means one tick, not one hour.

Durations are converted to ticks when rules run. Changing `SimulationConfig::ticksPerMinute` therefore rescales the whole ruleset. A zero period is invalid.

## `.ogc` save structure

```text
save
    ruleset test_city.ogs
    hash <sha256 of the .ogs bytes>
    types [ Road Dirt Home Work Shop Residential ]
end
```

The header is followed by the clock, `city Name size U V`, globals, paths, nodes, segments, units, areas, map cells, and agents. Traffic flow is also saved so a loaded city does not treat every road as empty.

A save identifies the ruleset it was created with. Loading fails if a required type is missing or the ruleset hash differs. During ruleset development, the demo can open saves with a stale checksum; required types must still exist.

See [engine documentation](engine.md#saving-citysave) for how the loader uses the fingerprint.
