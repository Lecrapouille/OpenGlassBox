# Simulation files

Two files, not three:

| File | Role |
|------|------|
| **`.ogs`** | Ruleset (the mod): resources, rules, maps, paths, segments, agents, units, areas. |
| **`.ogc`** | City save: header (ruleset name + SHA-256 of that `.ogs` + types in use), geometry, and live state (clock, globals, map cells, units, agents, way flows). |

A third file (rules / geometry / state split) is not used. The save points at the ruleset it was written against. On load, a missing type is a hard error (`this save expects type Dirt; the open ruleset does not define it`), and so is a hash mismatch unless File → `Open saves with a stale checksum` is ticked, which is how a ruleset gets written without re-stamping every save at each keystroke. The Script panel computes the checksum and re-stamps the open save.

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

`hour between` uses the simulation clock (wraps midnight when `A > B`).

### Periods

`rate` says how often a rule is attempted. A bare number counts simulation ticks; add a unit and it counts game time, which is what a reader can reason about:

```
    unitRule ProduceGoods
        rate 30 minutes
    end

    areaRule AbandonHomes
        rate 1 day
    end
```

| Written | Ticks, at the default 20 ticks per game minute |
|---------|------------------------------------------------|
| `rate 7` or `rate 7 ticks` | 7 |
| `rate 1 minute` | 20 |
| `rate 30 minutes` | 600 |
| `rate 2 hours` | 2400 |
| `rate 1 day` | 28800 |

The unit word must sit on the same line as the number, because `hour` is also a command: a rule that reads `rate 1` and then `hour between 8 18` on the next line asks for one tick, not one hour. A duration is converted to ticks when the rule runs rather than when it is parsed, so changing `SimulationConfig::ticksPerMinute` rescales the whole ruleset. A period of zero is refused.

## TestCity recipe

What the original GlassBox talk described, mapped onto this ruleset:

- **Maps** — Water, Grass, Pollution, Desirability, Trash, Power. Fields, not objects.
- **Units** — Home / Work / Shop / Restaurant. Bounded resource bins, rules on a period.
- **Agents** — People, Worker, Shopper, Diner, Truck. The traffic *is* the agents.
- **Paths** — one `Road` graph, `Dirt` ways. Routing is a pipe, not the gameplay.
- **Areas** — Residential, Commercial and Industrial, so a city can be built out of zones alone.

The clock and `hour between` were not in the C# port; they drive the day:

- 08:00 the houses send workers out, and a new simulation opens at that hour rather than at midnight (`SimulationConfig::startHour`, and the clock panel of the demo sets it). A household holds eight and lets one leave for work every three quarters of an hour, so the commute never empties it: what shares the residents between the factories and the shops is the period of the rules against the size of the house, and a condition such as `local People greater 1` is not needed;
- the factories turn people into goods, pollute their neighbourhood and pay the treasury;
- goods reach the shops on a `Truck`, since the workers carry people, not stock. Without that freight rule the shops had nothing to sell;
- 10:00 to 20:00 the residents left at home go shopping, and a shop only sells to the customers standing in it (`local People greater 0` in `SellGoods`), so the money follows the traffic instead of appearing on its own;
- 12:00 to 14:00 the workers eat: a `Diner` heads for the nearest `Restaurant`, and `SendDinersBack` returns them before 15:00. A restaurant answers to `Restaurant` only and is supplied by its own freight rule, `ShipFood`. It used to answer to `Shop` as well, so a canteen standing closer than the shop caught every truck and every shopper, and the shops were never opened;
- 18:00 to 22:00 everybody goes home.

A building that finds nowhere to deliver wanders for two game hours (`SimulationConfig::agentGiveUpTicks`) and then hands its load back to the building that sent it out. Agents searching without ever finding usually mean the city is short of a building type: a single factory holds twelve workers, so a district of houses needs an Industrial zone, or more factories, before everybody has somewhere to go.

Desirability and the growth of the zones are counted in hours and days: a district that is built and abandoned inside an afternoon reads as a bug, not as a city.

## `.ogc` header

```
save
    ruleset test_city.ogs
    hash <sha256 of the .ogs bytes>
    types [ Road Dirt Home Work Shop Residential ]
end
```

Then `clock`, `city Name size U V`, `globals`, `path` / `node` / `way`, `unit`, `area`, `map` cells, `agent`.
