# Simulation files

Two files, not three:

| File | Role |
|------|------|
| **`.ogs`** | Ruleset (the mod): resources, rules, maps, paths, segments, agents, units, areas. |
| **`.ogc`** | City save: header (ruleset name + SHA-256 of that `.ogs` + types in use), geometry, and live state (clock, globals, map cells, units, agents, way flows). |

A third file (rules / geometry / state split) is not used. The save points at the ruleset it was written against. On load, a hash mismatch or a missing type is a hard error (`this save expects type Dirt; the open ruleset does not define it`).

Shipped scenarios:

- `test_city.ogs` + `test_city.ogc` — RCI sandbox (Home / Work / Shop, pollution, desirability, area growth).
- `braess.ogs` + `braess.ogc` — four-node Braess paradox.
- `regular.ogs` + `regular.ogc` — CiudadSim-style `Regular(6,6)` grid, **bidirectional** ways.
- `chicago.ogs` + `chicago.ogc` — simplified downtown arteries (not the 546-node Scilab `chisincen.net`).

`New city` picks a `.ogs` and starts empty. `Apply` in the Script panel reparses the `.ogs` and keeps the geometry when every type still placed is still defined.

## `.ogs` language

Blocks are `name … end`. `#` starts a comment. Identifiers are bare words.

### `resources`

```
resources
    resource Water
    resource People
end
```

Named stocks. Units, maps, agents and globals all refer to these names.

### `paths` and `segments`

```
paths
    path Road color 0xAAAAAA
end

segments
    segment Dirt color 0xAAAAAA speed 30 capacity 20 beta 4
end
```

A `path` is a graph family (the road network). A `segment` (`WayType`) is a pavement: free-flow `speed`, `capacity` (agents that saturate the BPR curve) and `beta` (BPR exponent, default 4).

### `agents`

```
agents
    agent Worker color 0xFFFFFF speed 10
end
```

Mobile tokens. `speed` is world units per second. They carry resources and look for a Unit `target`.

### `maps`

```
maps
    map Water color 0x0000FF capacity 100 rules [ ]
    map Pollution color 0x555555 capacity 80 rules [ SpreadPollution ]
end
```

Shared 2D fields on the world grid. Units read and write them inside `mapRadius`.

### `units`

```
units
    unit Home color 0xFF00FF mapRadius 1
        rules [ SendPeopleToWork ]
        targets [ Home ]
        caps [ People 4 ]
        resources [ People 4 ]
end
```

Buildings. `caps` bound local bins. `targets` is the name Agents use to find them. A Unit is **not** a graph node: it may sit on a Way (offset), on a Node, or free in the world.

### `areas`

```
areas
    area Residential color 0x44AA44 rules [ GrowHomes AbandonHomes ]
end
```

RCI-style zones. Area rules run over the painted rectangle (`spawn`, `upgrade`, `destroy`, `count`).

### `rules`

```
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

Every command of a rule must succeed; the first refusal aborts the rule for that tick.

| Kind | Typical commands |
|------|------------------|
| `mapRule` | `map Name add\|remove\|greater\|less N`, optional `randomTilesPercent` |
| `unitRule` | `local`, `global`, `map`, `agent Type to Target add [ Res N ]`, `hour between A B` |
| `areaRule` | `count Type less\|greater N`, `spawn Type at nearestWay`, `upgrade`, `destroy Type` |

`rate` is the period in ticks. `hour between` uses the simulation clock (wraps midnight when `A > B`).

## TestCity recipe

What the original GlassBox talk described, mapped onto this ruleset:

- **Maps** — Water, Grass, Pollution, Desirability, Trash, Power. Fields, not objects.
- **Units** — Home / Work / Shop (RCI). Bounded resource bins, rules on a period.
- **Agents** — People, Worker, Shopper. The traffic *is* the agents.
- **Paths** — one `Road` graph, `Dirt` ways. Routing is a pipe, not the gameplay.
- **Areas** — Residential / Commercial with `GrowHomes`, `GrowShops`, `AbandonHomes`.

The clock and `hour between` were not in the C# port; they drive the commute.

## `.ogc` header

```
save
    ruleset test_city.ogs
    hash <sha256 of the .ogs bytes>
    types [ Road Dirt Home Work Shop Residential ]
end
```

Then `clock`, `city Name size U V`, `globals`, `path` / `node` / `way`, `unit`, `area`, `map` cells, `agent`.
